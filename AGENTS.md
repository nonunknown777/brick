# Brick

## Project

A programming language that compiles to C. Focus on maximum performance,
explicit block-based memory management, and native hot reload.

## Stack
- Compiler: C++20
- Runtime: C (external ABI for hot reload)
- Build: SCons
- Visualizer: ncurses (Linux) / PDCurses (Windows)
- Hot Reload: dlopen+inotify (Linux) / LoadLibrary+ReadDirectoryChangesW (Windows)

## Structure
```
src/           → compiler (C++20)
runtime/       → block memory allocator + hot reload (C)
visualizer/    → TUI ncurses (C++)
debugger/      → GDB pretty-printers + .gdbinit (Python)
tasks/         → 11 tasks, each with AGENTS.md + state
examples/      → example .brc code
tests/         → unit tests
vscode-ext/    → VS Code extension (highlight + LSP + debug webview)
```

## Tasks
Each task in tasks/ has its own AGENTS.md with specific instructions.
Always read STATE.md and NEXT.md when starting a session.

```
01-lexer      Tokenizer .brc -> tokens
02-parser     AST + Package Resolution
03-codegen    Type check + C generation (with #line for debug)
04-runtime    Block memory allocator (C)
05-hotreload  dlopen + inotify
06-visualizer TUI ncurses
07-builder    SCons build
08-vscoder    VS Code extension (highlight + LSP + debug webview)
09-debugger   GDB pretty-printers + #line directives + VS Code Memory View
10-tester     Test runner + optimizer + documentation + coordinator (Windows CI)
11-libs       Official libraries (window, input, audio, file, net, math)
```

## Conventions
- C++20, snake_case for functions, PascalCase for structs
- Runtime headers ALWAYS with `extern "C"`
- Every feature with a test in tests/
- Generated C code must be readable
- No `this`, no name shadowing
- `error()` for panic

## Full Spec
See shared-context.md for the complete language specification.
