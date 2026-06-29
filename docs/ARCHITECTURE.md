# Brick Architecture

> Explained simply for anyone to understand.

## Overview

Brick is a programming language you write in a `.brc` file and it becomes a real program. The path is:

```
Your .brc code  →  Compiler  →  C code  →  gcc  →  Final program
```

## Project Parts

### src/ — The Brain (Compiler in C++20)

The compiler has 4 parts in an assembly line:

```
1. LEXER         2. PARSER           3. MACRO SYSTEM    4. CODEGEN
   ┌─────┐         ┌──────┐           ┌──────────┐        ┌──────┐
   │ .brc │ → tokens → │ AST │ → expand → │ .c  │
   └─────┘         └──────┘           └──────────┘        └──────┘
                                            ↓
                                    collect + build + expand
```

**Lexer** (`src/lexer/`):
- Takes your `.brc` file and breaks it into tokens
- Example: `int x = 5` becomes [INT, IDENT("x"), ASSIGN, INT_LITERAL(5)]
- Ignores comments and whitespace

**Parser** (`src/parser/`):
- Takes tokens and builds an AST (Abstract Syntax Tree)
- Example: `if (x > 0) { }` becomes a branch with condition + body
- Also resolves `package` and `using` — figures out imports
- Handles `macro`, `build`, and `emit` declarations

**Macro System** (`src/parser/` — `macro_expander.cpp`, `build_eval.cpp`):
- Collects all `macro` declarations into a table
- Evaluates `build {}` blocks at compile time, running their `emit` calls
- Expands each `macro_call(...)` by cloning the macro body and substituting `$` parameters
- Generates unique names for `__`-prefixed variables (gensym)
- Detects infinite recursion (max 64 levels)

**Codegen** (`src/codegen/`):
- Walks the expanded AST (no macros left) and writes C code
- Each `struct` becomes `typedef struct`
- Each method becomes `StructName_method()`
- Type-checking: validates types, issues errors for mismatches
- Generates `#line` directives so you debug in `.brc`, not C

### runtime/ — The Foundation (C)

The core that runs alongside your program:

**block_memory.c**:
- Manages memory blocks (8MB, 32MB, 256MB...)
- Uses a bump allocator — super fast (~3 CPU cycles per allocation)
- Declare `block game = 64MB` and allocate objects inside it
- Clear the entire block with `block.reset()` — no individual free
- Freeze/thaw support for hot reload
- Cross-platform: `mmap` on Linux, `VirtualAlloc` on Windows (≥64KB), `malloc` fallback
- Thread safety: `pthread_mutex` on Linux, `CRITICAL_SECTION` on Windows
- TLS: `__thread` on GCC/Linux, `__declspec(thread)` on Windows/MinGW

**io.c**:
- Type-specific print functions: `io_print_i8`, `io_print_u32`, `io_print_f64`, etc.
- Uses exact PRI format specifiers from `<inttypes.h>`
- Formatted print with positional arguments: `print("x={0}", 42)`

**hot_reload.c**:
- Swap code without stopping your program
- Cross-platform abstraction: `dlopen`+`dlsym`+`inotify` on Linux, `LoadLibrary`+`GetProcAddress`+`ReadDirectoryChangesW` on Windows
- Atomic function pointer swap via `__atomic_store_n` (GCC) or `InterlockedExchange` (MSVC/MinGW)
- Watches `.so`/`.dll` files for modifications in a separate thread

### visualizer/ — The Eyes (TUI ncurses / PDCurses)

Shows memory blocks in real time in the terminal (ncurses on Linux, PDCurses on Windows):

```
global  256MB  ████████████░░░  67%
game     64MB  ██████░░░░░░░░  38%
```

### debugger/ — The Tools (GDB Python)

Helps debug Brick programs:
- Pretty-printers: shows BlockCtx nicely in GDB
- Commands: `info blocks`, `block <name>`
- .gdbinit loads everything automatically

### vscode-ext/ — The VS Code Extension

Makes VS Code understand the language:
- Syntax highlighting
- LSP server (completions, hover, diagnostics)
- Memory Webview (shows blocks visually during debug)

## How Everything Connects

```
You write:
    package GAME
    block global = 256MB
    block game = 64MB
    struct Player { int hp }
    fn main() { Player p = Player(100) @game }

         ↓ Lexer breaks into tokens
         ↓ Parser builds tree + resolves packages
         ↓ Macro system collects, builds, and expands macros
         ↓ Codegen writes C with #line directives
         ↓ gcc compiles C + runtime
         ↓

    Program running!
         ↓
    Debug with GDB (shows .brc source)
    View blocks with TUI visualizer
    Monitor in VS Code memory view
    Hot reload packages
```

## Why This Architecture?

- **Performance**: C++20 for the compiler, pure C for the runtime, gcc -O3 for the binary
- **Simplicity**: Each part does one thing well
- **Debuggable**: #line directives + pretty-printers + TUI visualizer
- **Hot Reload**: C runtime with dlopen = stable ABI
- **Memory-centric**: Blocks are the center of everything — visible, measurable, controllable
