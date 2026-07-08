# Changelog

## v1.0.0 — First Stable Release

### Major: Complete Language Implementation
- **First stable release** — Full compiler pipeline: lexer → parser → codegen → C → binary
- **Block-based memory management** — Bump allocator (`block name = 64MB`), pool allocator (`pool_create`), TLS blocks, double-buffer API
- **Native hot reload** — Linux (dlopen+inotify), Windows (LoadLibrary+ReadDirectoryChangesW), atomic function swap
- **TUI memory visualizer** — ncurses (Linux) / PDCurses (Windows), standalone and attach modes
- **Macro system** — `macro` with `$` interpolation, `macro NAME(args...)` varargs, `build {}` compile-time eval, `emit {}` code gen
- **`$macro()` explicit call syntax** — Call macros with `$name(args)` for visual clarity alongside `name(args)`
- **C interop** — `include`/`link`/`extern fn`, `@system` for angle-bracket includes, `export fn` for linker-visible functions
- **Windows native port** — MinGW-w64, VirtualAlloc blocks, LoadLibrary hot reload, Win32 window library, full CI pipeline

### Language Features (v1.0.0 Complete)
- **Binary `&` operator** — Bitwise AND implemented as a binary operator (`a & b`). Parser updated to handle `&` in expressions alongside `*`/`/` precedence. Type checker validates integer operands. Constant folding for compile-time evaluation.
- **Struct alignment** — Pool allocator now correctly accounts for struct field alignment padding. Added `type_alignment()` helper; struct size estimation includes per-field padding and trailing struct alignment. Fixes pool allocator crash for structs near the 64-byte threshold.
- **Enum type handling** — Enum variants now have type `int` instead of the enum name, allowing direct use in arithmetic and bitwise operations. `can_assign()` recognizes enum types as integers.
- **Integer conditions** — `if`/`while`/`for`/`not`/`or`/`and` now accept integer types in addition to `bool`, enabling natural C-style conditions like `if flags & 0x02`.
- **Bug fixes** — Removed BIT_OR/BIT_XOR dead code from type_checker and codegen. Fixed enum fallback in `can_assign()` to use normalized types. Fixed float constant folding (was incorrectly checking `INT_LITERAL` instead of `FLOAT_LITERAL`).
- **Arrays** — Fixed `int[10]`, dynamic `int[]` with `.append`/`.len`/`.cap`, local stack arrays, array literals `{1, 2, 3}`
- **Struct enhancements** — Arrays in struct fields, anonymous struct/union nesting, `@packed`/`@align(N)` attributes, init literals (positional + named)
- **Interfaces & polymorphism** — `interface` declarations, `impl Struct : Interface` separate block, vtbl dispatch, `is`/`as` type checks
- **Types** — Fixed-width (`u8`..`u64`, `i8`..`i64`, `f32`/`f64`, `usize`/`isize`, `byte`), `short`/`long`/`double` aliases, `type NAME = TYPE` aliases, bitfields (`u4`, `i3`), `sizeof`/`alignof`
- **Flow control** — `if`/`else`, `while`, `for` (C-style + `for x in N`), `break`/`continue`, `match` with patterns, `defer`
- **Literals** — Hex (`0xFF`), binary (`0b1010`), octal (`0o77`), underscore separators (`1_000_000`), float, char, string
- **Enums** — Named constants with hex values (`enum Flags { A = 0xFF; B }`)
- **Unions** — `union Data { int i; float f }`, anonymous union inside struct
- **Pointers** — `*T` pointer type, full pointer arithmetic, pointer arrays `*T[N]`
- **Constants** — `const NAME = value` with compile-time expression evaluation
- **Narrowing** — Explicit cast with `as` operator, overflow checking, widening rules
- **Logical operators** — `and`/`or`/`not` keywords alongside `&&`/`||`/`!`
- **Error handling** — `error("msg")` panic with message and abort
- **Private/public** — Visibility modifiers at field and top-level
- **Package system** — `package NAME`, `using Package`, relative includes

### VS Code Extension v1.0.0
- **Syntax highlighting** — All keywords, types, operators, attributes (`@packed`/`@align`), macro sigil (`$`), pointers
- **Language Server** — Completions (keywords, snippets, runtime functions, document symbols), hover docs, go-to-definition, signature help, document symbols, semantic tokens, diagnostics
- **Full feature support** — `union`, `impl`, `type`, dynamic arrays, bitfields, anonymous struct/union, `@packed`/`@align`, `export fn`, `@system` includes, `$macro()`, pointer types, all fixed-width types, `and`/`or`/`not`, `short`/`long`/`double`, `defer`, `const`, `match`, `break`/`continue`, `for x in N`
- **Debug webview** — Memory blocks, pool allocator inspection
- **Workspace setup** — Auto-generated `launch.json`/`tasks.json` with debug and compile tasks
- **283 scanner tests** — All passing with zero failures

### Documentation
- **Full language spec** — `docs/LANGUAGE.md` (EN + PT-BR) with 30+ sections covering every feature
- **Macro documentation** — `docs/MACROS.md` with comprehensive examples
- **Architecture docs** — `docs/ARCHITECTURE.md`, `docs/OPTIMIZATIONS.md`, `docs/hot-reload.md` (EN + PT-BR)
- **Website** — `docs/index.html` with feature cards for all capabilities
- **Wiki** — All pages updated: Language-Reference, Home, FAQ, Architecture, Getting-Started, Memory-Blocks, Hot-Reload, Performance, VS-Code-Extension, Contributing
- **215 codegen tests** — All passing, 0 failures

### Tooling & Infrastructure
- **`brick build`/`brick run`** — CLI for compile and execute
- **SCons build system** — Multi-target (Linux/Windows), profiles (release/debug/sanitize/pgo), parallel build
- **GDB integration** — Pretty-printers for BlockCtx/BrickString, `info blocks` command, `#line` directives
- **GitHub Actions CI** — Linux and Windows pipelines, full build + release automation
- **Gravity Index** — Integrated tool discovery for third-party services

---

## v0.6.0 — Windows Support

### Major: Native Windows Port
- Full Windows support via MinGW-w64 (GCC ≥ 13)
- `block_memory.c`: `VirtualAlloc`/`VirtualFree` (was `mmap`/`munmap`), `CRITICAL_SECTION` (was `pthread_mutex`), `BRICK_TLS` (was `__thread`)
- `hot_reload.c`: `LoadLibraryA`/`GetProcAddress`/`FreeLibrary` (was `dlopen`/`dlsym`/`dlclose`), `ReadDirectoryChangesW` with overlapped I/O (was `inotify`)
- `pool_allocator.c`: guarded `_GNU_SOURCE` behind `#ifndef _WIN32`
- `src/main.cpp`: `CreateProcess` for visualize mode, `GetTempPathA`/`CreateDirectoryA` for temp dirs, `cmd /c` for shell commands
- `SConstruct`: native Windows host detection, `g++`/`gcc` auto-detection, skips X11/pthread on Windows
- `scripts/embed_runtime.py`: Windows CC auto-detection via `shutil.which`, forward-slash path normalization
- `brc-run.bat`: Windows wrapper to compile `.brc` → `.c` → `.exe`
- `build-release.ps1`: Windows equivalent of `build-release.sh`
- `libs/window`: `window_win32.c` with Win32 API (CreateWindow, PeekMessage, etc.) — user32/gdi32 linking
- All 159 codegen tests pass on Windows, 18 new Windows-specific runtime tests
- 6 new Windows integration tests (`brick build`, `brick run`, error detection, C interop)

### Documentation
- All docs updated with Windows build instructions and platform details
- AGENTS.md updated with Windows stack and conventions

---

## v0.5.0 — Macro System

### Major: Compile-Time Macro System
- `macro` declarations with `$` interpolation and `__` hygiene (gensym)
- `build {}` blocks for compile-time computation (arithmetic, strings, arrays, loops)
- `emit {}` for code generation at compile time
- `...` varargs: capture remaining arguments as a list
- Type reflection: `T.name`, `T.size`, `T.fields` inside `build`
- Recursion guard: max 64 levels before abort
- 26 unit tests + 5 macro error tests + integration tests
- Dedicated docs: `docs/MACROS.md`, `docs/MACROS.pt-BR.md`

### Type Checker Hardening
- `can_assign()` now validates typed variable declarations (`int x = expr`)
- 12 new type-checker error tests (narrowing, signed/unsigned mixing, void return, etc.)
- C interop test (`test_c_interop.brc`) in integration suite

### VS Code Extension v0.5.0
- `macro`, `build`, `emit` keywords in syntax highlighting
- `macro`/`build`/`emit` snippets
- `$` and `...` tokenization support in language service

### Documentation
- All docs updated: LANGUAGE.md, ARCHITECTURE.md, GETTING_STARTED.md, DESIGN.md
- Wiki updated: Language-Reference, Architecture, Home, Getting-Started, Contributing
- `docs/index.html` has Macro feature card, pipeline step, and optimizations section

### Infrastructure
- CI: syntax highlighting tests for macro tokens
- Integration: `scons test` includes macro error tests

---

## v0.4.0 — Pool Allocator & Optimizations

### Major: 7 Optimizations
- Inline hints (`__attribute__((always_inline))`) in codegen
- SIMD alignment (aligned(16/32)) for float/f64 struct fields
- Constant folding (pre-compute constant expressions)
- Pool allocator (`runtime/pool_allocator.c/.h`) with O(1) alloc/free
- Thread-local blocks (TLS `__thread` current block)
- Pauseless hot reload (double-buffer API)
- PGO build profiles (`profile=pgo-gen`, `profile=pgo-use`)
- Pool allocator auto-integration in codegen (≤64 bytes → pool, >64 → bump)

### C Interop
- `include`/`link` directives for C headers and libraries
- `extern fn` for declaring C functions
- Automatic `String` → `*u8` conversion
- `brick bind <header.h>` for generating bindings

### Documentation Overhaul
- OPTIMIZATIONS.md deep-dive (EN + PT-BR)
- index.html updated with 7 optimization cards
- README.md with optimization summary table
- LANGUAGE.md: I/O, C Interop, Debugging sections
- All docs migrated from "Meta-C" to "Brick"

---

## v0.3.0 — Blocks & Memory

### Major: Block Memory Model
- Explicit block declarations (`block name = 64MB`)
- Bump allocator (O(1) allocation)
- Block scope (`block name: { }`)
- Inline block allocation (`@` annotation)
- `block.reset()` (O(1) free)
- Pool allocator threshold integration
- Thread-local current block

### Hot Reload
- dlopen + inotify engine
- Atomic function pointer swap
- Block freeze/thaw support
- Double-buffer for pauseless reload

### Visualizer
- ncurses TUI showing real-time block usage
- Standalone and attach modes
- Shared memory export for process monitoring

---

## v0.2.0 — C Code Generation

### Major: First Working Compiler
- Complete lexer, parser, codegen pipeline
- `.brc` → `.c` → `gcc` → binary flow
- Fixed-width types: i8..i64, u8..u64, f32/f64, usize/isize
- Structs with methods, constructors, inheritance, interfaces
- `#line` directives for debugging in .brc
- GDB pretty-printers for BlockCtx and String
- `using IO` with `print()` support
- Literal suffixes and overflow checking
- Package system with hierarchical namespaces

### Tooling
- `brick build`, `brick run` CLI
- SCons build system with debug/sanitize profiles
- VS Code extension with syntax highlighting and LSP
- Integration test framework (.brc → binary verification)
- Initial documentation (LANGUAGE.md, ARCHITECTURE.md, GETTING_STARTED.md)
