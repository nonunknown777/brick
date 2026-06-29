# Changelog

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
