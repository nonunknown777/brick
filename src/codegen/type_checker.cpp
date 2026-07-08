#include "type_checker.h"
#include <sstream>
#include <unordered_set>
#include <cstdint>
#include <cfloat>
#include <algorithm>
#include <cctype>

namespace brick {

// ─── Type helper utilities ───
// ─── Utilitarios de tipo ───

static bool is_dynamic_array_type(const std::string& t) {
    auto bracket = t.find('[');
    if (bracket == std::string::npos) return false;
    return t.size() >= bracket + 2 && t[bracket + 1] == ']';
}

static bool is_bitfield_type(const std::string& t) {
    if (t.size() < 2) return false;
    if (t[0] != 'u' && t[0] != 'i') return false;
    // Standard fixed-width types are NOT bitfields
    if (t == "u8" || t == "u16" || t == "u32" || t == "u64" ||
        t == "i8" || t == "i16" || t == "i32" || t == "i64") return false;
    for (size_t i = 1; i < t.size(); i++)
        if (!std::isdigit(t[i])) return false;
    int w = std::stoi(t.substr(1));
    return w >= 1 && w <= 64;
}

static int parse_bitfield_width(const std::string& t) {
    if (!is_bitfield_type(t)) return 0;
    return std::stoi(t.substr(1));
}

static std::string normalize_type(const std::string& t) {
    if (t == "byte" || t == "char") return "u8";
    if (t == "short") return "i16";
    if (t == "int" || t == "Int") return "i32";
    if (t == "long") return "i64";
    if (t == "float" || t == "Float") return "f32";
    if (t == "double" || t == "Double") return "f64";
    if (t == "U8") return "u8";
    if (t == "U16") return "u16";
    if (t == "U32") return "u32";
    if (t == "U64") return "u64";
    if (t == "I8") return "i8";
    if (t == "I16") return "i16";
    if (t == "I32") return "i32";
    if (t == "I64") return "i64";
    if (t == "F32") return "f32";
    if (t == "F64") return "f64";
    if (t == "Bool") return "bool";
    if (t == "Void") return "void";
    if (t == "Usize" || t == "USIZE") return "usize";
    if (t == "Isize" || t == "ISIZE") return "isize";
    if (is_bitfield_type(t)) return t;
    return t;
}

std::string TypeChecker::resolve_array_sizes(const std::string& type) {
    std::string result = type;
    size_t bracket = 0;
    while ((bracket = result.find('[', bracket)) != std::string::npos) {
        size_t close = result.find(']', bracket);
        if (close == std::string::npos) break;
        std::string inner = result.substr(bracket + 1, close - bracket - 1);
        if (!inner.empty() && !std::isdigit(inner[0]) && inner[0] != '-') {
            auto it = const_values_.find(inner);
            if (it != const_values_.end()) {
                result = result.substr(0, bracket + 1) +
                         std::to_string(it->second) +
                         result.substr(close);
                bracket = close + 1; // skip past the replaced value
                continue;
            }
        }
        bracket = close + 1;
    }
    return result;
}

static bool is_signed_int(const std::string& t) {
    std::string n = normalize_type(t);
    if (is_bitfield_type(n)) return n[0] == 'i';
    return n == "i8" || n == "i16" || n == "i32" || n == "i64" || n == "isize";
}

static bool is_unsigned_int(const std::string& t) {
    std::string n = normalize_type(t);
    if (is_bitfield_type(n)) return n[0] == 'u';
    return n == "u8" || n == "u16" || n == "u32" || n == "u64" || n == "usize" || n == "bool";
}

static bool is_type_keyword(const std::string& name) {
    return name == "int" || name == "float" || name == "bool" || name == "char" ||
           name == "byte" || name == "String" || name == "void" ||
           name == "u8" || name == "u16" || name == "u32" || name == "u64" ||
           name == "i8" || name == "i16" || name == "i32" || name == "i64" ||
           name == "f32" || name == "f64" || name == "usize" || name == "isize";
}

static bool is_integer_type(const std::string& t) {
    std::string n = normalize_type(t);
    if (is_bitfield_type(n)) return true;
    return is_signed_int(t) || is_unsigned_int(t);
}

static bool is_float_type(const std::string& t) {
    std::string n = normalize_type(t);
    return n == "f32" || n == "f64";
}

static int type_rank(const std::string& t) {
    std::string n = normalize_type(t);
    if (is_bitfield_type(n)) return 0;
    if (n == "u8" || n == "i8" || n == "bool") return 1;
    if (n == "u16" || n == "i16") return 2;
    if (n == "u32" || n == "i32") return 3;
    if (n == "u64" || n == "i64" || n == "usize" || n == "isize") return 4;
    if (n == "f32") return 5;
    if (n == "f64") return 6;
    return 0;
}

static std::string signed_type_for_rank(int r) {
    if (r <= 1) return "i8";
    if (r == 2) return "i16";
    if (r == 3) return "i32";
    return "i64";
}

static bool int_fits_in_type(int64_t value, const std::string& type, bool is_hex = false) {
    std::string n = normalize_type(type);
    uint64_t uval = static_cast<uint64_t>(value);
    if (is_bitfield_type(n)) {
        int w = parse_bitfield_width(n);
        if (w <= 0 || w > 64) return false;
        if (n[0] == 'u') {
            uint64_t max = (w >= 64) ? UINT64_MAX : ((1ULL << w) - 1);
            return is_hex ? (uval <= max) : (value >= 0 && (uint64_t)value <= max);
        } else {
            int64_t min = (w >= 64) ? INT64_MIN : -(1LL << (w - 1));
            int64_t max = (w >= 64) ? INT64_MAX : (1LL << (w - 1)) - 1;
            return value >= min && value <= max;
        }
    }
    if (is_hex) {
        if (n == "u8")  return uval <= UINT8_MAX;
        if (n == "u16") return uval <= UINT16_MAX;
        if (n == "u32") return uval <= UINT32_MAX;
        if (n == "u64") return true;
        if (n == "usize") return true;
        if (n == "i8")  return uval <= UINT8_MAX;
        if (n == "i16") return uval <= UINT16_MAX;
        if (n == "i32") return uval <= UINT32_MAX;
        if (n == "i64") return true;
        if (n == "isize") return true;
        return true;
    }
    if (n == "u8")  return value >= 0 && value <= UINT8_MAX;
    if (n == "u16") return value >= 0 && value <= UINT16_MAX;
    if (n == "u32") return value >= 0 && value <= UINT32_MAX;
    if (n == "u64") return value >= 0;
    if (n == "i8")  return value >= INT8_MIN  && value <= INT8_MAX;
    if (n == "i16") return value >= INT16_MIN && value <= INT16_MAX;
    if (n == "i32") return value >= INT32_MIN && value <= INT32_MAX;
    if (n == "i64") return true;
    if (n == "usize") return value >= 0;
    if (n == "isize") return true;
    return true;
}

static bool float_fits_in_type(double value, const std::string& type) {
    std::string n = normalize_type(type);
    if (n == "f32") return value >= -FLT_MAX && value <= FLT_MAX;
    if (n == "f64") return true;
    return true;
}

TypeChecker::TypeChecker(const PackageTable& packages)
    : packages(packages)
{
    declare_builtins();
}

void TypeChecker::declare_builtins() {
    // Built-in types are always known
    // Tipos embutidos sao sempre conhecidos
    // Push global scope
    // Empilha escopo global
    push_scope();
}

void TypeChecker::push_scope() {
    scopes.emplace_back();
}

void TypeChecker::pop_scope() {
    if (!scopes.empty()) scopes.pop_back();
}

void TypeChecker::declare(const std::string& name, const std::string& type,
                          bool is_private, bool is_param) {
    if (scopes.empty()) push_scope();
    auto& scope = scopes.back();
    std::string resolved = type;
    auto alias_it = type_aliases.find(resolved);
    if (alias_it != type_aliases.end()) resolved = alias_it->second;
    scope[name] = SymbolInfo{resolved, is_private, is_param};
}

SymbolInfo* TypeChecker::lookup(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

bool TypeChecker::is_type_known(const std::string& type_name) {
    // Pointer type: *T is known if T is known
    if (!type_name.empty() && type_name[0] == '*') {
        return is_type_known(type_name.substr(1));
    }

    // Function pointer type: fn(...)->...
    if (type_name.rfind("fn(", 0) == 0) {
        return true;
    }

    static const std::unordered_set<std::string> builtins = {
        "int", "float", "bool", "char", "String", "void", "block", "null",
        "u8", "u16", "u32", "u64",
        "i8", "i16", "i32", "i64",
        "f32", "f64",
        "usize", "isize",
        "byte", "short", "long",
    };
    if (builtins.count(type_name)) return true;
    if (struct_defs.count(type_name)) return true;
    if (interface_defs.count(type_name)) return true;
    if (union_defs.count(type_name)) return true;
    if (type_aliases.count(type_name)) return true;
    if (is_bitfield_type(type_name)) return true;

    // Check if it's an array type like "int[10]"
    // Verifica se e um tipo array como "int[10]"
    auto bracket = type_name.find('[');
    if (bracket != std::string::npos) {
        std::string base = type_name.substr(0, bracket);
        return is_type_known(base);
    }

    return false;
}

bool TypeChecker::is_struct_type(const std::string& type_name) {
    return struct_defs.count(type_name) > 0;
}

bool TypeChecker::is_interface_type(const std::string& type_name) {
    return interface_defs.count(type_name) > 0;
}

bool TypeChecker::is_union_type(const std::string& type_name) {
    return union_defs.count(type_name) > 0;
}

FieldDecl* TypeChecker::find_struct_field(StructDecl* sd, const std::string& name) {
    for (auto& f : sd->fields) {
        if (f->type == ASTNodeType::FIELD_DECL) {
            auto* fd = static_cast<FieldDecl*>(f.get());
            if (fd->name == name) return fd;
        }
    }
    if (!sd->extends.empty() && struct_defs.count(sd->extends)) {
        return find_struct_field(struct_defs[sd->extends], name);
    }
    return nullptr;
}

bool TypeChecker::can_assign(const std::string& from, const std::string& to) {
    // Resolve type aliases
    std::string f_alias = from;
    std::string t_alias = to;
    auto fit = type_aliases.find(f_alias);
    if (fit != type_aliases.end()) f_alias = fit->second;
    auto tit = type_aliases.find(t_alias);
    if (tit != type_aliases.end()) t_alias = tit->second;

    if (f_alias == t_alias) return true;
    if (f_alias == "null" || t_alias == "null") return true;

    // Function pointer type compatibility: fn(...)->ret = fn(...)->ret if same
    bool from_fn = f_alias.rfind("fn(", 0) == 0;
    bool to_fn = t_alias.rfind("fn(", 0) == 0;
    if (from_fn && to_fn) return f_alias == t_alias;

    // Pointer type compatibility: *T = *T, null = *T, *void = *T, String = *u8
    bool from_ptr = !f_alias.empty() && f_alias[0] == '*';
    bool to_ptr = !t_alias.empty() && t_alias[0] == '*';
    if (from_ptr && to_ptr) return f_alias == t_alias || f_alias == "*void" || t_alias == "*void";
    if (f_alias == "String" && t_alias == "*u8") return true;

    std::string f = normalize_type(f_alias);
    std::string t = normalize_type(t_alias);

    if (f == t) return true;

    // Enum types: treat as int for assignment compatibility
    // Tipos enum: trata como int para compatibilidade de atribuicao
    if (enum_defs.count(f) || enum_defs.count(t)) {
        std::string f_eff = enum_defs.count(f) ? "int" : f;
        std::string t_eff = enum_defs.count(t) ? "int" : t;
        return can_assign(f_eff, t_eff);
    }

    // Struct → interface: check if struct implements the interface
    // Struct → interface: verifica se struct implementa a interface
    if (is_interface_type(t) && struct_defs.count(f)) {
        auto* sd = struct_defs.at(f);
        for (const auto& iface : sd->interfaces) {
            if (iface == t) return true;
        }
    }

    // One of them is not a numeric type — allow only if same
    // Um deles nao e numerico — permite apenas se igual
    if (!is_signed_int(f) && !is_unsigned_int(f) && !is_float_type(f)) return false;
    if (!is_signed_int(t) && !is_unsigned_int(t) && !is_float_type(t)) return false;

    int f_rank = type_rank(f);
    int t_rank = type_rank(t);

    bool f_signed = is_signed_int(f);
    bool f_unsigned = is_unsigned_int(f);
    bool f_float = is_float_type(f);
    bool t_signed = is_signed_int(t);
    bool t_unsigned = is_unsigned_int(t);
    bool t_float = is_float_type(t);

    // Float → Int narrowing: always error
    // Float → Int estreitamento: sempre erro
    if (f_float && (t_signed || t_unsigned)) return false;

    // Int → Float widening: allow
    // Int → Float alargamento: permite
    if ((f_signed || f_unsigned) && t_float) return f_rank <= t_rank;

    // Float → Float: widening only
    if (f_float && t_float) return f_rank <= t_rank;

    // Signed ↔ Unsigned same rank: error
    // Signed ↔ Unsigned mesmo rank: erro
    if (f_signed != t_signed && f_rank == t_rank) return false;

    // Both int: widening only
    // Ambos int: apenas alargamento
    return f_rank <= t_rank;
}

std::string TypeChecker::promote_types(const std::string& a, const std::string& b) {
    std::string ra = a;
    std::string rb = b;
    {
        auto it = type_aliases.find(ra);
        if (it != type_aliases.end()) ra = it->second;
    }
    {
        auto it = type_aliases.find(rb);
        if (it != type_aliases.end()) rb = it->second;
    }
    std::string t1 = normalize_type(ra);
    std::string t2 = normalize_type(rb);

    if (t1 == t2) return ra;

    bool f1 = is_float_type(t1);
    bool f2 = is_float_type(t2);

    // Exception: int + float → float (promote integer to float)
    // Excecao: int + float → float (promove inteiro para float)
    if (f1 && !f2) return t1;
    if (!f1 && f2) return t2;

    int r1 = type_rank(t1);
    int r2 = type_rank(t2);

    // Both float → larger
    // Ambos float → maior
    if (f1 && f2) return r1 >= r2 ? a : b;

    bool s1 = is_signed_int(t1);
    bool s2 = is_signed_int(t2);

    // Same sign → larger rank
    // Mesmo sinal → maior rank
    if (s1 == s2) return r1 >= r2 ? a : b;

    // Mixed signed/unsigned
    // Misto signed/unsigned
    std::string signed_t  = s1 ? t1 : t2;
    std::string unsigned_t = s1 ? t2 : t1;
    int r_s = s1 ? r1 : r2;
    int r_u = s1 ? r2 : r1;

    if (r_s >= r_u) return signed_t;
    return signed_type_for_rank(r_u + 1);
}

void TypeChecker::add_error(const std::string& msg) {
    errors.push_back(msg);
}

std::vector<std::string> TypeChecker::check(
    const std::vector<std::unique_ptr<ProgramNode>>& asts)
{
    errors.clear();

    // First pass: collect all struct and interface definitions
    // Primeira passada: coleta todas as definicoes de struct e interface
    for (const auto& ast : asts) {
        for (const auto& decl : ast->declarations) {
            switch (decl->type) {
                case ASTNodeType::PACKAGE_DECL: {
                    auto* pd = static_cast<PackageDecl*>(decl.get());
                    for (size_t i = 0; i < pd->name_parts.size(); i++) {
                        if (i > 0) current_package += ".";
                        current_package += pd->name_parts[i];
                    }
                    break;
                }
                case ASTNodeType::STRUCT_DECL:
                    struct_defs[static_cast<StructDecl*>(decl.get())->name]
                        = static_cast<StructDecl*>(decl.get());
                    break;
                case ASTNodeType::UNION_DECL: {
                    auto* ud = static_cast<UnionDecl*>(decl.get());
                    if (!ud->is_anonymous)
                        union_defs[ud->name] = ud;
                    break;
                }
                case ASTNodeType::INTERFACE_DECL:
                    interface_defs[static_cast<InterfaceDecl*>(decl.get())->name]
                        = static_cast<InterfaceDecl*>(decl.get());
                    break;
                case ASTNodeType::TYPE_ALIAS:
                    type_aliases[static_cast<TypeAliasDecl*>(decl.get())->alias_name]
                        = static_cast<TypeAliasDecl*>(decl.get())->underlying_type;
                    break;
                case ASTNodeType::ENUM_DECL:
                    enum_defs[static_cast<EnumDecl*>(decl.get())->name]
                        = static_cast<EnumDecl*>(decl.get());
                    break;
                default:
                    break;
            }
        }
    }

    // Pre-pass: process impl declarations (add methods to structs)
    // Pre-passagem: processa declaracoes impl (adiciona metodos a structs)
    for (const auto& ast : asts) {
        for (const auto& decl : ast->declarations) {
            if (decl->type == ASTNodeType::IMPL_DECL) {
                auto* id = static_cast<ImplDecl*>(decl.get());
                // Validate struct exists
                if (!struct_defs.count(id->struct_name)) {
                    add_error(id->location.file + ":" + std::to_string(id->location.line) +
                              ": impl for unknown struct '" + id->struct_name + "'");
                    continue;
                }
                // Validate interface exists (if specified)
                if (!id->interface_name.empty() &&
                    !interface_defs.count(id->interface_name)) {
                    add_error(id->location.file + ":" + std::to_string(id->location.line) +
                              ": impl references unknown interface '" +
                              id->interface_name + "'");
                    continue;
                }
                auto* sd = struct_defs[id->struct_name];
                // Add interface to struct's interface list
                if (!id->interface_name.empty()) {
                    bool already = false;
                    for (const auto& iface : sd->interfaces) {
                        if (iface == id->interface_name) { already = true; break; }
                    }
                    if (!already) sd->interfaces.push_back(id->interface_name);
                }
                // Add methods from impl block to struct
                for (auto& m : id->methods) {
                    sd->methods.push_back(std::move(m));
                }
                id->methods.clear(); // prevent double-free
            }
        }
    }

    // Second pass: check all declarations
    // Segunda passada: verifica todas as declaracoes
    for (const auto& ast : asts) {
        for (const auto& decl : ast->declarations) {
            switch (decl->type) {
                case ASTNodeType::STRUCT_DECL:
                    check_struct(static_cast<StructDecl*>(decl.get()));
                    break;
                case ASTNodeType::UNION_DECL:
                    check_union(static_cast<UnionDecl*>(decl.get()));
                    break;
                case ASTNodeType::INTERFACE_DECL:
                    check_interface(static_cast<InterfaceDecl*>(decl.get()));
                    break;
                case ASTNodeType::TYPE_ALIAS:
                    // Already collected in first pass, just validate underlying type
                    {
                        auto* ta = static_cast<TypeAliasDecl*>(decl.get());
                        if (!is_type_known(ta->underlying_type)) {
                            add_error(ta->location.file + ":" + std::to_string(ta->location.line) +
                                      ": unknown type '" + ta->underlying_type + "' in type alias '" +
                                      ta->alias_name + "'");
                        }
                    }
                    break;
                case ASTNodeType::FUNC_DECL: {
                    auto* fd = static_cast<FuncDecl*>(decl.get());
                    if (fd->is_extern) {
                        extern_func_defs[fd->name] = fd;
                    }
                    // Build function pointer type for the function name
                    // Constroi tipo de ponteiro de funcao para o nome da funcao
                    std::string fnptr_type = "fn(";
                    for (size_t i = 0; i < fd->params.size(); i++) {
                        auto* pd = static_cast<ParamDecl*>(fd->params[i].get());
                        if (i > 0) fnptr_type += ",";
                        fnptr_type += pd->type_name;
                    }
                    fnptr_type += ")->" + (fd->return_type.empty() ? "void" : fd->return_type);
                    declare(fd->name, fnptr_type, false);
                    check_function(fd, "");
                    break;
                }
                case ASTNodeType::USING_DECL: {
                    auto* ud = static_cast<UsingDecl*>(decl.get());
                    if (ud->package_parts.size() == 1 && ud->package_parts[0] == "IO") {
                        using_io = true;
                    }
                    break;
                }
                case ASTNodeType::BLOCK_DECL: {
                    auto* bd = static_cast<BlockDecl*>(decl.get());
                    if (lookup(bd->name)) {
                        add_error(decl->location.file + ":" +
                                  std::to_string(decl->location.line) +
                                  ": duplicate block name '" + bd->name + "'");
                    }
                    declare(bd->name, "block", false);
                    break;
                }
                case ASTNodeType::ENUM_DECL:
                    check_enum(static_cast<EnumDecl*>(decl.get()));
                    break;
                case ASTNodeType::CONST_DECL: {
                    auto* cd = static_cast<ConstDecl*>(decl.get());
                    std::string val_type = "int";
                    if (cd->value) {
                        val_type = check_expression(cd->value.get());
                    }
                    if (!cd->type_name.empty()) {
                        if (!can_assign(val_type, cd->type_name)) {
                            add_error(cd->location.file + ":" + std::to_string(cd->location.line) +
                                      ": cannot assign '" + val_type + "' to '" + cd->type_name + "'");
                        }
                        declare(cd->name, cd->type_name, false);
                    } else {
                        declare(cd->name, val_type, false);
                    }
                    break;
                }
                case ASTNodeType::IMPL_DECL:
                case ASTNodeType::MACRO_DECL:
                case ASTNodeType::BUILD_BLOCK:
                case ASTNodeType::EMIT_STMT:
                case ASTNodeType::MACRO_CALL:
                case ASTNodeType::INTERPOLATE:
                case ASTNodeType::VALUE_PLACEHOLDER:
                    // Resolved before type checking — skip
                    // Resolvidos antes da verificacao de tipo — pula
                    break;
                default:
                    break;
            }
        }
    }

    return errors;
}

void TypeChecker::check_struct(StructDecl* sd) {
    current_struct = sd->name;

    // Check extends
    // Verifica extends
    if (!sd->extends.empty()) {
        if (!struct_defs.count(sd->extends)) {
            add_error(sd->location.file + ":" + std::to_string(sd->location.line) +
                      ": struct '" + sd->name + "' extends unknown struct '" +
                      sd->extends + "'");
        }
        // TODO: check cyclic inheritance
        // TODO: verificar heranca ciclica
    }

    // Check interfaces
    // Verifica interfaces
    for (const auto& iface : sd->interfaces) {
        if (!interface_defs.count(iface)) {
            add_error(sd->location.file + ":" + std::to_string(sd->location.line) +
                      ": struct '" + sd->name + "' implements unknown interface '" +
                      iface + "'");
        }
    }

    // Check fields (including inherited)
    // Verifica campos (incluindo herdados)
    push_scope();
    declare_inherited_fields(sd);
    for (const auto& field : sd->fields) {
        if (field->type == ASTNodeType::FIELD_DECL) {
            auto* fd = static_cast<FieldDecl*>(field.get());
            // Resolve type alias
            std::string resolved_type = fd->type_name;
            auto alias_it = type_aliases.find(resolved_type);
            if (alias_it != type_aliases.end()) resolved_type = alias_it->second;
            // Auto-set bit_width for bitfield types (u4, i3, u24, etc.)
            if (is_bitfield_type(resolved_type)) {
                fd->bit_width = parse_bitfield_width(resolved_type);
            }
            if (!is_type_known(resolved_type)) {
                add_error(sd->location.file + ":" + std::to_string(fd->location.line) +
                          ": unknown type '" + fd->type_name + "' in field '" +
                          fd->name + "'");
            }
            // Validate bitfield
            if (fd->bit_width > 0) {
                std::string n = normalize_type(resolved_type);
                bool is_int_type = is_signed_int(n) || is_unsigned_int(n);
                if (!is_int_type && n != "bool") {
                    add_error(sd->location.file + ":" + std::to_string(fd->location.line) +
                              ": bitfield requires integer type, got '" + fd->type_name + "'");
                }
                int max_bits = 0;
                if (n == "bool") max_bits = 1;
                else if (is_bitfield_type(n)) max_bits = parse_bitfield_width(n);
                else if (n == "u8" || n == "i8") max_bits = 8;
                else if (n == "u16" || n == "i16") max_bits = 16;
                else if (n == "u32" || n == "i32" || n == "int") max_bits = 32;
                else if (n == "u64" || n == "i64" || n == "long") max_bits = 64;
                if (fd->bit_width > max_bits) {
                    add_error(sd->location.file + ":" + std::to_string(fd->location.line) +
                              ": bitfield width " + std::to_string(fd->bit_width) +
                              " exceeds type '" + fd->type_name + "' capacity (" +
                              std::to_string(max_bits) + " bits)");
                }
            }
            declare(fd->name, resolved_type, fd->is_private);
        }
        // Anonymous union fields become struct fields directly
        if (field->type == ASTNodeType::UNION_DECL) {
            auto* ud = static_cast<UnionDecl*>(field.get());
            for (const auto& uf : ud->fields) {
                if (uf->type == ASTNodeType::FIELD_DECL) {
                    auto* fd = static_cast<FieldDecl*>(uf.get());
                    std::string resolved_type = fd->type_name;
                    auto alias_it = type_aliases.find(resolved_type);
                    if (alias_it != type_aliases.end()) resolved_type = alias_it->second;
                    if (is_bitfield_type(resolved_type)) {
                        fd->bit_width = parse_bitfield_width(resolved_type);
                    }
                    if (!is_type_known(resolved_type)) {
                        add_error(ud->location.file + ":" +
                                  std::to_string(fd->location.line) +
                                  ": unknown type '" + fd->type_name +
                                  "' in anonymous union field '" + fd->name + "'");
                    }
                    declare(fd->name, resolved_type, fd->is_private);
                }
                // Anonymous struct inside union is flattened too
                if (uf->type == ASTNodeType::STRUCT_DECL) {
                    auto* inner_sd = static_cast<StructDecl*>(uf.get());
                    for (const auto& inner_f : inner_sd->fields) {
                        if (inner_f->type == ASTNodeType::FIELD_DECL) {
                            auto* inner_fd = static_cast<FieldDecl*>(inner_f.get());
                            std::string resolved_type = inner_fd->type_name;
                            auto alias_it = type_aliases.find(resolved_type);
                            if (alias_it != type_aliases.end()) resolved_type = alias_it->second;
                            if (is_bitfield_type(resolved_type)) {
                                inner_fd->bit_width = parse_bitfield_width(resolved_type);
                            }
                            if (!is_type_known(resolved_type)) {
                                add_error(ud->location.file + ":" +
                                          std::to_string(inner_fd->location.line) +
                                          ": unknown type '" + inner_fd->type_name +
                                          "' in anonymous struct field '" + inner_fd->name + "'");
                            }
                            declare(inner_fd->name, resolved_type, inner_fd->is_private);
                        }
                    }
                }
            }
        }
    }

    // Check methods
    // Verifica metodos
    for (const auto& method : sd->methods) {
        if (method->type == ASTNodeType::FUNC_DECL) {
            check_function(static_cast<FuncDecl*>(method.get()), sd->name);
        }
    }

    pop_scope();
    current_struct = "";
}

void TypeChecker::check_union(UnionDecl* ud) {
    if (ud->is_anonymous) return;

    push_scope();
    for (const auto& field : ud->fields) {
        if (field->type == ASTNodeType::FIELD_DECL) {
            auto* fd = static_cast<FieldDecl*>(field.get());
            std::string resolved_type = fd->type_name;
            auto alias_it = type_aliases.find(resolved_type);
            if (alias_it != type_aliases.end()) resolved_type = alias_it->second;
            if (is_bitfield_type(resolved_type)) {
                fd->bit_width = parse_bitfield_width(resolved_type);
            }
            if (lookup(fd->name)) {
                add_error(ud->location.file + ":" + std::to_string(fd->location.line) +
                          ": duplicate field '" + fd->name + "' in union '" +
                          ud->name + "'");
            }
            if (!is_type_known(resolved_type)) {
                add_error(ud->location.file + ":" + std::to_string(fd->location.line) +
                          ": unknown type '" + fd->type_name + "' in field '" +
                          fd->name + "'");
            }
            declare(fd->name, resolved_type, fd->is_private);
        }
        // Anonymous struct inside named union (e.g., union Tag { u32 raw; struct { u24 addr; u8 size; } })
        if (field->type == ASTNodeType::STRUCT_DECL) {
            auto* inner_sd = static_cast<StructDecl*>(field.get());
            for (const auto& inner_f : inner_sd->fields) {
                if (inner_f->type == ASTNodeType::FIELD_DECL) {
                    auto* inner_fd = static_cast<FieldDecl*>(inner_f.get());
                    std::string resolved_type = inner_fd->type_name;
                    auto alias_it = type_aliases.find(resolved_type);
                    if (alias_it != type_aliases.end()) resolved_type = alias_it->second;
                    if (is_bitfield_type(resolved_type)) {
                        inner_fd->bit_width = parse_bitfield_width(resolved_type);
                    }
                    if (!is_type_known(resolved_type)) {
                        add_error(ud->location.file + ":" +
                                  std::to_string(inner_fd->location.line) +
                                  ": unknown type '" + inner_fd->type_name +
                                  "' in anonymous struct field '" + inner_fd->name + "'");
                    }
                    declare(inner_fd->name, resolved_type, inner_fd->is_private);
                }
            }
        }
    }
    pop_scope();
}

void TypeChecker::check_interface(InterfaceDecl* id) {
    // Interfaces only declare method signatures — just validate no conflicts
    // Interfaces so declaram assinaturas de metodo — so valida sem conflitos
    push_scope();
    for (const auto& method : id->methods) {
        if (method->type == ASTNodeType::FUNC_DECL) {
            auto* fd = static_cast<FuncDecl*>(method.get());
            if (lookup(fd->name)) {
                add_error(id->location.file + ":" + std::to_string(fd->location.line) +
                          ": duplicate method '" + fd->name + "' in interface '" +
                          id->name + "'");
            }
            declare(fd->name, "fn", false);
        }
    }
    pop_scope();
}

void TypeChecker::check_enum(EnumDecl* ed) {
    declare(ed->name, "int", false);
    for (auto& variant : ed->variants) {
        declare(variant.name, "int", false);
    }
}

void TypeChecker::declare_inherited_fields(StructDecl* sd) {
    if (sd->extends.empty()) return;
    if (!struct_defs.count(sd->extends)) return;
    auto* base = struct_defs[sd->extends];
    // First recurse to declare grandparent fields
    // Primeiro recorre para declarar campos de avos
    declare_inherited_fields(base);
    for (const auto& f : base->fields) {
        if (f->type == ASTNodeType::FIELD_DECL) {
            auto* fd = static_cast<FieldDecl*>(f.get());
            if (!lookup(fd->name)) {
                declare(fd->name, fd->type_name, fd->is_private);
            }
        }
    }
}

void TypeChecker::check_function(FuncDecl* fd, const std::string& struct_name) {
    // Check constructor naming
    // Verifica nomenclatura do construtor
    if (!struct_name.empty()) {
        fd->is_constructor = (fd->name == struct_name);
    }

    // Check return type
    // Verifica tipo de retorno
    if (!fd->return_type.empty() && !is_type_known(fd->return_type)) {
        add_error(fd->location.file + ":" + std::to_string(fd->location.line) +
                  ": unknown return type '" + fd->return_type + "' in function '" +
                  fd->name + "'");
    }

    push_scope();

    // Declare 'this' for methods
    // Declara "this" para metodos
    if (!struct_name.empty()) {
        declare("this", struct_name + "*", false, true);
    }

    // Declare parameters
    // Declara parametros
    for (const auto& param : fd->params) {
        if (param->type == ASTNodeType::PARAM_DECL) {
            auto* pd = static_cast<ParamDecl*>(param.get());
            if (!is_type_known(pd->type_name)) {
                add_error(fd->location.file + ":" + std::to_string(pd->location.line) +
                          ": unknown parameter type '" + pd->type_name + "' in function '" +
                          fd->name + "'");
            }
            if (lookup(pd->name)) {
                add_error(fd->location.file + ":" + std::to_string(pd->location.line) +
                          ": duplicate parameter '" + pd->name + "' in function '" +
                          fd->name + "'");
            }
            declare(pd->name, pd->type_name, false, true);
        }
    }

    // Check body
    // Verifica corpo
    if (fd->body && fd->body->type == ASTNodeType::BLOCK_STMT) {
        check_block(static_cast<BlockStmt*>(fd->body.get()), fd->return_type);
    }

    pop_scope();
}

void TypeChecker::check_block(BlockStmt* block, const std::string& return_type) {
    push_scope();
    for (const auto& stmt : block->statements) {
        check_statement(stmt.get(), return_type);
    }
    pop_scope();
}

void TypeChecker::check_statement(ASTNode* stmt, const std::string& return_type) {
    switch (stmt->type) {
        case ASTNodeType::BLOCK_STMT:
            check_block(static_cast<BlockStmt*>(stmt), return_type);
            break;

        case ASTNodeType::IF_STMT: {
            auto* is = static_cast<IfStmt*>(stmt);
            std::string cond_type = check_expression(is->condition.get());
            if (!can_assign(cond_type, "bool") && !is_integer_type(cond_type)) {
                add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                          ": if condition must be bool or integer, got '" + cond_type + "'");
            }
            check_statement(is->then_branch.get(), return_type);
            if (is->else_branch) {
                check_statement(is->else_branch.get(), return_type);
            }
            break;
        }

        case ASTNodeType::WHILE_STMT: {
            auto* ws = static_cast<WhileStmt*>(stmt);
            std::string cond_type = check_expression(ws->condition.get());
            if (!can_assign(cond_type, "bool") && !is_integer_type(cond_type)) {
                add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                          ": while condition must be bool or integer, got '" + cond_type + "'");
            }
            check_statement(ws->body.get(), return_type);
            break;
        }

        case ASTNodeType::FOR_STMT: {
            auto* fs = static_cast<ForStmt*>(stmt);
            if (fs->init) check_statement(fs->init.get(), return_type);
            if (fs->condition) {
                std::string cond_type = check_expression(fs->condition.get());
                if (!can_assign(cond_type, "bool") && !is_integer_type(cond_type)) {
                    add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                              ": for condition must be bool or integer, got '" + cond_type + "'");
                }
            }
            if (fs->increment) check_expression(fs->increment.get());
            check_statement(fs->body.get(), return_type);
            break;
        }

        case ASTNodeType::RETURN_STMT: {
            auto* rs = static_cast<ReturnStmt*>(stmt);
            if (rs->value) {
                std::string val_type = check_expression(rs->value.get());
                if (return_type.empty() || return_type == "void") {
                    add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                              ": cannot return value from void function");
                } else if (!can_assign(val_type, return_type)) {
                    add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                              ": cannot return '" + val_type + "' from function returning '" +
                              return_type + "'");
                }
            } else {
                if (!return_type.empty() && return_type != "void") {
                    add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                              ": must return a value of type '" + return_type + "'");
                }
            }
            break;
        }

        case ASTNodeType::EXPR_STMT: {
            auto* es = static_cast<ExprStmt*>(stmt);
            // Check if this is a variable declaration: ExprStmt(Assignment(IdentExpr, value))
            // Verifica se e declaracao de variavel: ExprStmt(Assignment(IdentExpr, value))
            // where the target is not yet declared
            // onde o alvo ainda nao foi declarado
                if (es->expr->type == ASTNodeType::ASSIGNMENT) {
                    auto* assign = static_cast<Assignment*>(es->expr.get());
                    if (assign->target->type == ASTNodeType::IDENT_EXPR) {
                        auto* ident = static_cast<IdentExpr*>(assign->target.get());
                        if (!lookup(ident->name)) {
                            // This is a variable declaration - infer type from value
                            // Isto e uma declaracao de variavel — infere tipo do valor
                            std::string val_type = check_expression(assign->value.get());
                            // Check if unsuffixed literal fits in declared type
                            // Verifica se literal sem sufixo cabe no tipo declarado
                            if (!ident->declared_type.empty()) {
                                ident->declared_type = resolve_array_sizes(ident->declared_type);
                                bool is_unsuffixed_literal = false;
                                if (assign->value->type == ASTNodeType::INT_LITERAL) {
                                    auto* il = static_cast<IntLiteral*>(assign->value.get());
                                    if (il->literal_type.empty()) {
                                        is_unsuffixed_literal = true;
                                        if (!int_fits_in_type(il->value, ident->declared_type, il->is_hex)) {
                                            add_error(assign->location.file + ":" +
                                                       std::to_string(assign->location.line) +
                                                       ": value " + std::to_string(il->value) +
                                                       " does not fit in type '" + ident->declared_type + "'");
                                        }
                                    }
                                }
                                if (assign->value->type == ASTNodeType::FLOAT_LITERAL) {
                                    auto* fl = static_cast<FloatLiteral*>(assign->value.get());
                                    if (fl->literal_type.empty()) {
                                        is_unsuffixed_literal = true;
                                    }
                                }
                                // Array literal initialization: {val1, val2, ...}
                                if (assign->value->type == ASTNodeType::ARRAY_LITERAL) {
                                    auto* al = static_cast<ArrayLiteral*>(assign->value.get());
                                    auto bracket = ident->declared_type.find('[');
                                    if (bracket != std::string::npos) {
                                        std::string target_elem = ident->declared_type.substr(0, bracket);
                                        for (auto& el : al->elements) {
                                            // Check if unsuffixed literal fits in target element type
                                            if (el->type == ASTNodeType::INT_LITERAL) {
                                                auto* iel = static_cast<IntLiteral*>(el.get());
                                                if (iel->literal_type.empty() &&
                                                    iel->resolved_type != target_elem && 
                                                    !int_fits_in_type(iel->value, target_elem, iel->is_hex)) {
                                                    add_error(assign->location.file + ":" +
                                                              std::to_string(assign->location.line) +
                                                              ": value " + std::to_string(iel->value) +
                                                              " does not fit in array element type '" +
                                                              target_elem + "'");
                                                }
                                                el->resolved_type = target_elem;
                                            } else if (el->type == ASTNodeType::FLOAT_LITERAL) {
                                                el->resolved_type = target_elem;
                                            } else if (!can_assign(el->resolved_type, target_elem)) {
                                                add_error(assign->location.file + ":" +
                                                          std::to_string(assign->location.line) +
                                                          ": cannot assign '" + el->resolved_type +
                                                          "' to array element type '" + target_elem + "'");
                                            } else {
                                                el->resolved_type = target_elem;
                                            }
                                        }
                                    } else if (is_struct_type(ident->declared_type)) {
                                        auto* sd = struct_defs[ident->declared_type];
                                        // Check if it's named init (elements are Assignments)
                                        bool is_named = (!al->elements.empty() &&
                                                         al->elements[0]->type == ASTNodeType::ASSIGNMENT);
                                        if (is_named) {
                                            for (auto& el : al->elements) {
                                                if (el->type != ASTNodeType::ASSIGNMENT) continue;
                                                auto* ass = static_cast<Assignment*>(el.get());
                                                if (ass->target->type != ASTNodeType::IDENT_EXPR) continue;
                                                std::string fname = static_cast<IdentExpr*>(ass->target.get())->name;
                                                auto* fd = find_struct_field(sd, fname);
                                                if (!fd) {
                                                    add_error(assign->location.file + ":" +
                                                              std::to_string(assign->location.line) +
                                                              ": struct '" + ident->declared_type +
                                                              "' has no field '" + fname + "'");
                                                    continue;
                                                }
                                                std::string val_type = ass->value->resolved_type;
                                                if (val_type.empty()) val_type = check_expression(ass->value.get());
                                                if (!can_assign(val_type, fd->type_name)) {
                                                    add_error(assign->location.file + ":" +
                                                              std::to_string(assign->location.line) +
                                                              ": cannot assign '" + val_type +
                                                              "' to field '" + fname + "'");
                                                }
                                            }
                                        } else {
                                            // Positional init: match by index
                                            size_t n = std::min(al->elements.size(), sd->fields.size());
                                            for (size_t ei = 0; ei < n; ei++) {
                                                auto* fd = static_cast<FieldDecl*>(sd->fields[ei].get());
                                                auto& el = al->elements[ei];
                                                if (el->type == ASTNodeType::INT_LITERAL) {
                                                    auto* iel = static_cast<IntLiteral*>(el.get());
                                                    if (iel->literal_type.empty() &&
                                                        iel->resolved_type != fd->type_name &&
                                                        !int_fits_in_type(iel->value, fd->type_name, iel->is_hex)) {
                                                        add_error(assign->location.file + ":" +
                                                                  std::to_string(assign->location.line) +
                                                                  ": value " + std::to_string(iel->value) +
                                                                  " does not fit in struct field '" +
                                                                  fd->name + "' of type '" + fd->type_name + "'");
                                                    }
                                                    el->resolved_type = fd->type_name;
                                                } else if (el->type == ASTNodeType::FLOAT_LITERAL) {
                                                    el->resolved_type = fd->type_name;
                                                } else if (!can_assign(el->resolved_type, fd->type_name)) {
                                                    add_error(assign->location.file + ":" +
                                                              std::to_string(assign->location.line) +
                                                              ": cannot assign '" + el->resolved_type +
                                                              "' to struct field '" + fd->name +
                                                              "' of type '" + fd->type_name + "'");
                                                } else {
                                                    el->resolved_type = fd->type_name;
                                                }
                                            }
                                        }
                                    } else {
                                        add_error(assign->location.file + ":" +
                                                  std::to_string(assign->location.line) +
                                                  ": cannot assign array literal to non-array type '" +
                                                  ident->declared_type + "'");
                                    }
                                } else if (!is_unsuffixed_literal && !can_assign(val_type, ident->declared_type)) {
                                    add_error(assign->location.file + ":" +
                                              std::to_string(assign->location.line) +
                                              ": cannot assign '" + val_type + "' to '" + ident->declared_type + "'");
                                }
                                val_type = ident->declared_type;
                            }
                            declare(ident->name, val_type, false);
                            assign->target->resolved_type = val_type;
                            return;
                        }
                    }
                }
            // Variable declaration without initializer: Type name
            // Declaracao de variavel sem inicializador: Tipo nome
            if (es->expr->type == ASTNodeType::IDENT_EXPR) {
                auto* ident = static_cast<IdentExpr*>(es->expr.get());
                if (!ident->declared_type.empty() && !lookup(ident->name)) {
                    ident->declared_type = resolve_array_sizes(ident->declared_type);
                    if (!is_type_known(ident->declared_type)) {
                        add_error(es->location.file + ":" +
                                  std::to_string(es->location.line) +
                                  ": unknown type '" + ident->declared_type + "'");
                    }
                    declare(ident->name, ident->declared_type, false);
                    ident->resolved_type = ident->declared_type;
                    return;
                }
            }
            check_expression(es->expr.get());
            break;
        }

        case ASTNodeType::BLOCK_DECL: {
            // Block declarations are statements too (block game = 64MB)
            // Declaracoes de bloco tambem sao statements (block game = 64MB)
            auto* bd = static_cast<BlockDecl*>(stmt);
            if (lookup(bd->name)) {
                add_error(stmt->location.file + ":" + std::to_string(stmt->location.line) +
                          ": duplicate block name '" + bd->name + "'");
            }
            declare(bd->name, "block", false);
            break;
        }

        case ASTNodeType::BLOCK_SCOPE: {
            auto* bs = static_cast<BlockScope*>(stmt);
            push_scope();
            for (const auto& s : bs->body) {
                check_statement(s.get(), return_type);
            }
            pop_scope();
            break;
        }

        case ASTNodeType::BREAK_STMT:
        case ASTNodeType::CONTINUE_STMT:
            break;

        case ASTNodeType::DEFER_STMT: {
            auto* ds = static_cast<DeferStmt*>(stmt);
            check_statement(ds->body.get(), return_type);
            break;
        }

        case ASTNodeType::MATCH_STMT: {
            auto* ms = static_cast<MatchStmt*>(stmt);
            std::string val_type = check_expression(ms->value.get());
            for (auto& arm : ms->arms) {
                for (auto& pat : arm.patterns) {
                    // Wildcard pattern: "_" — skip type checking
                    if (pat->type == ASTNodeType::IDENT_EXPR) {
                        auto* ident = static_cast<IdentExpr*>(pat.get());
                        if (ident->name == "_") { pat->resolved_type = val_type; continue; }
                    }
                    std::string pat_type = check_expression(pat.get());
                    if (pat_type == "unknown") continue;
                    if (!can_assign(val_type, pat_type) && !can_assign(pat_type, val_type)) {
                        add_error(pat->location.file + ":" +
                                  std::to_string(pat->location.line) +
                                  ": match pattern type '" + pat_type +
                                  "' does not match value type '" + val_type + "'");
                    }
                }
                if (arm.guard) {
                    std::string guard_type = check_expression(arm.guard.get());
                    if (!can_assign(guard_type, "bool")) {
                        add_error(arm.guard->location.file + ":" +
                                  std::to_string(arm.guard->location.line) +
                                  ": match guard must be bool, got '" + guard_type + "'");
                    }
                }
                if (arm.body) {
                    check_statement(arm.body.get(), return_type);
                }
            }
            break;
        }

        case ASTNodeType::CONST_DECL: {
            auto* cd = static_cast<ConstDecl*>(stmt);
            std::string val_type = "int";
            if (cd->value) {
                val_type = check_expression(cd->value.get());
                // Store const int value for array size resolution
                if (val_type == "int" && cd->value->type == ASTNodeType::INT_LITERAL) {
                    auto* il = static_cast<IntLiteral*>(cd->value.get());
                    const_values_[cd->name] = il->value;
                }
            }
            if (!cd->type_name.empty()) {
                if (!can_assign(val_type, cd->type_name)) {
                    add_error(cd->location.file + ":" + std::to_string(cd->location.line) +
                              ": cannot assign '" + val_type + "' to '" + cd->type_name + "'");
                }
                declare(cd->name, cd->type_name, false);
            } else {
                declare(cd->name, val_type, false);
            }
            break;
        }

        default: {
            std::string t = check_expression(stmt);
            break;
        }
    }
}

std::string TypeChecker::check_expression(ASTNode* expr) {
    if (!expr) return "void";

    switch (expr->type) {
        case ASTNodeType::INT_LITERAL: {
            auto* il = static_cast<IntLiteral*>(expr);
            if (!il->literal_type.empty()) {
                if (!int_fits_in_type(il->value, il->literal_type, il->is_hex)) {
                    add_error(expr->location.file + ":" +
                              std::to_string(expr->location.line) +
                              ": value " + std::to_string(il->value) +
                              " does not fit in type '" + il->literal_type + "'");
                }
                expr->resolved_type = il->literal_type;
                return il->literal_type;
            }
            expr->resolved_type = "int";
            return "int";
        }

        case ASTNodeType::FLOAT_LITERAL: {
            auto* fl = static_cast<FloatLiteral*>(expr);
            if (!fl->literal_type.empty()) {
                if (!float_fits_in_type(fl->value, fl->literal_type)) {
                    add_error(expr->location.file + ":" +
                              std::to_string(expr->location.line) +
                              ": value does not fit in type '" + fl->literal_type + "'");
                }
                expr->resolved_type = fl->literal_type;
                return fl->literal_type;
            }
            expr->resolved_type = "float";
            return "float";
        }

        case ASTNodeType::STRING_LITERAL:
            expr->resolved_type = "String";
            return "String";

        case ASTNodeType::BOOL_LITERAL:
            expr->resolved_type = "bool";
            return "bool";

        case ASTNodeType::CHAR_LITERAL:
            expr->resolved_type = "char";
            return "char";

        case ASTNodeType::NULL_LITERAL:
            expr->resolved_type = "null";
            return "null";

        case ASTNodeType::IDENT_EXPR: {
            auto* ident = static_cast<IdentExpr*>(expr);
            auto* sym = lookup(ident->name);
            if (!sym) {
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": undefined symbol '" + ident->name + "'");
                expr->resolved_type = "unknown";
                return "unknown";
            }
            expr->resolved_type = sym->type;
            return sym->type;
        }

        case ASTNodeType::BINARY_OP: {
            auto* bin = static_cast<BinaryOp*>(expr);
            std::string left_type = check_expression(bin->left.get());
            std::string right_type = check_expression(bin->right.get());

            // Bitwise AND: requires integer types, returns promoted type
            // AND binario: exige tipos inteiros, retorna tipo promovido
            if (bin->op == TokenType::BIT_AND) {
                bool left_int = is_signed_int(left_type) || is_unsigned_int(left_type);
                bool right_int = is_signed_int(right_type) || is_unsigned_int(right_type);
                if (!left_int || !right_int) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": bitwise AND requires integer types, got '" +
                              left_type + "' and '" + right_type + "'");
                }
                std::string result = promote_types(left_type, right_type);
                expr->resolved_type = result;
                return result;
            }

            // Arithmetic operators (including pointer arithmetic)
            // Operadores aritmeticos (incluindo aritmetica de ponteiros)
            if (bin->op == TokenType::PLUS || bin->op == TokenType::MINUS ||
                bin->op == TokenType::STAR || bin->op == TokenType::SLASH) {
                bool left_ptr = !left_type.empty() && left_type[0] == '*';
                bool right_ptr = !right_type.empty() && right_type[0] == '*';

                // Pointer arithmetic: *T ± int → *T
                if (left_ptr && !right_ptr && (bin->op == TokenType::PLUS || bin->op == TokenType::MINUS)) {
                    if (!can_assign(right_type, "int")) {
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": pointer arithmetic requires integer offset, got '" + right_type + "'");
                    }
                    expr->resolved_type = left_type;
                    return left_type;
                }
                // int + *T → *T (commutative for +)
                if (right_ptr && !left_ptr && bin->op == TokenType::PLUS) {
                    if (!can_assign(left_type, "int")) {
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": pointer arithmetic requires integer offset, got '" + left_type + "'");
                    }
                    expr->resolved_type = right_type;
                    return right_type;
                }
                // Pointer difference: *T - *T → isize
                if (left_ptr && right_ptr && bin->op == TokenType::MINUS) {
                    if (left_type != right_type) {
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": cannot subtract pointers of different types '" +
                                  left_type + "' and '" + right_type + "'");
                    }
                    expr->resolved_type = "isize";
                    return "isize";
                }

                if (!can_assign(left_type, "int") && !can_assign(left_type, "float") &&
                    !can_assign(left_type, "i32") && !can_assign(left_type, "f64")) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": arithmetic op requires numeric types, got '" +
                              left_type + "' and '" + right_type + "'");
                }
                std::string result = promote_types(left_type, right_type);
                expr->resolved_type = result;
                return result;
            }

            // Comparison operators (including pointer comparisons)
            // Operadores de comparacao (incluindo comparacao de ponteiros)
            if (bin->op == TokenType::EQ || bin->op == TokenType::NEQ ||
                bin->op == TokenType::LT || bin->op == TokenType::GT ||
                bin->op == TokenType::LEQ || bin->op == TokenType::GEQ) {
                // Pointer comparison: same pointer type or with null
                bool left_ptr = !left_type.empty() && left_type[0] == '*';
                bool right_ptr = !right_type.empty() && right_type[0] == '*';
                if (left_ptr && right_ptr) {
                    if (left_type != right_type) {
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": cannot compare different pointer types '" +
                                  left_type + "' and '" + right_type + "'");
                    }
                    expr->resolved_type = "bool";
                    return "bool";
                }
                if ((left_ptr && right_type == "null") || (right_ptr && left_type == "null")) {
                    expr->resolved_type = "bool";
                    return "bool";
                }
                if (!can_assign(left_type, right_type) && !can_assign(right_type, left_type)) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": cannot compare '" + left_type + "' and '" + right_type + "'");
                }
                expr->resolved_type = "bool";
                return "bool";
            }

            // Logical operators
            // Operadores logicos
            if (bin->op == TokenType::AND || bin->op == TokenType::OR) {
                if (!can_assign(left_type, "bool") && !is_integer_type(left_type)) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": logical op requires bool or integer, got '" + left_type + "'");
                }
                expr->resolved_type = "bool";
                return "bool";
            }

            expr->resolved_type = left_type;
            return left_type;
        }

        case ASTNodeType::UNARY_OP: {
            auto* un = static_cast<UnaryOp*>(expr);
            std::string op_type = check_expression(un->operand.get());
            if (un->op == TokenType::NOT) {
                if (!can_assign(op_type, "bool") && !is_integer_type(op_type)) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": not requires bool or integer, got '" + op_type + "'");
                }
                expr->resolved_type = "bool";
                return "bool";
            }
            // Dereference: *ptr → base type of *T
            if (un->op == TokenType::STAR) {
                if (op_type.empty() || op_type[0] != '*') {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": cannot dereference non-pointer type '" + op_type + "'");
                    expr->resolved_type = "unknown";
                    return "unknown";
                }
                std::string base = op_type.substr(1);
                expr->resolved_type = base;
                return base;
            }
            // Address-of: &var → *T (special case: &fn → fn type directly)
            if (un->op == TokenType::BIT_AND) {
                if (op_type.rfind("fn(", 0) == 0) {
                    // Function names are already function pointers; &fn = fn
                    expr->resolved_type = op_type;
                    return op_type;
                }
                std::string ptr_type = "*" + op_type;
                expr->resolved_type = ptr_type;
                return ptr_type;
            }
            expr->resolved_type = op_type;
            return op_type;
        }

        case ASTNodeType::CALL_EXPR: {
            auto* call = static_cast<CallExpr*>(expr);

            // Resolve arguments
            // Resolve argumentos
            for (const auto& arg : call->arguments) {
                check_expression(arg.get());
            }

            // If callee is an IdentExpr, look up the function
            // Se callee e um IdentExpr, busca a funcao
            if (call->callee->type == ASTNodeType::IDENT_EXPR) {
                auto* ident = static_cast<IdentExpr*>(call->callee.get());

                // Check for built-in print() function
                // Verifica funcao print() embutida
                if (ident->name == "print") {
                    if (!using_io) {
                        add_error(expr->location.file + ":" +
                                  std::to_string(expr->location.line) +
                                  ": use 'using IO;' before calling print()");
                    }
                    for (const auto& arg : call->arguments) {
                        std::string t = check_expression(arg.get());
                        std::string tn = normalize_type(t);
                        bool is_valid_print_type =
                            tn == "i8" || tn == "i16" || tn == "i32" || tn == "i64" ||
                            tn == "u8" || tn == "u16" || tn == "u32" || tn == "u64" ||
                            tn == "f32" || tn == "f64" ||
                            tn == "bool" ||
                            tn == "String" ||
                            tn == "isize" || tn == "usize";
                        if (!is_valid_print_type) {
                            add_error(expr->location.file + ":" +
                                      std::to_string(expr->location.line) +
                                      ": unsupported type '" + t + "' for print()");
                        }
                    }
                    expr->resolved_type = "void";
                    return "void";
                }

                // Check for type cast: T(expr) where T is a type keyword
                // Verifica cast de tipo: T(expr) onde T e uma palavra-chave de tipo
                if (call->arguments.size() == 1 && is_type_keyword(ident->name)) {
                    std::string val_type = check_expression(call->arguments[0].get());
                    std::string target = ident->name;
                    // Allow narrowing with explicit cast: check only if types are compatible
                    bool num_from = is_signed_int(val_type) || is_unsigned_int(val_type) || is_float_type(val_type);
                    bool num_to = is_signed_int(target) || is_unsigned_int(target) || is_float_type(target);
                    if (num_from && num_to) {
                        // Numeric cast is always allowed (including narrowing)
                        expr->resolved_type = target;
                        return target;
                    }
                    if (!can_assign(val_type, target) && !can_assign(target, val_type)) {
                        add_error(expr->location.file + ":" +
                                  std::to_string(expr->location.line) +
                                  ": cannot cast '" + val_type + "' to '" + target + "'");
                    }
                    expr->resolved_type = target;
                    return target;
                }

                // Check for constructor call: struct name matches
                // Verifica chamada de construtor: nome da struct corresponde
                if (is_struct_type(ident->name)) {
                    check_constructor_call(ident->name, call);
                    expr->resolved_type = ident->name;
                    return ident->name;
                }

                // Check for built-in error() function
                // Verifica funcao error() embutida
                if (ident->name == "error") {
                    expr->resolved_type = "void";
                    return "void";
                }

                // Extern function call
                // Chamada de funcao externa
                {
                    auto ext = extern_func_defs.find(ident->name);
                    if (ext != extern_func_defs.end()) {
                        std::string ret = ext->second->return_type;
                        expr->resolved_type = ret.empty() ? "void" : ret;
                        return expr->resolved_type;
                    }
                }

                // Function pointer call through variable
                // Chamada via ponteiro de funcao
                if (call->callee->type == ASTNodeType::IDENT_EXPR) {
                    auto* fn_ident = static_cast<IdentExpr*>(call->callee.get());
                    auto* sym = lookup(fn_ident->name);
                    if (sym && sym->type.rfind("fn(", 0) == 0) {
                        // Extract return type from fn(params)->ret
                        std::string fn_type = sym->type;
                        auto arrow = fn_type.rfind(")->");
                        if (arrow != std::string::npos) {
                            std::string ret_type = fn_type.substr(arrow + 3);
                            expr->resolved_type = ret_type.empty() ? "void" : ret_type;
                            return expr->resolved_type;
                        }
                    }
                }

                // Regular function call (default fallback)
                expr->resolved_type = "int";
                return "int";
            }

            // Method call: obj.method()
            // Chamada de metodo: obj.method()
            if (call->callee->type == ASTNodeType::MEMBER_EXPR) {
                auto* mem = static_cast<MemberExpr*>(call->callee.get());
                std::string obj_type = check_expression(mem->object.get());

                // Handle block.reset() built-in
                // Trata block.reset() embutido
                if (obj_type == "block") {
                    if (mem->member == "reset") {
                        expr->resolved_type = "void";
                        return "void";
                    }
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": block has no method '" + mem->member + "'");
                    expr->resolved_type = "void";
                    return "void";
                }

                // Strip pointer
                // Remove ponteiro
                std::string base_type = obj_type;
                if (!base_type.empty() && base_type.back() == '*') {
                    base_type.pop_back();
                }

                if (is_struct_type(base_type)) {
                    // Check if method exists
                    // Verifica se metodo existe
                    auto* sd = struct_defs[base_type];
                    bool found = false;
                    for (const auto& method : sd->methods) {
                        if (method->type == ASTNodeType::FUNC_DECL) {
                            auto* fd = static_cast<FuncDecl*>(method.get());
                            if (fd->name == mem->member) {
                                expr->resolved_type = fd->return_type.empty() ? "void" : fd->return_type;
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found) {
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": struct '" + base_type + "' has no method '" +
                                  mem->member + "'");
                    }
                } else if (is_interface_type(base_type)) {
                    // Interface method call
                    // Chamada de metodo de interface
                    auto* id = interface_defs[base_type];
                    bool found = false;
                    for (const auto& method : id->methods) {
                        if (method->type == ASTNodeType::FUNC_DECL) {
                            auto* fd = static_cast<FuncDecl*>(method.get());
                            if (fd->name == mem->member) {
                                expr->resolved_type = fd->return_type.empty() ? "void" : fd->return_type;
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found) {
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": interface '" + base_type + "' has no method '" +
                                  mem->member + "'");
                    }
                } else if (is_dynamic_array_type(base_type)) {
                    // Dynamic array T[] built-in methods
                    // Metodos embutidos de array dinamico T[]
                    if (mem->member == "append") {
                        if (call->arguments.size() != 1) {
                            add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                      ": append() requires exactly 1 argument");
                        } else {
                            auto bracket = base_type.find('[');
                            std::string elem_type = base_type.substr(0, bracket);
                            std::string arg_type = check_expression(call->arguments[0].get());
                            if (!can_assign(arg_type, elem_type)) {
                                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                          ": cannot append '" + arg_type + "' to array of '" +
                                          elem_type + "'");
                            }
                        }
                        expr->resolved_type = "void";
                    } else {
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": array type '" + base_type + "' has no method '" +
                                  mem->member + "'");
                        expr->resolved_type = "void";
                    }
                } else {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": cannot call method on non-struct type '" + obj_type + "'");
                }
                return expr->resolved_type;
            }

            expr->resolved_type = "int";
            return "int";
        }

        case ASTNodeType::MEMBER_EXPR: {
            auto* mem = static_cast<MemberExpr*>(expr);

            // Handle .sizeof and .alignof built-in properties
            if (mem->member == "sizeof" || mem->member == "alignof") {
                if (mem->object->type == ASTNodeType::IDENT_EXPR) {
                    auto* ident = static_cast<IdentExpr*>(mem->object.get());
                    if (is_type_keyword(ident->name) ||
                        struct_defs.count(ident->name) ||
                        interface_defs.count(ident->name) ||
                        type_aliases.count(ident->name)) {
                        expr->resolved_type = "i64";
                        return "i64";
                    }
                    auto* sym = lookup(ident->name);
                    if (sym) {
                        ident->resolved_type = sym->type;
                        expr->resolved_type = "i64";
                        return "i64";
                    }
                }
                // expression.sizeof — resolve object and get its type size
                {
                    std::string t = check_expression(mem->object.get());
                    if (t != "unknown" && !t.empty()) {
                        expr->resolved_type = "i64";
                        return "i64";
                    }
                }
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": cannot get sizeof of expression");
                expr->resolved_type = "unknown";
                return "unknown";
            }

            std::string obj_type = check_expression(mem->object.get());

            std::string base_type = obj_type;
            if (!base_type.empty() && base_type.back() == '*') {
                base_type.pop_back();
            }

            // Dynamic array T[] built-in properties: .len -> i64, .cap -> i64
            if (is_dynamic_array_type(base_type)) {
                if (mem->member == "len" || mem->member == "cap") {
                    expr->resolved_type = "i64";
                    return "i64";
                }
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": array type '" + base_type + "' has no member '" +
                          mem->member + "'");
                expr->resolved_type = "unknown";
                return "unknown";
            }

            // Interface type: check method access
            if (is_interface_type(base_type)) {
                auto* id = interface_defs[base_type];
                for (const auto& method : id->methods) {
                    if (method->type == ASTNodeType::FUNC_DECL) {
                        auto* fd = static_cast<FuncDecl*>(method.get());
                        if (fd->name == mem->member) {
                            expr->resolved_type = "fn";
                            return "fn";
                        }
                    }
                }
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": interface '" + base_type + "' has no member '" +
                          mem->member + "'");
                expr->resolved_type = "unknown";
                return "unknown";
            }

            if (is_struct_type(base_type) || is_union_type(base_type)) {
                // Check struct/union fields, including anonymous union/struct members
                // Verifica campos de struct/union, incluindo membros anonimos
                auto check_fields = [&](const auto& fields) -> bool {
                    for (const auto& field : fields) {
                        if (field->type == ASTNodeType::FIELD_DECL) {
                            auto* fd = static_cast<FieldDecl*>(field.get());
                            if (fd->name == mem->member) {
                                expr->resolved_type = fd->type_name;
                                return true;
                            }
                        }
                        // Anonymous union: fields are directly accessible
                        if (field->type == ASTNodeType::UNION_DECL) {
                            auto* ud = static_cast<UnionDecl*>(field.get());
                            for (const auto& uf : ud->fields) {
                                if (uf->type == ASTNodeType::FIELD_DECL) {
                                    auto* fd = static_cast<FieldDecl*>(uf.get());
                                    if (fd->name == mem->member) {
                                        expr->resolved_type = fd->type_name;
                                        return true;
                                    }
                                }
                                // Anonymous struct inside union
                                if (uf->type == ASTNodeType::STRUCT_DECL) {
                                    auto* inner_sd = static_cast<StructDecl*>(uf.get());
                                    for (const auto& inner_f : inner_sd->fields) {
                                        if (inner_f->type == ASTNodeType::FIELD_DECL) {
                                            auto* inner_fd = static_cast<FieldDecl*>(inner_f.get());
                                            if (inner_fd->name == mem->member) {
                                                expr->resolved_type = inner_fd->type_name;
                                                return true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        // Anonymous struct: fields are directly accessible
                        if (field->type == ASTNodeType::STRUCT_DECL) {
                            auto* inner_sd = static_cast<StructDecl*>(field.get());
                            for (const auto& inner_f : inner_sd->fields) {
                                if (inner_f->type == ASTNodeType::FIELD_DECL) {
                                    auto* inner_fd = static_cast<FieldDecl*>(inner_f.get());
                                    if (inner_fd->name == mem->member) {
                                        expr->resolved_type = inner_fd->type_name;
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                    return false;
                };

                if (is_struct_type(base_type)) {
                    auto* sd = struct_defs[base_type];
                    if (check_fields(sd->fields)) return expr->resolved_type;

                    // Check methods
                    // Verifica metodos
                    for (const auto& method : sd->methods) {
                        if (method->type == ASTNodeType::FUNC_DECL) {
                            auto* func = static_cast<FuncDecl*>(method.get());
                            if (func->name == mem->member) {
                                expr->resolved_type = "fn";
                                return "fn";
                            }
                        }
                    }

                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": struct '" + base_type + "' has no member '" +
                              mem->member + "'");
                } else {
                    auto it = union_defs.find(base_type);
                    if (it == union_defs.end()) {
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": unknown union type '" + base_type + "'");
                    } else {
                        if (check_fields(it->second->fields)) return expr->resolved_type;
                        add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                                  ": union '" + base_type + "' has no member '" +
                                  mem->member + "'");
                    }
                }
            } else {
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": cannot access member of non-struct type '" + obj_type + "'");
            }

            expr->resolved_type = "unknown";
            return "unknown";
        }

        case ASTNodeType::INDEX_EXPR: {
            auto* ie = static_cast<IndexExpr*>(expr);
            std::string arr_type = check_expression(ie->array.get());
            std::string idx_type = check_expression(ie->index.get());
            if (!can_assign(idx_type, "int")) {
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": array index must be int, got '" + idx_type + "'");
            }

            // Extract element type from array type "int[10]" -> "int"
            // Extrai tipo do elemento do tipo array "int[10]" -> "int"
            auto bracket = arr_type.find('[');
            if (bracket != std::string::npos) {
                std::string elem_type = arr_type.substr(0, bracket);
                expr->resolved_type = elem_type;
                return elem_type;
            }

            // Pointer indexing: *T -> T (e.g., *int p; p[0] -> int)
            // Indexacao de ponteiro: *T -> T (ex: *int p; p[0] -> int)
            if (!arr_type.empty() && arr_type[0] == '*') {
                std::string elem_type = arr_type.substr(1);
                expr->resolved_type = elem_type;
                return elem_type;
            }

            expr->resolved_type = arr_type;
            return arr_type;
        }

        case ASTNodeType::CAST_EXPR: {
            auto* ce = static_cast<CastExpr*>(expr);
            std::string val_type = check_expression(ce->expr.get());
            std::string target = ce->target_type;
            // Allow all numeric casts (including narrowing) with explicit cast
            bool num_from = is_signed_int(val_type) || is_unsigned_int(val_type) || is_float_type(val_type);
            bool num_to = is_signed_int(target) || is_unsigned_int(target) || is_float_type(target);
            if (!(num_from && num_to)) {
                add_error(expr->location.file + ":" +
                          std::to_string(expr->location.line) +
                          ": cannot cast '" + val_type + "' to '" + target + "'");
            }
            expr->resolved_type = target;
            return target;
        }

        case ASTNodeType::ASSIGNMENT: {
            auto* assign = static_cast<Assignment*>(expr);
            std::string target_type = check_expression(assign->target.get());
            std::string value_type = check_expression(assign->value.get());

            // Pointer compound assignment: *T += int / *T -= int (pointer arithmetic)
            // Atribuicao composta com ponteiro: *T += int / *T -= int (aritmetica de ponteiro)
            bool target_ptr = !target_type.empty() && target_type[0] == '*';
            if (target_ptr && (assign->op == TokenType::PLUS_ASSIGN ||
                               assign->op == TokenType::MINUS_ASSIGN)) {
                if (!can_assign(value_type, "int")) {
                    add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                              ": pointer arithmetic requires integer offset, got '" +
                              value_type + "'");
                }
                expr->resolved_type = target_type;
                return target_type;
            }

            // Array literal initialization: {val1, val2, ...}
            if (assign->value->type == ASTNodeType::ARRAY_LITERAL) {
                auto* al = static_cast<ArrayLiteral*>(assign->value.get());
                auto bracket = target_type.find('[');
                if (bracket == std::string::npos) {
                    if (is_struct_type(target_type)) {
                        auto* sd = struct_defs[target_type];
                        // Check if it's named init (elements are Assignments)
                        bool is_named = (!al->elements.empty() &&
                                         al->elements[0]->type == ASTNodeType::ASSIGNMENT);
                        if (is_named) {
                            // Named init: match by field name
                            // Init nomeado: casa por nome do campo
                            for (auto& el : al->elements) {
                                if (el->type != ASTNodeType::ASSIGNMENT) {
                                    add_error(expr->location.file + ":" +
                                              std::to_string(expr->location.line) +
                                              ": named init requires all elements to be 'field = value'");
                                    continue;
                                }
                                auto* ass = static_cast<Assignment*>(el.get());
                                if (ass->target->type != ASTNodeType::IDENT_EXPR) {
                                    add_error(expr->location.file + ":" +
                                              std::to_string(expr->location.line) +
                                              ": expected field name in named init");
                                    continue;
                                }
                                std::string fname = static_cast<IdentExpr*>(ass->target.get())->name;
                                auto* fd = find_struct_field(sd, fname);
                                if (!fd) {
                                    add_error(expr->location.file + ":" +
                                              std::to_string(expr->location.line) +
                                              ": struct '" + target_type + "' has no field '" +
                                              fname + "'");
                                    continue;
                                }
                                std::string val_type = ass->value->resolved_type;
                                if (val_type.empty()) val_type = check_expression(ass->value.get());
                                if (!can_assign(val_type, fd->type_name)) {
                                    add_error(expr->location.file + ":" +
                                              std::to_string(expr->location.line) +
                                              ": cannot assign '" + val_type +
                                              "' to field '" + fname + "' of type '" +
                                              fd->type_name + "'");
                                }
                            }
                        } else {
                            // Positional init: match by index
                            size_t n = std::min(al->elements.size(), sd->fields.size());
                            for (size_t ei = 0; ei < n; ei++) {
                                auto* fd = static_cast<FieldDecl*>(sd->fields[ei].get());
                                auto& el = al->elements[ei];
                                if (el->type == ASTNodeType::INT_LITERAL) {
                                    auto* iel = static_cast<IntLiteral*>(el.get());
                                    if (iel->literal_type.empty() &&
                                        iel->resolved_type != fd->type_name &&
                                        !int_fits_in_type(iel->value, fd->type_name, iel->is_hex)) {
                                        add_error(expr->location.file + ":" +
                                                  std::to_string(expr->location.line) +
                                                  ": value " + std::to_string(iel->value) +
                                                  " does not fit in struct field '" +
                                                  fd->name + "' of type '" + fd->type_name + "'");
                                    }
                                    el->resolved_type = fd->type_name;
                                } else if (el->type == ASTNodeType::FLOAT_LITERAL) {
                                    el->resolved_type = fd->type_name;
                                } else if (!can_assign(el->resolved_type, fd->type_name)) {
                                    add_error(expr->location.file + ":" +
                                              std::to_string(expr->location.line) +
                                              ": cannot assign '" + el->resolved_type +
                                              "' to struct field '" + fd->name +
                                              "' of type '" + fd->type_name + "'");
                                } else {
                                    el->resolved_type = fd->type_name;
                                }
                            }
                        }
                    } else {
                        add_error(expr->location.file + ":" +
                                  std::to_string(expr->location.line) +
                                  ": cannot assign array literal to non-array type '" +
                                  target_type + "'");
                    }
                } else {
                    std::string target_elem = target_type.substr(0, bracket);
                    for (auto& el : al->elements) {
                        if (el->type == ASTNodeType::INT_LITERAL) {
                            auto* iel = static_cast<IntLiteral*>(el.get());
                            if (iel->literal_type.empty() &&
                                iel->resolved_type != target_elem &&
                                !int_fits_in_type(iel->value, target_elem, iel->is_hex)) {
                                add_error(expr->location.file + ":" +
                                          std::to_string(expr->location.line) +
                                          ": value " + std::to_string(iel->value) +
                                          " does not fit in array element type '" +
                                          target_elem + "'");
                            }
                            el->resolved_type = target_elem;
                        } else if (el->type == ASTNodeType::FLOAT_LITERAL) {
                            el->resolved_type = target_elem;
                        } else if (!can_assign(el->resolved_type, target_elem)) {
                            add_error(expr->location.file + ":" +
                                      std::to_string(expr->location.line) +
                                      ": cannot assign '" + el->resolved_type +
                                      "' to array element type '" + target_elem + "'");
                        } else {
                            el->resolved_type = target_elem;
                        }
                    }
                }
                expr->resolved_type = target_type;
                return target_type;
            }

            // Check if unsuffixed literal fits in target type
            // Verifica se literal sem sufixo cabe no tipo destino
            if (assign->value->type == ASTNodeType::INT_LITERAL) {
                auto* il = static_cast<IntLiteral*>(assign->value.get());
                if (il->literal_type.empty() && target_type != value_type) {
                    if (!int_fits_in_type(il->value, target_type, il->is_hex)) {
                        add_error(expr->location.file + ":" +
                                  std::to_string(expr->location.line) +
                                  ": value " + std::to_string(il->value) +
                                  " does not fit in type '" + target_type + "'");
                    }
                    // Infer the literal's type from the target
                    // Infere o tipo do literal a partir do destino
                    assign->value->resolved_type = target_type;
                    value_type = target_type;
                }
            }

            if (!can_assign(value_type, target_type)) {
                add_error(expr->location.file + ":" + std::to_string(expr->location.line) +
                          ": cannot assign '" + value_type + "' to '" + target_type + "'");
            }
            expr->resolved_type = target_type;
            return target_type;
        }

        case ASTNodeType::ALLOC_INLINE: {
            auto* ai = static_cast<AllocInline*>(expr);
            std::string inner_type = check_expression(ai->expr.get());
            expr->resolved_type = inner_type;
            return inner_type;
        }

        case ASTNodeType::ARRAY_LITERAL: {
            auto* al = static_cast<ArrayLiteral*>(expr);
            std::string common;
            for (auto& el : al->elements) {
                if (el->type == ASTNodeType::ASSIGNMENT) {
                    // Named struct init: field = value — only check the value part
                    auto* ass = static_cast<Assignment*>(el.get());
                    std::string t = check_expression(ass->value.get());
                    if (common.empty()) common = t;
                } else {
                    std::string t = check_expression(el.get());
                    if (common.empty()) {
                        common = t;
                    } else if (t != common) {
                        add_error(expr->location.file + ":" +
                                  std::to_string(expr->location.line) +
                                  ": all elements of array literal must have the same type, got '" +
                                  common + "' and '" + t + "'");
                    }
                }
            }
            if (common.empty()) common = "int"; // Empty array -> int[] default
            expr->resolved_type = common + "[]";
            return expr->resolved_type;
        }

        case ASTNodeType::RESET_EXPR: {
            expr->resolved_type = "void";
            return "void";
        }

        default:
            expr->resolved_type = "unknown";
            return "unknown";
    }
}

void TypeChecker::check_constructor_call(const std::string& struct_name, CallExpr* call) {
    auto* sd = struct_defs[struct_name];
    // Find matching constructor
    // Encontra construtor correspondente
    bool found = false;
    for (const auto& method : sd->methods) {
        if (method->type == ASTNodeType::FUNC_DECL) {
            auto* fd = static_cast<FuncDecl*>(method.get());
            if (fd->is_constructor) {
                // Check param count
                // Verifica contagem de parametros
                if (fd->params.size() == call->arguments.size()) {
                    found = true;
                    for (size_t i = 0; i < fd->params.size(); i++) {
                        auto* pd = static_cast<ParamDecl*>(fd->params[i].get());
                        if (!can_assign(call->arguments[i]->resolved_type, pd->type_name)) {
                            add_error(call->location.file + ":" +
                                      std::to_string(call->location.line) +
                                      ": constructor argument " + std::to_string(i) +
                                      " type mismatch: expected '" + pd->type_name +
                                      "', got '" + call->arguments[i]->resolved_type + "'");
                        }
                    }
                }
            }
        }
    }
    if (!found && !sd->methods.empty()) {
        // If struct has no constructor, that's ok — can still be allocated
        // Se struct nao tem construtor, tudo bem — ainda pode ser alocada
    }
}

} // namespace brick
  // namespace brick
