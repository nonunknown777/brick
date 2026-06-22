# Getting Started with Meta-C

Quick guide to install, build, compile, run, and debug Meta-C programs.

## Prerequisites

| Dependency | Purpose | Install (Arch Linux) | Install (Ubuntu/Debian) |
|------------|---------|---------------------|-------------------------|
| **g++** (C++20) | Compile the Meta-C compiler | `sudo pacman -S gcc` | `sudo apt install g++` |
| **gcc** | Compile generated C code | `sudo pacman -S gcc` | `sudo apt install gcc` |
| **SCons** | Build system | `sudo pacman -S scons` | `sudo apt install scons` |
| **ncurses** | TUI visualizer | `sudo pacman -S ncurses` | `sudo apt install libncurses-dev` |
| **X11** (optional) | Window library | `sudo pacman -S libx11` | `sudo apt install libx11-dev` |

## Clone & Build

```bash
git clone https://github.com/nonunknown777/meta-c.git
cd meta-c
scons                     # release build (default)
scons profile=debug       # debug build
scons profile=sanitize    # sanitizers (for finding bugs)
```

The `meta-c` compiler will be at `build/meta-c`.

## Compile and Run Your First Program

### One-step (build + run)

```bash
build/meta-c run examples/hello.mc
```

### Build to binary

```bash
build/meta-c build examples/hello.mc -o hello
./hello
```

### Expected output

```
Hello from Meta-C!
42
3.140000
true
Hello World!
done with 2 blocks
```

## Two-in-One Commands

The `meta-c` CLI provides convenience subcommands:

```bash
# Build everything in one step (.mc → .c → gcc → binary)
build/meta-c build hello.mc -o hello

# Build and run in one step
build/meta-c run hello.mc

# Release mode (no tracking overhead, max performance)
build/meta-c build hello.mc --release -o hello
```

## Debugging

```bash
# Build with debug symbols
build/meta-c build hello.mc -o hello   # always includes -g

# Launch GDB
gdb ./hello
```

The `.gdbinit` in the project automatically loads Meta-C's debug support.

**Custom GDB Commands:**

```
(gdb) info blocks           # List all memory blocks with usage stats
(gdb) block <name>          # Show details of a specific block
(gdb) block-watch <name>    # Set breakpoint on block allocation
(gdb) ib                    # Alias for "info blocks"
```

Because the generated C code includes `#line` directives, GDB will display your original `.mc` source code.

## Running Tests

```bash
scons test                    # unit tests
tests/test_integration.sh     # integration tests (.mc → compile → run)
benchmarks/run_benchmarks.sh  # performance benchmarks
```

## TUI Memory Visualizer

```bash
build/meta-c --visualize examples/hello.mc   # compile, run, show TUI
build/meta-c --attach <pid>                  # attach to running process
```

## Quick Reference

```bash
scons                          # Build compiler
build/meta-c run input.mc      # Compile and run
build/meta-c build input.mc -o out  # Build to binary
build/meta-c input.mc -o out.c # Compile to C only
scons test                     # Run tests
build/meta-c --visualize file  # Compile + run + visualize
```

## Project Layout

```
meta-c/
├── src/              → Compiler (C++20)
├── runtime/          → C runtime (block alloc, IO, hot reload)
├── visualizer/       → ncurses TUI
├── debugger/         → GDB Python scripts
├── vscode-ext/       → VS Code extension
├── tests/            → Test suites
├── examples/         → Example .mc code
├── benchmarks/       → Performance benchmarks
└── docs/             → GitHub Pages site
```
