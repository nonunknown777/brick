# Meta-C Language Reference

Everything you need to know to write code in Meta-C.

## File Structure (.mc)

```
package NAME              ← which package this file belongs to
using OTHER               ← import another package

block global = 256MB       ← block declaration (REQUIRED in main file)
block game   = 64MB

struct Player { }          ← your structs with methods

fn main() { }              ← program entry point
```

## Packages

```
package SPRITES                    ← declare package
package SPRITES.EFFECTS            ← sub-package (hierarchy)
using SPRITES                      ← import everything from package
private int secret                 ← visible only within the package
```

Everything is `public` by default. Use `private` to hide.

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

### Interface

```
interface Damageable {
    fn take_damage(int d)
}

struct Enemy : Damageable {       ← or "extends" + "implements"
    fn take_damage(int d) { }
}
```

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

Rules:
- `global` is the default block (must be declared in main)
- `block name: { }` changes the default block within that scope
- `@name` allocates a specific variable in a block
- `block.reset()` clears the entire block (no individual free)
- If the block fills up: `error("block overflow")` — the program aborts

## Types

### Fixed-Width Types

| Meta-C | C Type       | Size    | Description          |
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

### Other Types

| Meta-C     | C Type         | Description                  |
|------------|----------------|------------------------------|
| `bool`     | `uint8_t`      | true/false                   |
| `String`   | `MetaCString`  | Built-in string (dynamic)    |
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

- Unsuffixed literals infer their type from context (fits target -> allowed)
- Overflow on compile-time literal -> error

### Type Rules

- **Widening** allowed: i8 -> i16, u8 -> u64, f32 -> f64
- **Narrowing** prohibited: i64 -> i32 = error
- **Signed <-> Unsigned** same rank prohibited: i32 <-> u32 = error
- **Int + Float** -> Float (int promotes to float)
- **Mixed expressions**: promotion to type that fits both operands

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

Parameters and return values go to the compiler's internal "anonymous block".

## Flow Control

```
if cond { }
else { }

while cond { }

for int i = 0; i < 10; i++ { }

return expr
```

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

String is a built-in type. It lives in blocks like any other struct.

## Arrays

```
int[10] arr                            ← fixed array of 10 integers
int[5] vals = int[5] @game             ← array in a block
```

Fixed size defined at declaration.

## Error Handling

```
error("something went wrong")          ← prints message and aborts
```

No try/catch. If something goes wrong, the program prints the error and exits.

## Visibility

```
public int x                           ← visible everywhere (default)
private int y                          ← visible only within own package
```

## Comments

```
// This is a comment  ← line comment only
```

Line comments only (`//`). Block comments (`/* */`) are not supported.

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
