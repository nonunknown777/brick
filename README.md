# Brick

**v1.1.0** — A programming language that compiles to C. Maximum performance, explicit block-based memory, native hot reload.

```brick
package GAME
using IO
block global = 64MB

interface Damageable { fn take_damage(i32 dmg) }

struct Player : Damageable {
    i32 hp
    String name
    int[] items

    fn Player(i32 h, String n) {
        hp = h; name = n
    }

    fn take_damage(i32 dmg) {
        hp -= dmg
        if hp < 0 { hp = 0 }
        print("{0} took {1} damage, hp={2}", name, dmg, hp)
    }
}

fn main() {
    Player p = Player(100, "Felipe") @game
    p.take_damage(20)
    game.reset()
}
```

## Quick Start

```bash
git clone <repo> && cd brick
scons                          # build compiler
brick run examples/hello.brc   # compile and run
brick build hello.brc -o hello # build to binary
```

## Features

| Category | Features |
|----------|----------|
| **Types** | Fixed-width: `u8`..`u64`, `i8`..`i64`, `f32`/`f64`, `usize`/`isize`, `byte`, `bool`, `String`. Aliases: `short`/`long`/`double`, `type NAME = TYPE`. Bitfields: `u4`, `i3` |
| **Literals** | Hex (`0xFF`), binary (`0b1010`), octal (`0o777`), underscore separators (`1_000_000`), type suffixes (`42u8`, `3.14f64`) |
| **Arrays** | Fixed `int[10]`, dynamic `int[]` with `.append`/`.len`/`.cap`, array literals `{1, 2, 3}` |
| **Structs** | Methods, constructors, inheritance (`extends`), interfaces (`interface`), vtbl dispatch, `impl` blocks, anonymous struct/union nesting, `@packed`/`@align(N)`, named init literals |
| **Unions** | Named unions, anonymous unions inside structs, anonymous structs inside unions |
| **Enums** | Named constants with hex values (`enum Flags { A = 0xFF }`) |
| **Pointers** | `*T`, full pointer arithmetic (`+`, `-`, `+=`, `-=`, `++`, `--`, `[]`, `==`, `!=`, `<`, `>`), null checks |
| **Flow Control** | `if`/`else`, `while`, `for` (C-style + `for x in N`), `break`/`continue`, `match` with guards, `defer` |
| **Constants** | `const NAME = value` with compile-time evaluation, used in array sizes |
| **Functions** | `fn`, `->` return type, `export fn` for C visibility, function pointers `fn(int)->void`, default params |
| **Macros** | `macro` with `$` interpolation, `build {}` compile-time eval, `emit {}` code gen, varargs `args...`, hygiene, `$macro()` explicit syntax |
| **Packages** | `package NAME`, `using PACKAGE`, `export`/`private`, multi-file projects, nested packages (`MATH.VEC2`), auto-resolution from filesystem, `-I` flag, `BRICK_PATH` |
| **C Interop** | `include`, `link`, `extern fn`, `@system` for angle-bracket includes, `export fn` for linker visibility, `*u8` → `char*`, `String` → `*u8` auto-conversion |
| **Operators** | `and`/`or`/`not` keywords, `&&`/`||`/`!`, bitwise (`&` `|` `^` `~` `<<` `>>`), `sizeof`/`alignof`, increment/decrement (`++`/`--`) |
| **Memory** | Block-based bump allocator (~3 cycles), pool allocator for types ≤64B, TLS blocks, double-buffer hot reload |
| **Visibility** | `private`/`public` on fields, functions, structs, consts, enums, unions, interfaces, type aliases, macros |
| **Error Handling** | `error("msg")` panic with message and abort |

## CLI

```bash
brick <input.brc> [-o output.c]           # compile to C
brick build <files...> [-o output]         # build to binary
brick run <input.brc>                      # compile and run
brick new <project>                        # scaffold new project
brick bind <header.h>                      # C bindings
brick --visualize <file>                   # compile + TUI visualizer
brick --attach <pid>                       # attach visualizer to process
```

## Documentation

- [Getting Started](docs/GETTING_STARTED.md) — Install, build, first program
- [Language Reference](docs/LANGUAGE.md) — Complete syntax, types, packages, memory
- [Architecture](docs/ARCHITECTURE.md) — Compiler pipeline (Lexer → Parser → Macros → Codegen)
- [Macros](docs/MACROS.md) — Compile-time code generation
- [Hot Reload](docs/hot-reload.md) — Live code swapping
- [Optimizations](docs/OPTIMIZATIONS.md) — Performance details

## License

MIT
