#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <algorithm>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/package.h"
#include "parser/macro_expander.h"
#include "parser/build_eval.h"
#include "codegen/codegen.h"
#include "shared/lsp.h"
#include "shared/version.h"
#include "embedded_runtime.h"
#include "bind/bind.h"
#ifdef BRICK_TRACK_BLOCKS
#include "memvis.h"
#endif

#if defined(_WIN32)
#  include <windows.h>
#  include <direct.h>
#  include <process.h>
#  define getpid()       _getpid()
#  define mkdir(p, m)    _mkdir(p)
#  define WEXITSTATUS(s) ((int)((s) & 0xff))
#  define PATH_SEP       '\\'
#  define PATH_SEP_STR   "\\"
#  define TMPDIR_BASE    (getenv("TEMP") ? getenv("TEMP") : "C:\\Temp")
#  define TMPDIR_PREFIX  "brick-"
#  define SHELL_CMD(c)   ("cmd /c \"" c "\"")
#else
#  include <unistd.h>
#  include <sys/stat.h>
#  include <sys/wait.h>
#  include <signal.h>
#  define PATH_SEP       '/'
#  define PATH_SEP_STR   "/"
#  define TMPDIR_BASE    "/tmp"
#  define TMPDIR_PREFIX  "brick-"
#  define SHELL_CMD(c)   (c)
#endif

void print_usage() {
    std::cerr << "Brick Compiler v" << BRICK_VERSION_STRING << "\n";
    std::cerr << "Usage:\n";
    std::cerr << "  brick <input.brc> [-o output.c] [--lsp]\n";
    std::cerr << "  brick build <file1.brc> [<file2.brc> ...] [-o output] [--release]\n";
    std::cerr << "                    [--include <path>] [--emit-c-only]\n";
    std::cerr << "  brick run   <input.brc> [--release]\n";
#ifdef BRICK_TRACK_BLOCKS
    std::cerr << "  brick --visualize <file>   compile, run and visualize memory blocks\n";
    std::cerr << "  brick --attach <pid>       attach visualizer to running process\n";
#endif
    std::cerr << "  brick bind <c_header.h>   generate .brc bindings from C header\n";
    std::cerr << "  brick new <project_name>  create a new multi-file project scaffold\n";
    std::cerr << "  brick --help\n";
    std::cerr << "  brick --version\n";
    std::cerr << "\n";
    std::cerr << "  --release       omit tracking overhead (max performance, no visualizer)\n";
    std::cerr << "  --emit-c-only   only generate .c + runtime sources, do not link\n";
    std::cerr << "  --include <dir> add C include path for compilation\n";
    std::cerr << "  -I <dir>        add search path for package resolution\n";
    std::cerr << "\n";
    std::cerr << "Packages: using PACKAGE auto-resolves <PACKAGE>.brc, <PACKAGE>/<PACKAGE>.brc,\n";
    std::cerr << "          or <PACKAGE>/main.brc in current dir, -I paths, and BRICK_PATH.\n";
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "error: could not open " << path << "\n";
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static bool write_file(const std::string& path, const char* data, size_t len) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    fwrite(data, 1, len, f);
    fclose(f);
    return true;
}

static void ensure_parent_dirs(const std::string& path) {
    size_t pos = 0;
    while ((pos = path.find_first_of("/\\", pos)) != std::string::npos) {
        std::string sub = path.substr(0, pos);
        mkdir(sub.c_str(), 0755);
        pos++;
    }
}

static std::string get_prog_suffix() {
    const char* target = brick::embedded::runtime_info.target;
    if (strcmp(target, "windows") == 0) return ".exe";
    return "";
}

static bool create_temp_dir(std::string& out_path, const char* prefix) {
#if defined(_WIN32)
    char temp_path[MAX_PATH];
    DWORD ret = GetTempPathA(MAX_PATH, temp_path);
    if (ret == 0 || ret > MAX_PATH) return false;

    char dir_name[MAX_PATH];
    // Use process ID + time-based unique name instead of mkdtemp
    snprintf(dir_name, sizeof(dir_name), "%s%s%d-%ld",
             temp_path, prefix, (int)GetCurrentProcessId(), (long)time(NULL));

    if (!CreateDirectoryA(dir_name, NULL)) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // Append a counter
            for (int i = 0; i < 1000; i++) {
                snprintf(dir_name, sizeof(dir_name), "%s%s%d-%ld-%d",
                         temp_path, prefix, (int)GetCurrentProcessId(), (long)time(NULL), i);
                if (CreateDirectoryA(dir_name, NULL)) break;
                if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
            }
        } else {
            return false;
        }
    }
    out_path = dir_name;
    return true;
#else
    char tmpdir[] = "/tmp/brick-build-XXXXXX";
    if (!mkdtemp(tmpdir)) return false;
    out_path = tmpdir;
    return true;
#endif
}

// ─── Multi-file compilation state ──────────────────────────
// ─── Estado de compilacao multi-arquivo ─────────────────────
struct CompileState {
    brick::PackageTable packages;
    std::vector<std::unique_ptr<brick::ProgramNode>> asts;
    std::vector<std::string> parsed_files;
};

// Try all case variants of a filename: exact, lowercase, uppercase
// Tenta todas as variantes de maiusculas/minusculas de um nome de arquivo
static bool try_access_variants(const std::string& dir, const std::string& name,
                                 const std::string& subname, std::string& out) {
    // Pattern: <dir>/<name>.brc
    // Padrao: <dir>/<name>.brc
    char path[1024];

    // Try exact case
    if (subname.empty()) {
        int n = snprintf(path, sizeof(path), "%s/%s.brc", dir.c_str(), name.c_str());
        if (n > 0 && (size_t)n < sizeof(path) && access(path, F_OK) == 0) {
            out = path; return true;
        }
        // Try lowercase
        std::string lower = name;
        for (auto& c : lower) c = std::tolower((unsigned char)c);
        n = snprintf(path, sizeof(path), "%s/%s.brc", dir.c_str(), lower.c_str());
        if (n > 0 && (size_t)n < sizeof(path) && access(path, F_OK) == 0) {
            out = path; return true;
        }
    } else {
        // Pattern: <dir>/<name>/<subname>.brc or <dir>/<name>/main.brc
        // Padrao: <dir>/<name>/<subname>.brc ou <dir>/<name>/main.brc
        // Try exact case
        int n = snprintf(path, sizeof(path), "%s/%s/%s.brc",
                         dir.c_str(), name.c_str(), subname.c_str());
        if (n > 0 && (size_t)n < sizeof(path) && access(path, F_OK) == 0) {
            out = path; return true;
        }
        // Try lowercase name
        std::string lower = name;
        for (auto& c : lower) c = std::tolower((unsigned char)c);
        n = snprintf(path, sizeof(path), "%s/%s/%s.brc",
                     dir.c_str(), lower.c_str(), subname.c_str());
        if (n > 0 && (size_t)n < sizeof(path) && access(path, F_OK) == 0) {
            out = path; return true;
        }
        // Try lowercase subname (for the <name>/<name>.brc pattern)
        std::string lower_sub = subname;
        for (auto& c : lower_sub) c = std::tolower((unsigned char)c);
        n = snprintf(path, sizeof(path), "%s/%s/%s.brc",
                     dir.c_str(), name.c_str(), lower_sub.c_str());
        if (n > 0 && (size_t)n < sizeof(path) && access(path, F_OK) == 0) {
            out = path; return true;
        }
        // Try both lowercase
        n = snprintf(path, sizeof(path), "%s/%s/%s.brc",
                     dir.c_str(), lower.c_str(), lower_sub.c_str());
        if (n > 0 && (size_t)n < sizeof(path) && access(path, F_OK) == 0) {
            out = path; return true;
        }
    }
    return false;
}

// Search for a package file on disk, supporting nested (dotted) package names
// Procura por um arquivo de pacote no disco, suportando nomes aninhados (com ponto)
// e.g. "MATH.VEC2" -> tries <dir>/MATH/VEC2.brc, <dir>/MATH/VEC2/VEC2.brc, <dir>/MATH/VEC2/main.brc
static std::string find_package_file(const std::string& name,
                                      const std::vector<std::string>& search_paths) {
    std::string found;

    // Try current directory first
    // Tenta diretorio atual primeiro
    {
        std::string tmp;
        if (try_access_variants(".", name, "", tmp)) found = tmp;
    }
    if (!found.empty()) return found;

    // For nested package names (e.g., "MATH.VEC2"), try subdirectory paths
    // Para nomes de pacote aninhados (ex: "MATH.VEC2"), tenta caminhos de subdiretorio
    std::string subdir_name = name;
    auto dot = subdir_name.find('.');
    if (dot != std::string::npos) {
        // Convert dots to directory separators: "MATH.VEC2" -> "MATH/VEC2"
        // Converte pontos em separadores de diretorio: "MATH.VEC2" -> "MATH/VEC2"
        std::string dir_path = subdir_name;
        for (auto& c : dir_path) if (c == '.') c = PATH_SEP;

        // Get the last segment for the filename pattern (e.g., "VEC2")
        // Obtem o ultimo segmento para o padrao de nome de arquivo (ex: "VEC2")
        std::string last_seg = dir_path.substr(dir_path.rfind(PATH_SEP) + 1);

        // Try current dir: <dir_path>.brc (e.g., "MATH/VEC2.brc")
        // Tenta dir atual: <dir_path>.brc (ex: "MATH/VEC2.brc")
        if (try_access_variants(".", dir_path, "", found)) return found;

        for (const auto& dir : search_paths) {
            // Try <dir>/<dir_path>.brc (e.g., "pkgs/MATH/VEC2.brc")
            if (try_access_variants(dir, dir_path, "", found)) return found;

            // Try <dir>/<dir_path>/<last_seg>.brc (e.g., "pkgs/MATH/VEC2/VEC2.brc")
            if (try_access_variants(dir, dir_path, last_seg, found)) return found;

            // Try <dir>/<dir_path>/main.brc (e.g., "pkgs/MATH/VEC2/main.brc")
            if (try_access_variants(dir, dir_path, "main", found)) return found;
        }
        if (!found.empty()) return found;
    }

    // Also try flat name patterns in each search path
    // Tambem tenta padroes de nome simples em cada caminho de busca
    for (const auto& dir : search_paths) {
        // Pattern: <dir>/<name>.brc
        if (try_access_variants(dir, name, "", found)) return found;

        // Pattern: <dir>/<name>/<name>.brc
        if (try_access_variants(dir, name, name, found)) return found;

        // Pattern: <dir>/<name>/main.brc
        if (try_access_variants(dir, name, "main", found)) return found;
    }

    return "";
}

// Parse a .brc file and merge it into the compilation state
// Analisa um arquivo .brc e mescla no estado de compilacao
static void process_brc_file(CompileState& state, const std::string& path) {
    // Avoid re-parsing the same file
    // Evita re-analisar o mesmo arquivo
    for (const auto& f : state.parsed_files) {
        if (f == path) return;
    }

    std::string source = read_file(path);
    auto tokens = brick::tokenize(source, path);
    auto parse_result = brick::parse(tokens);
    if (!parse_result.errors.empty()) {
        for (const auto& e : parse_result.errors)
            std::cerr << "error: " << e << "\n";
        exit(1);
    }
    if (!parse_result.ast) exit(1);

    auto local_packages = brick::resolve_packages(parse_result.ast, path);
    brick::merge_package_tables(state.packages, std::move(local_packages));
    state.asts.push_back(std::move(parse_result.ast));
    state.parsed_files.push_back(path);
}

// Auto-resolve package dependencies from filesystem
// Auto-resolve dependencias de pacotes do sistema de arquivos
static void resolve_package_deps(CompileState& state,
                                  const std::vector<std::string>& search_paths) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& ast : state.asts) {
            for (const auto& decl : ast->declarations) {
                if (decl->type != brick::ASTNodeType::USING_DECL)
                    continue;
                auto* ud = static_cast<brick::UsingDecl*>(decl.get());

                // Build full package name
                // Constroi nome completo do pacote
                std::string pkg_name;
                for (size_t i = 0; i < ud->package_parts.size(); i++) {
                    if (i > 0) pkg_name += ".";
                    pkg_name += ud->package_parts[i];
                }

                // Skip IO (built-in runtime)
                // Pula IO (runtime embutido)
                if (pkg_name == "IO") continue;

                // Skip if already resolved
                // Pula se ja resolvido
                if (state.packages.packages.count(pkg_name)) continue;

                // Try to find the package file on disk using the full dotted name
                // Tenta encontrar o arquivo do pacote no disco usando o nome completo com pontos
                std::string found = find_package_file(pkg_name, search_paths);
                if (found.empty()) {
                    std::cerr << "error: package '" << pkg_name << "' not found\n";
                    std::cerr << "  Searched in current dir and -I paths.\n";
                    std::cerr << "  Set BRICK_PATH to add more search directories.\n";
                    exit(1);
                }

                process_brc_file(state, found);
                changed = true;
                break; // restart scanning since asts changed
            }
            if (changed) break;
        }
    }
}

// Run the full compilation pipeline: macros, type check, codegen
// Executa o pipeline completo de compilacao: macros, type check, codegen
static brick::CodegenResult run_pipeline(CompileState& state) {
    // 1. Macro collection
    // 1. Coleta de macros
    brick::MacroTable macro_table;
    brick::collect_macros(state.asts, macro_table);

    // 2. Evaluate build blocks
    // 2. Avalia blocos build
    auto build_result = brick::eval_build_blocks(state.asts, macro_table);
    if (!build_result.success) {
        for (const auto& e : build_result.errors)
            std::cerr << "error: " << e << "\n";
        brick::CodegenResult err;
        err.success = false;
        return err;
    }

    // 3. Expand macros
    // 3. Expande macros
    auto expand_result = brick::expand_macros(state.asts, macro_table);
    if (!expand_result.success) {
        for (const auto& e : expand_result.errors)
            std::cerr << "error: " << e << "\n";
        brick::CodegenResult err;
        err.success = false;
        return err;
    }

    // 4. Code generation (includes type checking)
    // 4. Geracao de codigo (inclui verificacao de tipos)
    return brick::generate_c(state.asts, state.packages);
}

static int cmd_build(const std::vector<std::string>& inputs,
                     const std::string& output,
                     bool run_mode, bool release_mode,
                     bool emit_c_only = false,
                     const std::vector<std::string>& include_paths = {}) {
    // 1. Temp dir for build artifacts
    // 1. Diretorio temporario para artefatos de build
    std::string tmp;
    if (!create_temp_dir(tmp, TMPDIR_PREFIX "build-")) {
        std::cerr << "error: could not create temp directory\n";
        return 1;
    }

    // 2. Extract embedded runtime files
    // 2. Extrai arquivos de runtime embutidos
    auto& info = brick::embedded::runtime_info;
    for (int i = 0; i < info.num_sources; i++) {
        auto& f = info.sources[i];
        std::string fp = tmp + PATH_SEP_STR + f.name;
        ensure_parent_dirs(fp);
        if (!write_file(fp, f.content, f.length)) {
            std::cerr << "error: could not write " << fp << "\n";
            return 1;
        }
    }

    // 3. Compile all .brc files -> .c
    // 3. Compila todos arquivos .brc -> .c
    CompileState state;
    for (const auto& input : inputs) {
        process_brc_file(state, input);
    }

    // Auto-resolve package dependencies (using PACKAGE)
    // Auto-resolve dependencias de pacotes (using PACKAGE)
    resolve_package_deps(state, include_paths);

    // Run full pipeline: macros -> type check -> codegen
    // Executa pipeline completo: macros -> type check -> codegen
    auto codegen_result = run_pipeline(state);
    for (const auto& e : codegen_result.errors)
        std::cerr << "error: " << e << "\n";
    if (!codegen_result.success) return 1;

    std::string c_path = tmp + PATH_SEP_STR + "_gen.c";
    {
        std::ofstream out(c_path);
        if (!out.is_open()) {
            std::cerr << "error: could not write " << c_path << "\n";
            return 1;
        }
        out << codegen_result.c_code;
    }

    // 4. Collect .c sources
    // 4. Coleta fontes .c
    std::string srcs;
    for (int i = 0; i < info.num_sources; i++) {
        std::string name = info.sources[i].name;
        size_t len = name.size();
        if (len >= 2 && name.substr(len - 2) == ".c")
            srcs += " \"" + tmp + PATH_SEP_STR + name + "\"";
    }

    // 5. Collect link flags
    // 5. Coleta flags de linkagem
    std::string flags;
    for (int i = 0; i < info.num_flags; i++)
        flags += " " + std::string(info.flags[i]);
    for (const auto& lf : codegen_result.link_flags)
        flags += " -l" + lf;

    // 6. Assemble include flags
    // 6. Monta flags de include
    std::string inc_flags = " -I\"" + tmp + "\"";
    for (const auto& inc_path : include_paths) {
        inc_flags += " -I\"" + inc_path + "\"";
    }

    // Track mode (used by both emit-c-only and normal build)
    // Modo track (usado tanto por emit-c-only quanto build normal)
    std::string track = release_mode ? "" : " -DBRICK_TRACK_BLOCKS";

    // 6b. Emit-C-Only mode: just write .c and extract runtime, no linking
    // 6b. Modo Emit-C-Only: so escreve .c e extrai runtime, sem linkar
    if (emit_c_only) {
        std::string c_output = output;
        if (c_output.size() < 2 || c_output.substr(c_output.size() - 2) != ".c") {
            c_output += ".c";
        }
        // Write generated C code
        {
            std::ifstream gen(c_path);
            std::ofstream out(c_output);
            if (out.is_open() && gen.is_open()) {
                out << gen.rdbuf();
                out.close();
            }
        }
        // Copy runtime sources alongside
        // Copia fontes do runtime ao lado
        for (int i = 0; i < info.num_sources; i++) {
            auto& f = info.sources[i];
            std::string dest = output + "_" + f.name;
            ensure_parent_dirs(dest);
            write_file(dest, f.content, f.length);
        }
        std::cerr << "[build] c-only: wrote " << c_output << " and runtime sources\n";
        std::cerr << "[build] compile with: " << info.cc << " -O3" << track << inc_flags
                  << " \"" << c_output << "\"" << srcs << " -o <output>" << flags << "\n";
        return 0;
    }

    // 6c. Invoke C compiler
    // 6c. Invoca compilador C
    std::string bin = run_mode ? (tmp + PATH_SEP_STR + "_run" + get_prog_suffix()) : output;
    std::string cmd = std::string(info.cc) + " -O3" + track + inc_flags
        + " \"" + c_path + "\"" + srcs + " -o \"" + bin + "\"" + flags;

    std::cerr << "[build] " << info.cc << " -O3 ... -o " << bin << "\n";
    int ret = system(cmd.c_str());
#if !defined(_WIN32)
    ret = WEXITSTATUS(ret);
#endif

    if (ret != 0) {
        std::cerr << "[build] failed (exit " << ret << ")\n";
        // Keep temp dir for debugging
        // Mantem diretorio temp para debug
        std::cerr << "[build] artifacts left at " << tmp << "\n";
        return ret;
    }

    // 7. Move binary to final location if built to temp
    // 7. Move binario para local final se construido em temp
    if (run_mode) {
#if defined(_WIN32)
        std::string mv = "move /Y \"" + bin + "\" \"" + output + "\"";
#else
        std::string mv = "mv \"" + bin + "\" \"" + output + "\"";
#endif
        system(mv.c_str());
    }

    // 8. Cleanup temp
    // 8. Limpa diretorio temporario
#if defined(_WIN32)
    std::string rm = "rmdir /S /Q \"" + tmp + "\"";
#else
    std::string rm = "rm -rf \"" + tmp + "\"";
#endif
    system(rm.c_str());

    std::cerr << "[build] " << output << " ready\n";
    return 0;
}

static int cmd_bind(const std::string& header_path) {
    brick::bind::Options opts;
    auto bind_result = brick::bind::generate(header_path, opts);
    if (!bind_result.success) {
        for (const auto& err : bind_result.errors)
            std::cerr << "error: " << err << "\n";
        return 1;
    }

    std::string output = header_path;
    auto pos = output.rfind('.');
    if (pos != std::string::npos) output = output.substr(0, pos);
    output += "_bindings.brc";

    std::ofstream out(output);
    if (!out.is_open()) {
        std::cerr << "error: could not write " << output << "\n";
        return 1;
    }
    out << bind_result.brc_code;
    out.close();

    std::cerr << "[bind] wrote " << output << " ("
              << std::count(bind_result.brc_code.begin(), bind_result.brc_code.end(), '\n')
              << " lines)\n";
    return 0;
}

// Generate a new multi-file project scaffold
// Gera um projeto multi-arquivo novo
static int cmd_new(const std::string& name) {
    std::string dir = name;
    std::string lib_dir = dir + "/lib";
    std::string lib_pkg = dir + "/lib/MATH.brc";
    std::string main_brc = dir + "/main.brc";

    // Create directories
    // Cria diretorios
    mkdir(dir.c_str(), 0755);
    mkdir(lib_dir.c_str(), 0755);

    // Create lib/MATH.brc — library package
    // Cria lib/MATH.brc — pacote de biblioteca
    {
        std::string content =
            std::string("package MATH\n") +
            "\n" +
            "// ── Exported interface ──\n" +
            "export interface Damageable {\n" +
            "    fn take_damage(i32 dmg)\n" +
            "}\n" +
            "\n" +
            "// ── Exported struct with methods ──\n" +
            "export struct Player {\n" +
            "    i32 hp\n" +
            "    String name\n" +
            "\n" +
            "    fn Player(i32 h, String n) {\n" +
            "        hp = h\n" +
            "        name = n\n" +
            "    }\n" +
            "\n" +
            "    fn take_damage(i32 dmg) {\n" +
            "        hp -= dmg\n" +
            "        if hp < 0 { hp = 0 }\n" +
            "    }\n" +
            "}\n" +
            "\n" +
            "// ── Exported function ──\n" +
            "export fn add(i32 a, i32 b) -> i32 {\n" +
            "    return a + b\n" +
            "}\n" +
            "\n" +
            "// ── Exported constant ──\n" +
            "export const MAX_ENEMIES = 100\n" +
            "\n" +
            "// ── Private helper (not exported) ──\n" +
            "private fn internal_helper() -> i32 {\n" +
            "    return 999\n" +
            "}\n";
        write_file(lib_pkg, content.c_str(), content.size());
    }

    // Create main.brc — entry point
    // Cria main.brc — ponto de entrada
    {
        std::string content =
            std::string("package ") + name + "\n" +
            "\n" +
            "using IO\n" +
            "using MATH\n" +
            "\n" +
            "block global = 64MB\n" +
            "block game = 16MB\n" +
            "\n" +
            "fn main() {\n" +
            "    // Use exported function\n" +
            "    i32 sum = add(3, 4)\n" +
            "    print(\"3 + 4 = {0}\", sum)\n" +
            "\n" +
            "    // Use exported const\n" +
            "    i32 max = MAX_ENEMIES\n" +
            "    print(\"max enemies = {0}\", max)\n" +
            "\n" +
            "    // Use exported struct with constructor\n" +
            "    Player p = Player(100, \"Hero\") @game\n" +
            "    print(\"{0} has {1} HP\", p.name, p.hp)\n" +
            "\n" +
            "    // Use interface via virtual dispatch\n" +
            "    Damageable d = p\n" +
            "    d.take_damage(20)\n" +
            "\n" +
            "    game.reset()\n" +
            "    global.reset()\n" +
            "}\n";
        write_file(main_brc, content.c_str(), content.size());
    }

    // Create README
    // Cria README
    {
        std::string readme = "# " + name + "\n\n"
            "A Brick project.\n\n"
            "## Build and Run\n\n"
            "```bash\n"
            "brick build main.brc -I lib -o " + name + " --release\n"
            "./" + name + "\n"
            "```\n\n"
            "## Debug Build\n\n"
            "```bash\n"
            "brick build main.brc -I lib -o " + name + "\n"
            "./" + name + "\n"
            "```\n";
        write_file(dir + "/README.md", readme.c_str(), readme.size());
    }

    std::cerr << "[new] created project '" << name << "'\n";
    std::cerr << "[new]   " << main_brc << "\n";
    std::cerr << "[new]   " << lib_pkg << "\n";
    std::cerr << "[new]   " << dir << "/README.md\n";
    std::cerr << "[new] build with: brick build " << dir << "/main.brc -I "
              << dir << "/lib -o " << name << " --release\n";
    return 0;
}

static int cmd_run(const std::string& input) {
    // Build to a temp binary
    // Compila para binario temporario
    std::string tmpdir;
    if (!create_temp_dir(tmpdir, TMPDIR_PREFIX "run-")) {
        std::cerr << "error: could not create temp directory\n";
        return 1;
    }
    std::string out = tmpdir + PATH_SEP_STR + "prog" + get_prog_suffix();

    int ret = cmd_build({input}, out, false, false, false, {});
    if (ret != 0) {
#if defined(_WIN32)
        std::string rm = "rmdir /S /Q \"" + tmpdir + "\"";
#else
        std::string rm = "rm -rf \"" + tmpdir + "\"";
#endif
        system(rm.c_str());
        return ret;
    }

    // Execute
    // Executa
    std::cerr << "[run] " << out << "\n";
    ret = system(out.c_str());
#if !defined(_WIN32)
    ret = WEXITSTATUS(ret);
#endif

    // Cleanup
    // Limpeza
#if defined(_WIN32)
    std::string rm = "rmdir /S /Q \"" + tmpdir + "\"";
#else
    std::string rm = "rm -rf \"" + tmpdir + "\"";
#endif
    system(rm.c_str());

    return ret;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string subcommand;
    std::string input_file;
    std::string output_file;
    bool lsp_mode = false;
    int arg_idx = 1;

    // Check for subcommand (build / run)
    // Verifica subcomando (build / run)
    std::string first = argv[1];
    if (first == "bind") {
        if (argc < 3) {
            std::cerr << "error: missing C header file\n";
            return 1;
        }
        return cmd_bind(argv[2]);
    }
    if (first == "new") {
        if (argc < 3) {
            std::cerr << "error: missing project name\n";
            return 1;
        }
        return cmd_new(argv[2]);
    }
    if (first == "build" || first == "run") {
        subcommand = first;
        arg_idx = 2;
        if (argc < 3) {
            std::cerr << "error: missing input file\n";
            return 1;
        }
    }

    // ─── Visualizer flags ──────────────────────────────────────
    // ─── Flags do Visualizador ──────────────────────────────────────
    bool visualize_mode = false;
    int attach_pid = 0;
#ifdef BRICK_TRACK_BLOCKS
    for (int i = arg_idx; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--visualize") {
            visualize_mode = true;
        }
        if (arg == "--attach" && i + 1 < argc) {
            attach_pid = std::stoi(argv[++i]);
        }
    }
#else
    (void)visualize_mode;
    (void)attach_pid;
#endif

    bool build_release = false;
    bool emit_c_only = false;
    std::vector<std::string> include_paths;
    std::vector<std::string> input_files;

    for (int i = arg_idx; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "--version") {
            std::cout << "Brick Compiler v" << BRICK_VERSION_STRING << "\n";
            return 0;
        } else if (arg == "--lsp") {
            lsp_mode = true;
        } else if (arg == "--release") {
            build_release = true;
        } else if (arg == "--emit-c-only") {
            emit_c_only = true;
        } else if (arg == "--include" && i + 1 < argc) {
            include_paths.push_back(argv[++i]);
        } else if (arg == "-I" && i + 1 < argc) {
            include_paths.push_back(argv[++i]);
        } else if (arg == "--visualize") {
            // handled above
        } else if (arg == "--attach") {
            i++; // skip PID arg
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else {
            input_files.push_back(arg);
        }
    }

    // ─── Visualize mode: compile + run + attach TUI ────────────
    // ─── Modo visualizar: compila + executa + anexa TUI ────────
#ifdef BRICK_TRACK_BLOCKS
    if (visualize_mode) {
        if (input_file.empty()) {
            std::cerr << "error: --visualize requires a .brc file (or try --attach <pid>)\n";
            return 1;
        }

        // Build to temp binary
        // Compila para binario temporario
        std::string tmpdir;
        if (!create_temp_dir(tmpdir, TMPDIR_PREFIX "viz-")) {
            std::cerr << "error: could not create temp directory\n";
            return 1;
        }
        std::string bin = tmpdir + PATH_SEP_STR + "prog" + get_prog_suffix();
        int ret = cmd_build({input_file}, bin, false, false, false, {});
        if (ret != 0) {
#if defined(_WIN32)
            std::string rm = "rmdir /S /Q \"" + tmpdir + "\"";
#else
            std::string rm = "rm -rf \"" + tmpdir + "\"";
#endif
            system(rm.c_str());
            return ret;
        }

#if defined(_WIN32)
        // Windows: use CreateProcess instead of fork/exec
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        memset(&pi, 0, sizeof(pi));
        si.cb = sizeof(si);
        char cmdline[2048];
        snprintf(cmdline, sizeof(cmdline), "\"%s\"", bin.c_str());
        if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            std::cerr << "error: could not launch " << bin << "\n";
            return 1;
        }
        int pid = (int)pi.dwProcessId;

        Sleep(150); // 150ms wait for shm

        MemVisConfig cfg = MEMVIS_DEFAULT_CONFIG;
        std::cerr << "[viz] attached to PID " << pid << " ...\n";
        memvis_attach(pid, cfg);

        TerminateProcess(pi.hProcess, 0);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        std::string rm_cmd = "rmdir /S /Q \"" + tmpdir + "\"";
#else
        int pid = fork();
        if (pid == 0) {
            execl(bin.c_str(), bin.c_str(), nullptr);
            _exit(1);
        }

        usleep(150000); // 150ms

        MemVisConfig cfg = MEMVIS_DEFAULT_CONFIG;
        std::cerr << "[viz] attached to PID " << pid << " ...\n";
        memvis_attach(pid, cfg);

        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        std::string rm_cmd = "rm -rf \"" + tmpdir + "\"";
#endif
        system(rm_cmd.c_str());
        return 0;
    }

    if (attach_pid > 0) {
        MemVisConfig cfg = MEMVIS_DEFAULT_CONFIG;
        memvis_attach(attach_pid, cfg);
        return 0;
    }
#endif

    if (input_files.empty()) {
        std::cerr << "error: no input file\n";
        return 1;
    }
    input_file = input_files[0]; // first file is the "main" file for output naming

    // ─── build / run ───────────────────────────────────────────
    // ─── build / run ───────────────────────────────────────────
    if (subcommand == "build") {
        if (output_file.empty()) {
            auto pos = input_file.rfind('.');
            output_file = (pos != std::string::npos)
                ? input_file.substr(0, pos) : input_file;
        }
        if (lsp_mode)
            std::cerr << "warning: --lsp has no effect in build mode\n";
        // Also add BRICK_PATH env var directories to search paths
        // Tambem adiciona diretorios da variavel BRICK_PATH aos caminhos de busca
        const char* brick_path = getenv("BRICK_PATH");
        if (brick_path) {
            std::string bp = brick_path;
            size_t start = 0, colon;
            while ((colon = bp.find(':', start)) != std::string::npos) {
                include_paths.push_back(bp.substr(start, colon - start));
                start = colon + 1;
            }
            if (start < bp.size())
                include_paths.push_back(bp.substr(start));
        }
        return cmd_build(input_files, output_file, false, build_release,
                          emit_c_only, include_paths);
    }

    if (subcommand == "run") {
        if (lsp_mode)
            std::cerr << "warning: --lsp has no effect in run mode\n";
        if (build_release)
            std::cerr << "warning: --release has no effect in run mode (use with build)\n";
        return cmd_run(input_file);
    }

    // ─── Compile only (backward compatible) ────────────────────
    // ─── Compile only (backward compatible) ────────────────────
    if (output_file.empty()) {
        auto pos = input_file.rfind('.');
        output_file = (pos != std::string::npos
            ? input_file.substr(0, pos) : input_file) + ".c";
    }

    // Also add BRICK_PATH env var directories to search paths for package resolution
    // Tambem adiciona diretorios da variavel BRICK_PATH aos caminhos de busca
    const char* brick_path = getenv("BRICK_PATH");
    if (brick_path) {
        std::string bp = brick_path;
        size_t start = 0, colon;
        while ((colon = bp.find(':', start)) != std::string::npos) {
            include_paths.push_back(bp.substr(start, colon - start));
            start = colon + 1;
        }
        if (start < bp.size())
            include_paths.push_back(bp.substr(start));
    }

    std::string source = read_file(input_file);

    brick::LspOutput lsp_out;

    auto tokens = brick::tokenize(source, input_file);
    if (lsp_mode) {
        lsp_out.tokens = tokens;
    } else {
        std::cout << "[lexer] " << tokens.size() << " tokens\n";
    }

    auto parse_result = brick::parse(tokens);
    if (!parse_result.errors.empty()) {
        for (const auto& err : parse_result.errors) {
            if (lsp_mode) {
                lsp_out.errors.push_back(
                    brick::parse_error_string(err, input_file));
            } else {
                std::cerr << "error: " << err << "\n";
            }
        }
        if (!lsp_mode) return 1;
    }

    if (!parse_result.ast) {
        if (!lsp_mode) return 1;
        std::cout << brick::emit_lsp_json(lsp_out);
        return 1;
    }
    if (!lsp_mode) std::cout << "[parser] AST built\n";

    auto packages = brick::resolve_packages(parse_result.ast, input_file);
    if (!lsp_mode) std::cout << "[package] resolved\n";

    std::vector<std::unique_ptr<brick::ProgramNode>> asts;
    asts.push_back(std::move(parse_result.ast));

    // Macro pipeline: collect -> eval build -> expand
    // Pipeline de macros: coleta -> avalia build -> expande
    brick::MacroTable macro_table;
    brick::collect_macros(asts, macro_table);
    if (!macro_table.empty() && !lsp_mode)
        std::cout << "[macro] " << macro_table.size() << " macros collected\n";

    auto build_result = brick::eval_build_blocks(asts, macro_table);
    for (const auto& e : build_result.errors) {
        if (lsp_mode) {
            lsp_out.errors.push_back(brick::parse_error_string(e, input_file));
        } else {
            std::cerr << "error: " << e << "\n";
        }
    }
    if (!build_result.success && !lsp_mode) return 1;

    auto expand_result = brick::expand_macros(asts, macro_table);
    for (const auto& e : expand_result.errors) {
        if (lsp_mode) {
            lsp_out.errors.push_back(brick::parse_error_string(e, input_file));
        } else {
            std::cerr << "error: " << e << "\n";
        }
    }
    if (!expand_result.success && !lsp_mode) return 1;
    if (!lsp_mode) std::cout << "[macro] expansion done\n";

    auto codegen_result = brick::generate_c(asts, packages);

    for (const auto& err : codegen_result.errors) {
        if (lsp_mode) {
            lsp_out.errors.push_back(
                brick::parse_error_string(err, input_file));
        } else {
            std::cerr << "error: " << err << "\n";
        }
    }
    if (!codegen_result.success && !lsp_mode) return 1;
    if (!lsp_mode) std::cout << "[codegen] C code generated\n";

    if (!asts.empty() && asts[0]) {
        brick::collect_symbols(asts[0].get(), lsp_out.symbols);
    }

    if (lsp_mode) {
        std::cout << brick::emit_lsp_json(lsp_out);
        return lsp_out.errors.empty() && codegen_result.success ? 0 : 1;
    }

    std::ofstream out(output_file);
    if (!out.is_open()) {
        std::cerr << "error: could not write " << output_file << "\n";
        return 1;
    }
    out << codegen_result.c_code;
    out.close();

    std::cout << "[output] " << output_file << "\n";
    return 0;
}
