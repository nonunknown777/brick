# Getting Started with Brick

## Prerequisites

- **Linux** or **Windows** with MinGW-w64
- **C++ compiler**: gcc ≥ 9 or clang ≥ 12
- **Python 3** and **pip**
- **SCons** (`pip install scons`)
- **ncurses** (Linux): `sudo apt install libncurses-dev` (optional, for visualizer)

## Build the Compiler

```bash
git clone <repo>
cd brick
scons                    # builds the 'brick' compiler in build/
```

### Build Options

```bash
scons profile=debug      # debug build (no optimization, GDB-friendly)
scons visualizer=no      # skip TUI visualizer
scons vsix=yes           # build VS Code extension (.vsix)
```

## Hello World

Create `hello.brc`:

```brick
using IO

block global = 64MB

fn main() {
    print("Hello, Brick!")
}
```

Compile and run:

```bash
brick run hello.brc
# Output: Hello, Brick!
```

Or step by step:

```bash
brick hello.brc -o hello.c    # compile Brick → C
gcc -O3 hello.c runtime/block_memory.c runtime/io.c -o hello
./hello                        # Hello, Brick!
```

## First Real Program

```brick
package GAME
using IO

block global = 256MB
block game = 64MB

interface Damageable {
    fn take_damage(int dmg)
}

struct Enemy : Damageable {
    int hp
    String name

    fn Enemy(int h, String n) {
        hp = h; name = n
    }

    fn take_damage(int dmg) {
        hp -= dmg
        if hp <= 0 {
            print("{0} destroyed!", name)
        }
    }
}

fn main() {
    Enemy e = Enemy(100, "Goblin") @game
    e.take_damage(30)          # hp = 70
    e.take_damage(80)          # hp = -10 → "Goblin destroyed!"
    game.reset()               # cleanup
}
```

```bash
brick build game.brc -o game
./game
```

## Multi-File Project

```
project/
├── main.brc
├── lib/
│   └── MATH.brc
```

`lib/MATH.brc`:

```brick
package MATH

export fn add(int a, int b) -> int {
    return a + b
}

export const PI = 31415
```

`main.brc`:

```brick
package GAME
using IO
using MATH

block global = 64MB

fn main() {
    int r = add(3, 4)
    print("{0}", r)          # 7
}
```

Build:

```bash
brick build main.brc -I lib -o program
./program
```

## VS Code Extension

```bash
cd vscode-ext
npm install
npm run compile
# Then press F5 to launch Extension Development Host
```

Features: syntax highlighting, LSP (completions, hover, go-to-def, signature help, semantic tokens), memory webview panel.

## Next Steps

- [Language Reference](LANGUAGE.md) — Complete syntax reference
- [Architecture](ARCHITECTURE.md) — How the compiler works
- [Macros](MACROS.md) — Compile-time code generation
- [Hot Reload](hot-reload.md) — Live code swapping
