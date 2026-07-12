# Brick Architecture

## Overview

Brick compiles `.brc` source files to C code, then delegates to `gcc` or `clang` for machine code generation. The compiler itself is written in C++20.

```
.bricada → Lexer → Token Stream → Parser → AST → Type Checker → C Codegen → .c file → gcc -O3 → binary
                                ↕
                          Macro Expander
                          Build Eval
```

## Pipeline Stages

### 1. Lexer (`src/lexer/`)

**File**: `lexer.cpp`, `lexer.h`

Tokenizes `.brc` source into a flat token stream. Handles:

- Keywords (`fn`, `struct`, `if`, `while`, `for`, `return`, `match`, `defer`, `const`, etc.)
- Identifiers (package names, function names, variable names)
- Literals:
  - **Integer**: decimal, hex (`0xFF`), binary (`0b1010`), octal (`0o777`), with suffixes (`42u8`, `0x1Au16`)
  - **Float**: with optional suffixes (`3.14f32`, `3.14f64`)
  - **Char**: `'a'`, `'\n'`
  - **String**: `"hello"` with escape sequences
  - **Bool**: `true`, `false`
  - **Null**: `null`
- Operators: `+ - * / % = == != < > <= >= && || ! & | ^ ~ << >> ++ -- += -= *= /= @ . , ; : () [] {} ->`
- Comments: `//` line-only

### 2. Parser (`src/parser/`)

**Files**: `parser.cpp`, `parser.h`, `ast.h`, `package.cpp`, `package.h`, `build_eval.cpp`, `build_eval.h`, `macro_expander.cpp`, `macro_expander.h`

Recursive-descent parser that produces an AST. Architecture:

```
Parser
├── Top-level declarations: structs, enums, unions, functions, consts, macros, interfaces, impl blocks, type aliases
├── Expressions: binary ops, unary ops, literals, identifiers, calls, arrays, pointer deref, sizeof/alignof
├── Statements: if, while, for, for-in, return, break, continue, match, defer, block scopes
├── Package resolution: using, export, private, nested packages (MATH.VEC2)
├── Build system: build {}, emit {} compile-time eval
├── Macro system: macro definition, $interpolation, $macro() explicit call, hygiene, varargs
└── C interop: include, link, extern fn, @system
```

The AST is an untyped tree. Type checking happens separately in the codegen stage.

### 2a. Macro Expander

Before codegen, the macro expander processes all `macro` definitions and `build`/`emit` blocks:

1. Collect all `macro` definitions during parsing
2. On `macro_name(...)` call, expand body with `$param` substitutions
3. `build { ... }` evaluates code at compile time
4. `emit { ... }` generates code from the build context
5. Hygiene: `__`-prefixed variables get unique gensyms
6. Varargs: `args...` captures remaining arguments

### 2b. Build Eval

`build {}` blocks execute at compile time. Variables inside don't exist in the final binary. Uses a simple interpreter:

- Supports arithmetic, variable assignment, conditionals
- `emit {}` inside build generates code
- `T.name`, `T.size`, `T.fields` for type reflection

### 2c. Package Resolution

1. `using PACKAGE` → searches for `PACKAGE.brc`:
   - Current directory
   - `-I <dir>` paths
   - `<dir>/PACKAGE/PACKAGE.brc` (nested)
   - `<dir>/PACKAGE/main.brc`
   - `BRICK_PATH` environment variable paths
2. Nested packages: `MATH.VEC2` → `MATH/VEC2.brc` (dots → directory separators)
3. `export`/`private` visibility enforcement during type checking

### 3. Type Checker & Codegen (`src/codegen/`)

**Files**: `codegen.cpp`, `codegen.h`, `type_checker.cpp`, `type_checker.h`

Two-phase code generation:

#### Phase 1: Type Checking (`type_checker.cpp`)

- Walks the AST and resolves types
- Checks: widening/narrowing rules, overflow, function signatures, struct field access, interface conformance
- Visibility enforcement (private → same-package only)
- Generates error messages with source location

#### Phase 2: C Code Generation (`codegen.cpp`)

Produces readable C code with `#line` directives for debugging.

**Type Map**:

| Brick | C |
|-------|---|
| `i8` | `int8_t` |
| `i16` | `int16_t` |
| `i32` | `int32_t` |
| `i64` | `int64_t` |
| `u8` | `uint8_t` |
| `u16` | `uint16_t` |
| `u32` | `uint32_t` |
| `u64` | `uint64_t` |
| `f32` | `float` |
| `f64` | `double` |
| `bool` | `uint8_t` |
| `usize` | `size_t` |
| `isize` | `ptrdiff_t` |
| `String` | `BrickString` |
| `void` | `void` |
| `T[N]` | `T name[N]` |
| `T[]` | `T* name; int64_t name_cnt; int64_t name_cap` |

**Codegen details**:

- `fn` → `static inline` (always_inline attribute) — zero call overhead
- `export fn` → visible (no `static`)
- `extern fn` → declaration only, no body
- `block` → `BlockCtx*` with bump allocator
- `.reset()` → `block_reset(ctx)`
- `error("msg")` → `fprintf(stderr, "msg"); exit(1)`
- `@block` → `block_alloc(ctx, size)`
- `defer` → `__attribute__((cleanup))` or inline at scope exit
- `sizeof(T)` → `sizeof(T)`
- `alignof(T)` → `_Alignof(T)`
- `match` → `switch` with `if` guards
- `for x in N` → `for (int64_t __i = 0; __i < N; __i++)`
- `const` → `static const` (compiler-substituted)
- `impl` block → vtbl struct + wrapper functions
- `interface` → vtbl struct + `void* data` wrapper
- `String` → `{char* data; int64_t len}` struct
- `#line` directives for every .brc line (GDB-friendly)

### 4. Embedded Runtime

**Files**: `embedded_runtime.cpp`, `embedded_runtime.h`

The runtime (`block_memory.c`, `io.c`, `hot_reload.c`, `pool_allocator.c`) is either:
- **Embedded**: compiled into the brick binary (for `brick run` / `brick build`)
- **External**: linked separately via `runtime/block_memory.c` etc.

`embedded_runtime.cpp` converts runtime C files into C++ string literals at build time.

## Runtime Components

### Block Memory (`runtime/block_memory.c`, `.h`)

- **Bump allocator**: `block_alloc(ctx, size)` — ~3 CPU cycles
- **Aligned allocator**: `block_alloc_aligned(ctx, size, align)`
- **Reset**: `block_reset(ctx)` — O(1), ~5 ns
- **TLS**: `block_set_tls(ctx)` / `block_get_tls()` — thread-local blocks
- **Double buffer**: `block_enable_double_buffer(ctx)` / `block_swap_buffers(ctx)` — zero-pause hot reload

### Hot Reload (`runtime/hot_reload.c`, `.h`)

- **Linux**: `dlopen` + `inotify` — watches `.so` for changes
- **Windows**: `LoadLibrary` + `ReadDirectoryChangesW` — watches `.dll`
- States: `HR_WAITING` → `HR_LOADING` → `HR_OK` / `HR_ERROR`
- Thread-safe: atomic function pointer swap
- Cross-platform macros: `HR_THREAD_CREATE`, `HR_MUTEX_LOCK`, etc.

### I/O (`runtime/io.c`, `.h`)

- `print()` with `{0}` formatting — int, float, string, bool
- `BrickString` type: `{char* data; int64_t len}`
- `brick_string_create()` — allocates from block
- Numeric conversion, escape sequence parsing

### Pool Allocator (`runtime/pool_allocator.c`, `.h`)

- For types ≤ 64 bytes
- `pool_create()`, `pool_destroy()`, `pool_add_slot()`
- `pool_alloc(size)` — O(1) allocation
- `pool_free(ptr)` — O(1) free
- `pool_slot_stats(slot)` — monitoring
- Max 8 slots, configurable

### Memory Visualizer (`visualizer/memvis.cpp`)

- **Linux**: ncurses TUI dashboard
- Shows: capacity, usage, peak, allocation count per block
- `memvis_run()` — start visualizer
- `memvis_attach()` — attach to running process
- Shared memory: `memvis_read_shm()`

## Build System

### SCons (`SConstruct`, `src/SConscript`, `runtime/SConscript`, `tests/SConscript`)

```
SConstruct
├── src/SConscript         → brick compiler binary
├── runtime/SConscript     → runtime C objects
├── tests/SConscript       → test binaries
└── visualizer/SConscript  → TUI visualizer
```

### CLI Usage

```bash
brick <input.brc> [-o output]     # compile to C
brick build <files> [-o output]    # compile to binary
brick run <input.brc>              # compile and run
brick new <project>                # scaffold
brick bind <header.h>              # C bindings
```

## VS Code Extension (`vscode-ext/`)

- **Syntax highlighting**: TextMate grammar (`syntaxes/brick.tmLanguage.json`)
- **LSP server**: TypeScript (`server/src/server.ts`)
- **Extension**: Activates on `.brc` files, provides:
  - Completions
  - Hover info
  - Go-to-definition
  - Signature help
  - Semantic tokens
  - Memory webview (`memoryWebview.ts`)

## Debugger (`debugger/`)

- **`.gdbinit`**: Auto-loads in project directory
- **`gdb_commands.py`**: Custom commands:
  - `info blocks` — show all block contexts
  - `block <name>` — inspect a specific block
- **`gdb_pretty_printers.py`**: Python pretty-printers for `BlockCtx`, `BrickString`

`#line` directives in generated C map source locations back to `.brc` files for breakpoints and stack traces.

## Test Suite

- **Unit tests**: `tests/test_codegen.cpp` (198+ tests), `tests/test_lexer.cpp`, `tests/test_parser.cpp`, `tests/test_macros.cpp`
- **Integration tests**: `tests/test_integration.sh` (30+ tests)
- **Feature tests**: `tests/features/test_*.brc` (16 feature tests)
- **Macro error tests**: `tests/test_macro_errors.sh`
- **Windows CI**: `.github/workflows/ci-windows.yml`

## Performance Characteristics

| Operation | Time |
|-----------|------|
| Block allocation | ~3 cycles |
| Global reset | ~5 ns (O(1)) |
| `fn` call (static inline) | 0 cycles (inlined) |
| `fn` call (export) | ~1-2 cycles |
| Interface dispatch (vtbl) | ~2-3 cycles |
| Pool alloc (≤64B) | O(1) |
| Compiler throughput | ~50K lines/sec |
| Generated C performance | ~gcc -O3 native |
