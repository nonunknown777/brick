# Architecture

This page describes the Brick project architecture in detail — for contributors who want to understand how everything fits together.

---

## Overview

Brick is a programming language that compiles to C. The pipeline is:

```
.brc file  →  [Lexer] → tokens  →  [Parser] → AST  →  [Macro System] → clean AST  →  [Codegen] → .c file
                                                           (collect + build + expand)                     ↓
                                                                                                    [gcc -O3]
                                                                                                       ↓
                                                                                                    binary
```

The runtime (C library) is linked at the gcc step. The visualizer, debugger, and VS Code extension are auxiliary tools that operate on the running program.

---

## Compiler Pipeline

### 1. Lexer (`src/lexer/`)

**File**: `lexer.cpp`, `lexer.h`

The lexer converts a `.brc` source string into a sequence of **tokens**.

**Input**: Raw source code string
**Output**: `std::vector<Token>`

```cpp
// Each token has:
struct Token {
    TokenType type;       // what kind of token
    std::string lexeme;   // the actual text
    SourceLocation location;  // line, col, file
};
```

**Token types** (from `shared/types.h`):

```
Keywords:    package, using, public, private, struct, extends,
             interface, fn, return, if, else, while, for,
             block, reset, true, false, null, error,
             macro, build, emit
Types:       int, float, bool, char, String, void
Macro:       $, ... (ELLIPSIS)
Literals:    int literal, float literal, string literal, char literal
Operators:   +, -, *, /, =, ==, !=, <, >, <=, >=,
             &&, ||, !, +=, -=, *=, /=,
             &, |, ^, ~, <<, >>, ++, --
Delimiters:  {, }, (, ), [, ], ;, ,, ., @, ->
```

**Implementation details:**
- Character-by-character scanner
- Keyword lookup via `std::unordered_map` (O(1) amortized)
- Handles escape sequences in strings: `\n`, `\t`, `\\`, `\"`, `\'`
- Single-line comments only (`//` — skips to end of line)
- Tracks `SourceLocation` (line + column + filename) for error reporting
- All literal values are stored as strings in the token (parsed later if needed)

### 2. Parser (`src/parser/`)

**Files**: `parser.cpp`, `parser.h`, `ast.h`, `package.cpp`, `package.h`

The parser converts tokens into an **Abstract Syntax Tree (AST)** and resolves package relationships.

**Input**: `std::vector<Token>`
**Output**: `ParseResult` containing `ProgramNode` (AST root)

```cpp
struct ParseResult {
    std::unique_ptr<ProgramNode> ast;
    std::vector<std::string> errors;
};
```

**Parser type**: Recursive descent (hand-written, no parser generator)

**AST structure** (from `ast.h`):

```
ProgramNode
├── PackageDecl        → "package SPRITES.EFFECTS"
├── UsingDecl          → "using SPRITES"
├── BlockDecl          → "block global = 256MB"
├── MacroDecl          → "macro swap(a, b) { ... }"              ← MACRO
├── BuildBlock         → "build { x = 42; emit { ... } }"       ← MACRO
├── StructDecl         → "struct Player { ... }"
│   ├── FieldDecl      → "int hp"
│   └── FuncDecl       → "fn take_damage(int d) { ... }" (methods)
│       ├── ParamDecl  → "int d"
│       └── BlockStmt  → { body }
├── InterfaceDecl      → "interface Damageable { ... }"
├── FuncDecl           → "fn main() { ... }" (top-level functions)
│   ├── ParamDecl
│   └── BlockStmt
├── MacroCall          → "swap(x, y)"                            ← MACRO
└── EmitStmt           → "emit { fn $name() { } }"               ← MACRO
    └── BlockStmt → { body with $ interpolation }

Statements:
├── IfStmt, WhileStmt, ForStmt, ReturnStmt
├── ExprStmt (variable declarations, assignments)
├── BlockScope ("block name: { ... }")
└── Expressions:
    ├── IntLiteral, FloatLiteral, StringLiteral
    ├── BoolLiteral, CharLiteral, NullLiteral
    ├── IdentExpr, CallExpr, MemberExpr, IndexExpr
    ├── BinaryOp, UnaryOp, Assignment
    ├── AllocInline, ResetExpr
    ├── Interpolate                                 ← MACRO ($name)
    └── ValuePlaceholder                            ← MACRO ($(expr))
```

**Package Resolution** (`package.cpp`):

- Each file declares a package via `package NOME`
- `PackageTable` maps package names to `PackageInfo` (containing exported structs and functions)
- `resolve_packages()` processes the AST and populates the table
- Tracks which symbols are `private` vs `public`
- `merge_package_tables()` combines tables from multiple files
- `is_accessible()` checks visibility rules

**Macro handling** — The parser recognizes `macro`, `build`, and `emit` keywords and builds the corresponding AST nodes. See [Macro System](#35-macro-system-srcparser) below.

### 3.5. Macro System (`src/parser/`)

**Files**: `macro_expander.cpp`, `macro_expander.h`, `build_eval.cpp`, `build_eval.h`

The macro system sits between the parser and codegen. It runs before type checking, so macros operate on raw AST (no type info yet).

**Pipeline step:**
```
Parser AST  →  [Collect Macros]  →  [Eval Build]  →  [Expand Macros]  →  clean AST  →  Type Checker + Codegen
```

**Collect Macros:**
- Walks the AST for all `MACRO_DECL` nodes
- Stores them in a `MacroTable` mapping name → `MacroDecl*`
- Removes `macro` declarations from the AST (they don't generate C code)

**Eval Build (`build_eval.cpp`):**
- Interprets `BUILD_BLOCK` nodes at compile time
- Supports arithmetic, strings, arrays, loops, conditionals
- Executes `EMIT_STMT` calls to produce generated code
- Runtime I/O is forbidden inside `build` blocks
- Provides type reflection: `T.name`, `T.size`, `T.fields`

**Expand Macros (`macro_expander.cpp`):**
- Deep-clones the macro body AST for each call site
- Substitutes `$param` with the actual argument expressions
- Renames `__`-prefixed identifiers with unique gensyms (e.g., `__tmp` → `__tmp__1`)
- Handles varargs (`...`) by wrapping remaining arguments in a list
- Nested expansion: expands macros inside macro bodies recursively
- Recursion guard: aborts after 64 levels to catch infinite recursion

**Result:** A clean AST with no macro nodes — just plain Brick code ready for type checking and codegen.

### 4. Type Checker (`src/codegen/type_checker.cpp`)

**Files**: `type_checker.cpp`, `type_checker.h`

Runs **after** macro expansion on the clean AST.

**Files**: `type_checker.cpp`, `type_checker.h`

Runs before code generation to validate types and catch errors early.

**What it checks:**
- Struct field types exist (built-in or user-defined)
- Function return types exist
- Constructor parameters match argument types
- `if`/`while`/`for` conditions are boolean
- `return` values match declared return type
- Variables are declared before use
- No duplicate names (fields, parameters, functions)
- `print()` requires `using IO`
- Assignment type compatibility (`int` ↔ `float` implicit, `null` assignable to anything)

**Symbol tables** are scoped:
```cpp
std::vector<std::unordered_map<std::string, SymbolInfo>> scopes;
```

When entering a block or function, a new scope is pushed. When leaving, it's popped. Name lookup walks scopes from innermost to outermost.

### 4. Codegen (`src/codegen/`)

**Files**: `codegen.cpp`, `codegen.h`

The codegen walks the AST and emits C code.

**Input**: ASTs + PackageTable
**Output**: `CodegenResult` containing C source code string

```cpp
struct CodegenResult {
    std::string c_code;        // the generated C source
    std::vector<std::string> errors;
    bool success;
};
```

**What the codegen produces:**

1. **Headers** — includes for `<stdint.h>`, `<string.h>`, `block_memory.h`, `io.h`
2. **Struct definitions** — each Brick struct becomes a `typedef struct` in C
   - Inheritance flattens into a `base` field (parent struct as first member)
3. **Block context variables** — `BlockCtx* global; BlockCtx* game;`
4. **`__brick_init()`** — initializes blocks at program start
5. **Functions + methods** — each function becomes a C function
   - Methods: `StructName_method_name(StructName* this, args...)`
   - Constructors: `StructName_StructName(StructName* this, args...)`
   - `main()` calls `__brick_init()` and returns `int`
6. **`#line` directives** — embedded in the C code for source mapping

**Naming conventions:**
- Struct methods: `Player_take_damage(Player* this, int dmg)`
- Constructors: `Player_Player(Player* this, int hp, String name)`
- Top-level functions: `function_name(args)`

**Memory management in generated code:**
- Variable declarations: if block-allocated (`@` annotation), emits `block_alloc()` call
- Constructor calls with `@`: emits `block_alloc()` + constructor call
- Block scope: emits save/restore of `_current_block`
- `block.reset()`: emits `block_reset(block_name)`
- `block name: { }` scope: emits `_current_block` save/restore

**IO generation:**
- `print(x)`: maps to `io_print_int()`, `io_print_float()`, etc. based on type
- `print("fmt {0}", x)`: maps to `io_printf()` with type-appropriate format specifiers
- `error("msg")`: maps to `fprintf(stderr, msg); exit(1)`

---

## Runtime (`runtime/`)

The C runtime library is linked into every Brick program.

### Block Memory (`block_memory.c` / `block_memory.h`)

The bump allocator:

```c
typedef struct BlockCtx {
    uint8_t*  data;         // block memory base
    size_t    capacity;     // total size
    size_t    used;         // current bump offset
    size_t    peak_used;    // highest usage
    size_t    allocation_count;  // total allocs
} BlockCtx;
```

Key functions:
- `block_create(size_t megabytes)` / `block_create_bytes(size_t bytes)` — allocate block
- `block_alloc(BlockCtx* ctx, size_t size)` — bump allocate (default 8-byte alignment)
- `block_alloc_aligned(BlockCtx* ctx, size_t size, size_t alignment)` — aligned bump
- `block_reset(BlockCtx* ctx)` — O(1), just `ctx->used = 0`
- `block_destroy(BlockCtx* ctx)` — free memory
- `block_stats(BlockCtx* ctx)` — returns usage statistics

**Optional registry** (enabled with `-DBRICK_TRACK_BLOCKS`):
- `block_register(ctx, name)` — register a block for monitoring
- `block_unregister(ctx)` — unregister
- `block_find(name)` — find a block by name
- `block_snapshot(out, max)` — snapshot all registered blocks
- `block_shm_export()` — export to `/tmp/brick-mem-<pid>.bin` for visualizer

### IO (`io.c` / `io.h`)

Print wrappers for the `print()` built-in:

```c
void io_print_int(int64_t val);        // prints "42\n"
void io_print_float(double val);       // prints "3.140000\n"
void io_print_string(const char* data, int64_t len);  // prints "hello\n"
void io_print_char(char val);          // prints "a\n"
void io_print_bool(uint8_t val);       // prints "true\n" or "false\n"
void io_print_newline(void);           // prints "\n"
void io_printf(const char* fmt, ...);  // formatted print
```

### Hot Reload (`hot_reload.c` / `hot_reload.h`)

See [Hot Reload](Hot-Reload) for details.

---

## Visualizer (`visualizer/`)

The TUI memory visualizer uses ncurses to display memory blocks in real time.

**Files**: `memvis.cpp`, `memvis.h`

**Modes:**
1. **Embedded** — `memvis_run(config)`: reads blocks from the runtime's registry (blocks must be registered with `BRICK_TRACK_BLOCKS`)
2. **Attach** — `memvis_attach(pid, config)`: reads from `/tmp/brick-mem-<pid>.bin` shared memory export

**Display:**

```
global  256MB  ████████████░░░░  67%
game     64MB  ██████░░░░░░░░  38%
temp      8MB  ██░░░░░░░░░░░░  15%
```

Each block shows:
- Name
- Capacity
- Usage bar (Unicode block characters)
- Percentage full
- Warning ⚠ at >80%

**CLI Access:**

```bash
build/brick --visualize        # standalone TUI
build/brick --attach <pid>     # attach to running process
```

---

## Debugger (`debugger/`)

The debugger provides GDB integration for Brick programs.

**Files:**
- `.gdbinit` — auto-loaded by VS Code or manually: `source debugger/.gdbinit`
- `gdb_pretty_printers.py` — pretty-printers for `BlockCtx*`, `BrickString`, block-allocated pointers
- `gdb_commands.py` — custom GDB commands

**Pretty-Printers:**
- `BlockCtx*`: Shows name, capacity, used, peak, allocation count, usage bar
- `BrickString`: Shows `"content" (len=N)`
- Block-allocated pointers: Shows `@blockname+offset` with block fullness

**Custom Commands:**
```
(gdb) info blocks           # List all blocks with usage
(gdb) block <name>          # Detailed block info
(gdb) block-watch <name>    # Breakpoint on block_alloc for this block
(gdb) blocks-list           # Names only (for DAP integration)
```

**`#line` Directives:**
The codegen emits `#line` directives in the C code:
```c
#line 42 "game.brc"
```

This tells GDB to show the original `.brc` file and line numbers when debugging, not the generated C.

---

## VS Code Extension (`vscode-ext/`)

**Files**: TypeScript + JSON configuration

Capabilities:
- **Language support**: Syntax highlighting (TextMate grammar), snippets, language configuration
- **LSP client**: Communicates with the Brick compiler in `--lsp` mode for diagnostics
- **Debug adapter**: Launch configurations for debugging compiled `.brc` programs with GDB
- **Memory Webview**: Visual representation of blocks during debug sessions

See [VS Code Extension](VS-Code-Extension) for full details.

---

## Build System (`SConstruct`)

Brick uses **SCons** (Python) as its build system.

**Key features:**
- **Profiles**: `release` (-O3), `debug` (-g -O0), `sanitize` (ASan+UBSan)
- **Cross-compilation**: `target=linux`, `target=windows` (mingw-w64)
- **Compiler detection**: Auto-detects g++ or clang++
- **Feature detection**: Checks for ncurses, X11, pthreads
- **Caching**: `CacheDir('.scons_cache')` for incremental builds
- **Embedded runtime**: Python script `scripts/embed_runtime.py` generates `embedded_runtime.h/.cpp` that embeds all runtime C source files into the compiler binary, enabling `brick build` and `brick run` commands

**Build targets:**
```bash
scons                    # Release build
scons profile=debug      # Debug build
scons profile=sanitize   # Sanitizer build
scons test               # Build + run all tests
scons target=windows     # Cross-compile to Windows
scons install            # Install to prefix
```

---

## Data Flow Summary

```
INPUT: hello.brc
│
├─ Lexer
│  → tokens: [PACKAGE("HELLO"), USING("IO"), BLOCK("global"), ...]
│
├─ Parser
│  → AST: ProgramNode
│     ├─ PackageDecl("HELLO")
│     ├─ UsingDecl("IO")
│     ├─ BlockDecl("global", 64, "MB")
│     ├─ MacroDecl("swap")                    ← MACRO
│     ├─ BuildBlock { ... }                    ← MACRO
│     ├─ StructDecl("Greeter")
│     │   ├─ FieldDecl("String", "message")
│     │   ├─ FuncDecl("Greeter") constructor
│     │   └─ FuncDecl("greet")
│     └─ FuncDecl("main")
│         └─ BlockStmt
│             └─ MacroCall("swap")             ← MACRO
│
├─ [Macro System]
│  → 1. Collect: gathers MacroDecl into a table
│  → 2. Build:  evaluates BuildBlock, runs emit calls
│  → 3. Expand: replaces each MacroCall with cloned/substituted AST
│  → clean AST (no MacroDecl, BuildBlock, MacroCall, or EmitStmt)
│
├─ TypeChecker
│  → Types resolved, errors collected
│  → Each ASTNode.resolved_type filled
│
├─ Codegen
│  → C code with #line directives
│
├─ gcc -O3
│  → binary
│
└─ Runtime (linked in)
   → Block allocator + IO + Hot Reload
```

---

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| **Compile to C, not LLVM IR** | Simpler, human-readable output, leverages existing C optimizers |
| **C++20 for compiler** | Modern C++ features (templates, constexpr, smart pointers) for tooling |
| **C for runtime** | Stable ABI for hot reload, maximum portability |
| **Bump allocator** | Minimal overhead, zero fragmentation, cache-friendly |
| **No exceptions** | Runtime overhead is antithetical to the performance goal |
| **No GC** | Predictable performance, no pause times |
| **Explicit blocks** | Gives the programmer full control over memory lifetime |
| **Recursive descent parser** | Simple, hand-written, debuggable, O(n) |
| **Line directives** | Debug in source language, not the generated code |
| **SCons** | Python-based, cross-platform, simpler than CMake |
