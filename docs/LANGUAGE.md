# Brick Language Reference

> Everything you need to know to write code in Brick.

## File Structure (.brc)

```
package NAME              ← which package this file belongs to
using OTHER               ← import another package

block global = 256MB       ← block declaration (REQUIRED in main file)
block game   = 64MB

struct Player { }          ← your structs with methods

fn main() { }              ← program entry point
```

> Every `.brc` file starts with a package declaration, optional imports, block declarations, structs, and functions.

## Packages

```
package SPRITES                    ← declare package
package SPRITES.EFFECTS            ← sub-package (hierarchy)
using SPRITES                      ← import everything from package
private int secret                 ← visible only within the package
```

> Everything is `public` by default. Use `private` to hide.

## Structs (with methods!)

```
struct Player {
    int hp                     ← field
    String name                ← field

    fn Player(int h, String n) {    ← CONSTRUCTOR (same name as struct)
        hp = h
        name = n
    }

    fn take_damage(int dmg) {       ← normal method
        hp -= dmg
    }

    fn get_hp() -> int {            ← method with return value
        return hp
    }
}
```

> Structs bundle data and methods together. The constructor has the same name as the struct.

### Inheritance

```
struct NPC extends Player {
    int ai_type

    fn NPC(int h, int ai) {
        hp = h                    ← inherited field from Player
        ai_type = ai
    }
}
```

> A struct can extend another, inheriting its fields.

### Interface

```
interface Damageable {
    fn take_damage(int d)
}

struct Enemy : Damageable {       ← or "extends" + "implements"
    fn take_damage(int d) { }
}
```

> Interfaces define method contracts that structs must implement.

## Memory Blocks

```
block global = 256MB        ← declare block (KB, MB, GB)
block temp   = 8KB

int x = 5                   ← goes to global block (default)

block game {                ← scope: everything here goes to block game
    Player p = Player(100, "Felipe")
    Enemy e = Enemy(50)
}

float f = 2.0 @temp        ← explicit inline allocation
int hp = p.get_hp()

game.reset()               ← clears EVERYTHING in block game (super fast)
```

> Memory blocks are the heart of Brick. They are contiguous regions of memory. Allocating is just moving a pointer — super fast. Resetting a block clears everything at once.

Rules:
- `global` is the default block (must be declared in main)
- `block name: { }` changes the default block within that scope
- `@name` allocates a specific variable in a block
- `block.reset()` clears the entire block (no individual free)
- If the block fills up: `error("block overflow")` — the program aborts

## Types

### Fixed-Width Types

| Brick | C Type       | Size    | Description          |
|--------|-------------|---------|----------------------|
| `i8`   | `int8_t`    | 8 bits  | Signed integer       |
| `i16`  | `int16_t`   | 16 bits | Signed integer       |
| `i32`  | `int32_t`   | 32 bits | Signed integer       |
| `i64`  | `int64_t`   | 64 bits | Signed integer       |
| `u8`   | `uint8_t`   | 8 bits  | Unsigned integer     |
| `u16`  | `uint16_t`  | 16 bits | Unsigned integer     |
| `u32`  | `uint32_t`  | 32 bits | Unsigned integer     |
| `u64`  | `uint64_t`  | 64 bits | Unsigned integer     |
| `f32`  | `float`     | 32 bits | Floating point       |
| `f64`  | `double`    | 64 bits | Double precision     |
| `usize`| `size_t`    | pointer | Unsigned pointer-size|
| `isize`| `ptrdiff_t` | pointer | Signed pointer-size  |

> Every number type has a fixed, predictable size — no guessing.

### Aliases

| Alias    | Maps To | Notes                      |
|----------|---------|----------------------------|
| `int`    | `i32`   | Default integer            |
| `float`  | `f32`   | Default float              |
| `char`   | `u8`    | Character                  |
| `byte`   | `u8`    | Same as char               |
| `short`  | `i16`   | Short integer              |
| `long`   | `i64`   | Long integer               |
| `double` | `f64`   | Double precision           |

> Friendly names for the most common types.

### Other Types

| Brick     | C Type         | Description                  |
|------------|----------------|------------------------------|
| `bool`     | `uint8_t`      | true/false                   |
| `String`   | `BrickString`  | Built-in string (dynamic)    |
| `T[N]`     | `T[]`          | Fixed array of N elements    |
| `null`     | `NULL`         | Null pointer                 |
| `void`     | `void`         | No return value (functions)  |

### Literal Suffixes

```
42u8   42u16  42u32  42u64     ← unsigned integer types
42i8   42i16  42i32  42i64     ← signed integer types
3.14f32  3.14f64               ← float types
42usz  42isize                 ← pointer-size types
```

> Unsuffixed literals infer their type from context (fits target -> allowed).
> Overflow on compile-time literal -> error.

### Type Rules

> - **Widening** allowed: i8 -> i16, u8 -> u64, f32 -> f64
> - **Narrowing** prohibited: i64 -> i32 = error
> - **Signed <-> Unsigned** same rank prohibited: i32 <-> u32 = error
> - **Int + Float** -> Float (int promotes to float)
> - **Mixed expressions**: promotion to type that fits both operands

## Functions

```
fn main() { }                          ← entry point (no return)

fn add(int a, int b) -> int {          ← function returning int
    return a + b
}

fn log(String msg) {                   ← void function (no return)
    // ...
}
```

> Parameters and return values go to the compiler's internal "anonymous block".

## Flow Control

```
if cond { }
else { }

while cond { }

for int i = 0; i < 10; i++ { }

return expr
```

> Standard flow control: if/else, while, for, and return.

## Operators

| Operator       | What it does              |
|----------------|---------------------------|
| `+ - * /`      | Basic math                |
| `== != < > <= >=` | Comparison             |
| `&& \|\| !`    | Logic (and, or, not)      |
| `= `           | Assignment                |
| `.`            | Access field/method       |
| `()`           | Function call             |
| `[]`           | Array index               |
| `@`            | Allocate in specific block|
| `->`           | Return type               |
| `& \| ^ ~ << >>` | Bitwise operations     |

## Strings

```
String s = "hello"                     ← creates string
String nome = "Felipe" @game           ← string in a block
String empty = ""                      ← empty string
```

> String is a built-in type. It lives in blocks like any other struct.

## Arrays

```
int[10] arr                            ← fixed array of 10 integers
int[5] vals = {1, 2, 3, 4, 5}         ← with brace initializer
f32[4] m = {1.0, 0.0, 0.0, 1.0}      ← matrix init
```

> Fixed size defined at declaration. Brace init `{val, ...}` sets all elements.
> Array literals in expressions generate C99 compound literals `(T[]){val, ...}`.

## Error Handling

```
error("something went wrong")          ← prints message and aborts
```

> No try/catch. If something goes wrong, the program prints the error and exits.

## Visibility

```
public int x                           ← visible everywhere (default)
private int y                          ← visible only within own package
```

> `public` is the default. `private` restricts visibility to the package.

## Comments

```
// This is a comment  ← line comment only
```

> Line comments only (`//`). Block comments (`/* */`) are not supported.

## Macros

Macros generate code at compile time. You define a pattern once and the compiler stamps it out everywhere you call it.

```
macro swap(a, b) {
    __tmp = $a
    $a = $b
    $b = __tmp
}

fn main() {
    x = 10; y = 20
    swap(x, y)            // expands to the swap body
    print("{0} {1}", x, y) // → "20 10"
}
```

### Declaration

```
macro name(param1, param2, ...) { body }
```

- Parameters are **untyped** — they hold expressions, not values
- `$name` inserts the argument passed for `name`
- `$(expr)` evaluates `expr` at compile time and inserts the result
- `__`-prefixed names get unique gensyms (no collision with user code)

### `build {}` — Compile-Time Computation

```
build {
    x = 42
    emit { z = x }        // generates: z = 42
}
```

`build` runs at compile time. Variables inside `build` do not exist in the final binary. Use `emit` to produce actual code.

### `emit {}` — Code Generation

Everything inside `emit` is literal code with `$` interpolation. It is stamped into the output at the macro call site.

```
macro make_getter(name, field) {
    emit {
        fn get_$name() { return $field }
    }
}
```

### Hygiene

Variables starting with `__` inside a macro get unique names (e.g. `__tmp` → `__tmp__1`), preventing collisions with user code.

### Type Reflection (inside `build`)

| Expression | Returns | Example |
|-----------|---------|---------|
| `T.name` | Type name as string | `"i32"` |
| `T.size` | Size in bytes | `4` |
| `T.fields` | Field names as strings | `["x", "y"]` |

### Error Handling

| Situation | What happens |
|-----------|-------------|
| Wrong argument count | Compile error |
| `$` outside macro/build | Compile error: "unexpected $" |
| Recursive macro | Caught after 64 levels |
| I/O inside `build` | Build blocks can't call print() |

See [Macros](MACROS.md) for full documentation with examples.

## I/O (Package IO)

```
using IO

fn main() {
    print(42)                    → "42"
    print(3.14)                  → "3.140000"
    print(true)                  → "true"
    print("hello")               → "hello"
    print()                      → (blank line)
    print("x = {0}", 10)         → "x = 10"
    print("{0} + {1} = {2}", 1, 2, 3)  → "1 + 2 = 3"
}
```

> `using IO` is required to use `print()`. It always adds a newline at the end (println semantics).
>
> `{0}`, `{1}` etc. refer to arguments positionally. Supported types: all numeric types, bool, char, String.

## C Interop

### Include a C header and link a library

```
include "math.h" and link m
```

Or separately:

```
include "SDL.h"
link SDL2
```

### Declare external C functions

```
extern fn sqrt(f64 x) -> f64
extern fn atoi(*u8 str) -> i32
extern fn puts(*u8 s) -> i32
extern fn sin(f64 x) -> f64
```

> Functions from always-included headers (stdio.h, stdlib.h, string.h) only need `extern fn` for the type checker — prototypes are already provided by the headers.
>
> `String` → `*u8` conversion happens automatically when passing a String to a `*u8` parameter.

### Pointer syntax

- `*u8` → `char*`
- `*void` → `void*`
- `*MyStruct` → `MyStruct*`

### Generate C bindings from a C header

```
brick bind <header.h>    # generates .brc bindings
```

## Debugging

Brick generates `#line` directives in the C output so you debug in `.brc` code, not generated C:

```
brick input.brc -o output.c      # generate C with #line
gcc -g -O0 output.c ...          # compile with debug info
gdb ./a.out                      # GDB shows .brc source
```

GDB pretty-printers are in `debugger/` — they add custom commands:

| Command | What it does |
|---------|-------------|
| `info blocks` | Lists all memory blocks and their usage |
| `block <name>` | Shows detailed info about a block |
| `block-watch` | Watches block allocations |

Load them automatically with:

```
gdb -x debugger/.gdbinit ./program
```

## Complete Example

```
package GAME

using IO

block global = 256MB
block game   = 64MB

interface Drawable {
    fn draw()
}

struct Player {
    i32 hp
    String name
    u8 active

    fn Player(i32 h, String n) {
        hp = h
        name = n
        active = 1u8
    }

    fn damage(i32 dmg) {
        hp -= dmg
        if hp < 0 { hp = 0 }
        print("{0} took {1} damage, hp={2}", name, dmg, hp)
    }
}

fn main() {
    Player p = Player(100, "Felipe") @game
    p.damage(20)
    game.reset()
}
```

> This example shows a complete Brick program with package, blocks, interface, struct, and main function.
