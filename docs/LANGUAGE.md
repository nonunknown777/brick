# Brick Language Reference (v1.1.0)

Complete reference for the Brick programming language. Covers every feature.

---

## 1. File Structure

```brick
package NAME              ← which package this file belongs to
using OTHER               ← import another package

block global = 256MB       ← block declaration (REQUIRED in main)
block game   = 64MB

type MyInt = int           ← type alias (optional)

struct Player { }          ← structs with methods
union Data { }             ← unions
enum Color { RED; GREEN }  ← enums

const MAX_HP = 100          ← compile-time constant
const int TILE = 16         ← constant with explicit type

fn main() { }              ← entry point
```

---

## 2. Comments

```brick
// This is a line comment
int x = 5  // inline comment
```

Line comments only (`//`). Block comments (`/* */`) are not supported.

---

## 3. Types

### 3.1 Fixed-Width Types

| Brick | C Type | Size | Description |
|-------|--------|:----:|-------------|
| `i8` | `int8_t` | 8 | Signed integer |
| `i16` | `int16_t` | 16 | Signed integer |
| `i32` | `int32_t` | 32 | Signed integer (default `int`) |
| `i64` | `int64_t` | 64 | Signed integer |
| `u8` | `uint8_t` | 8 | Unsigned integer (also `char`/`byte`) |
| `u16` | `uint16_t` | 16 | Unsigned integer |
| `u32` | `uint32_t` | 32 | Unsigned integer |
| `u64` | `uint64_t` | 64 | Unsigned integer |
| `f32` | `float` | 32 | Floating point (default `float`) |
| `f64` | `double` | 64 | Double precision |
| `usize` | `size_t` | ptr | Unsigned pointer-size |
| `isize` | `ptrdiff_t` | ptr | Signed pointer-size |
| `bool` | `uint8_t` | 8 | Boolean (true/false) |
| `String` | `BrickString` | dyn | Dynamic text (block-allocated) |
| `void` | `void` | — | Nothing (for functions) |

### 3.2 Aliases

| Alias | Maps To |
|-------|---------|
| `int` | `i32` |
| `float` | `f32` |
| `char` | `u8` |
| `byte` | `u8` |
| `short` | `i16` |
| `long` | `i64` |
| `double` | `f64` |

### 3.3 User-Defined Type Aliases

```brick
type MyInt = int
type Coord = f64
type Color = u32
type Callback = fn(i32)->void
```

### 3.4 Bitfield Types

Types `uN`/`iN` define fields with specific bit width (1–64):

```brick
struct Flags {
    u4  low_nibble     // 4-bit unsigned
    i3  signed_3bit    // 3-bit signed
    u1  single_bit     // 1-bit flag
    u24 address_part   // 24-bit address
    u8  byte_val       // 8-bit (same as u8, but explicit bitfield)
}
```

### 3.5 Literal Suffixes

```
42u8   42u16  42u32  42u64        ← unsigned integer types
42i8   42i16  42i32  42i64        ← signed integer types
3.14f32  3.14f64                  ← float types
42usz  42isize                    ← pointer-size types
```

Unsuffixed literals infer type from context. Overflow → compile error.

---

## 4. Literals

### Integer

```brick
42           ← untyped (inferred)
-128i8       ← typed suffix
0x00FF       ← hex literal
0b1010       ← binary literal
0o777        ← octal literal
1_000_000    ← underscore separator
```

### Float

```brick
3.14         ← inferred as f32 or f64
3.14f32      ← 32-bit float
3.14159f64   ← 64-bit float
1.0e40       ← large value promoted to f64
```

### Hex

```brick
0xFF         ← 255
0xABCD       ← 43981
0x1A2Bu16    ← hex with type suffix
0x443355FF   ← RGBA color
```

### Other

```brick
true         ← bool literal
false        ← bool literal
'a'          ← char literal
"hello"      ← string literal
null         ← null pointer literal
```

---

## 5. Type Rules

- **Widening** allowed: `i8` → `i16`, `u8` → `u64`, `f32` → `f64`
- **Narrowing** prohibited: `i64` → `i32` = error (use explicit cast: `i32(expr)`)
- **Signed ↔ Unsigned** same rank: prohibited (`i32` ↔ `u32` = error)
- **Int + Float** → Float (int promotes to float)
- **Explicit cast**: `T(expr)` or `expr as T` — allows narrowing

### Overflow Checking

```brick
u8 a = 300     // error: 300 does not fit in u8
u8 b = 0x1FF   // error: hex value too large for type
```

---

## 6. Variables

```brick
x = 5                ← inferred as int
int y = 10           ← explicit type
String name = "Brick"
float pi = 3.14
i64 big = 9223372036854775807
u8 small = 255u8
f32 precise = 3.14f32
```

Variables without initializer use the type name:

```brick
int x               ← declared but not initialized
String s            ← declared
```

---

## 7. Constants

```brick
const MAX_PLAYERS = 4            ← compile-time constant (type inferred)
const int TILE_SIZE = 16         ← with explicit type
const SCREEN_W = 800             ← can use in array sizes

const GRID_SIZE = 32
int[GRID_SIZE] buffer            ← constant used as array size
```

Constants are compile-time evaluated. Values are substituted directly in generated C code as `static const` variables.

---

## 8. Blocks of Memory

Blocks are contiguous memory regions with a bump allocator.

### Declaration

```brick
block global = 256MB       ← default block
block game   = 64MB
block temp   = 8KB
block data   = 1GB
```

**Units**: `B`, `KB`, `MB`, `GB` (case-sensitive).

### Allocation Modes

**1. Default block** — variables go to `global`:
```brick
int x = 5                  ← in global
String s = "hello"         ← in global
```

**2. Block scope** — everything inside targets that block:
```brick
block game {
    Player p = Player(100, "Felipe")   ← both in 'game'
    Enemy e = Enemy(50)
}
```

**3. Inline annotation** — allocate in a specific block:
```brick
float f = 2.0 @temp        ← f lives in 'temp'
Player p = Player(100, "Felipe") @game
```

### Block Operations

```brick
game.reset()               ← O(1) — releases ALL memory in 'game'
global.reset()
```

- **No individual free** — only reset entire blocks
- **Cross-references** between blocks are allowed
- **Overflow** → `error("block overflow")` — program aborts

### Pool Allocator

Types ≤ 64 bytes automatically use a pool allocator (O(1) free):
```brick
// Particle = 16 bytes → pool_alloc() used automatically
struct Particle { f32 x, y, z; i32 life }
Particle p = Particle() @global
```

### TLS Blocks

Each thread can have its own block via `__thread`:
```c
block_set_tls(my_block);  // from C
// Then allocations go to my_block without specifying it
```

### Double-Buffer

Zero-pause hot reload:
```c
block_enable_double_buffer(scene);
block_swap_buffers(scene);  // atomic swap in ~1 cycle
```

---

## 9. Structs (OOP)

### Basic Struct

```brick
struct Player {
    int hp
    String name
    int ammo

    // Constructor — same name as the struct
    fn Player(int h, String n, int a) {
        hp = h
        name = n
        ammo = a
    }

    // Method
    fn take_damage(int dmg) {
        hp -= dmg
    }

    // Method with return value
    fn get_hp() -> int {
        return hp
    }
}
```

Usage:
```brick
Player p = Player(100, "Felipe", 30) @game
p.take_damage(20)
int hp = p.get_hp()
```

### Constructor

```brick
Player p = Player(100, "Felipe", 30) @game
```

### Inheritance

```brick
struct NPC extends Player {
    int ai_type

    fn NPC(int h, String n, int a, int ai) {
        hp = h          // inherited from Player
        name = n
        ammo = a
        ai_type = ai    // new field
    }

    fn patrol() {
        // NPC-specific behavior
    }
}
```

Generated C: parent struct is first field (`base`), fields inherited directly.

### Interfaces

```brick
interface Damageable {
    fn take_damage(int d)
}

interface Serializable {
    fn save() -> String
}

// A struct implements multiple interfaces
struct Enemy : Damageable, Serializable {
    fn take_damage(int d) { }
    fn save() -> String { return "Enemy" }
}
```

### Separate impl Block

```brick
struct Arrow { int damage }

impl Arrow : Damageable {
    fn take_damage(int d) {
        damage = d
    }
}
```

### Virtual Dispatch (vtbl)

When `impl Struct : Interface` exists, the compiler generates:
- A vtbl struct with function pointers
- A wrapper struct with `void* data` and `const Vtbl* vtbl`
- Static vtbl instances
- Wrapper functions that cast `void*` to the concrete type

```brick
interface Drawable { fn draw() }
struct Circle : Drawable { fn draw() { print("Circle") } }

fn main() {
    Drawable d = Circle() @global
    d.draw()  // dispatches through vtbl
}
```

### is / as — Interface Type Checks

At runtime, check if a value implements an interface with `is`, and cast with `as`:

```brick
interface Drawable { fn draw() }
struct Circle : Drawable { fn draw() { print("Circle") } }
struct Square : Drawable { fn draw() { print("Square") } }

fn main() {
    Drawable d = Circle() @global

    // is — check if the underlying value is a specific type
    if d is Circle {
        print("it's a circle!")
    }

    // as — cast to concrete type
    Circle c = d as Circle
    c.draw()
}
```

- `expr is Type` → `bool` (does the interface value hold a `Type`?)
- `expr as Type` → `Type` (cast the interface value to the concrete type; aborts if wrong type)
- Both work on interfaces (vtbl dispatch) only

### Named Struct Initialization

```brick
Player p = {hp = 100, name = "Felipe", ammo = 30}  ← named
Player p2 = {100, "Felipe", 30}                     ← positional
```

### @packed/@align

```brick
struct Packed @packed { u8 a; i32 b }       // __attribute__((packed))
struct Aligned @align(64) { u8 a; i32 b }   // __attribute__((aligned(64)))
struct Both @packed @align(16) { u8 x; i64 y }
```

### No `this`, No Name Shadowing

Field names are resolved directly inside methods. No `this->hp` needed. Name shadowing (parameter or local with same name as field) is not allowed.

---

## 10. Unions

### Named Union

```brick
union Data {
    int i
    float f
    bool b
}

fn main() {
    Data d
    d.i = 42     // set integer
    d.f = 3.14   // overwrites same memory as float
}
```

### Anonymous Union inside Struct

```brick
struct Packet {
    int id
    union {
        int x
        float y
    }             // x and y overlap in memory
}

fn main() {
    Packet p
    p.id = 1
    p.x = 99     // access union field directly
}
```

### Anonymous Struct inside Union

```brick
union Data {
    u32 raw
    struct { u8 low; u8 high }
}

fn main() {
    Data d
    d.raw = 0x0A0B
    u8 l = d.low    // 0x0B
    u8 h = d.high   // 0x0A
}
```

---

## 11. Enums

```brick
enum Color {
    RED          // = 0 (auto-increment)
    GREEN        // = 1
    BLUE         // = 2
}

enum TextureFlags {
    CLAMP_U = 0x01    ← hex value
    CLAMP_V = 0x02
    FILTER  = 0x04
}

fn main() {
    Color c = GREEN
    if c == GREEN { print("green") }

    // Bitwise enum usage
    u32 flags = CLAMP_U | CLAMP_V
    if flags & CLAMP_U { print("clamped") }
}
```

Enums generate `#define` constants in C. Variants are global constants.

---

## 12. Functions

### Basic Functions

```brick
fn main() { }                            ← entry point (returns void)

fn add(int a, int b) -> int {            ← returns int
    return a + b
}

fn log(String msg) {                     ← void function
    // ...
}
```

### export fn — C Visibility

```brick
export fn calculate(int x) -> int {
    return x * 2
}
```

| Declaration | Generated C |
|------------|-------------|
| `fn calc()` | `static inline int32_t calc()` |
| `export fn calc()` | `int32_t calc()` (visible to linker) |

### Function Pointers

```brick
fn add(int a, int b) -> int {
    return a + b
}

fn main() {
    fn(int, int)->int op    ← declare function pointer
    op = add                ← assign function
    int r = op(3, 4)        ← call through pointer
    print(r)                // 7
}
```

Syntax: `fn(param_types)->return_type var_name`

### Default Parameters

```brick
fn move(int x, int y, int speed = 1) {
    // speed has default value 1
}

fn main() {
    move(10, 20)        // uses speed = 1
    move(10, 20, 5)     // uses speed = 5
}
```

---

## 13. Control Flow

### If/Else

```brick
if hp <= 0 {
    print("dead")
} else {
    print("alive")
}

if x > 0 {
    print("positive")
} else if x < 0 {
    print("negative")
} else {
    print("zero")
}
```

### While

```brick
while hp > 0 {
    apply_damage(10)
}
```

### For (C-Style)

```brick
for int i = 0; i < 10; i++ {
    print(i)
}
```

### For x in N (Range)

```brick
fn sum_to(int N) -> int {
    int total = 0
    for x in N {
        total = total + x
    }
    return total      // sum_to(5) = 0+1+2+3+4 = 10
}
```

The compiler generates a loop with `__i` from 0 to N-1.

### Break / Continue

```brick
while true {
    if done { break }
    if skip { continue }
}
```

### Return

```brick
fn add(int a, int b) -> int {
    return a + b
}

fn log(String msg) {
    // no return needed for void
}
```

---

## 14. Pointers and Pointer Arithmetic

### Basic Pointer Operations

```brick
// Declaration: *T means pointer to T
*int p = &x             // &x = address of x
int v = *p              // *p = dereference

// Pointer arithmetic (C semantics — scales by sizeof(T))
p = p + 1               // advance 1 element
p += 2
p -= 1

// Pointer difference
isize diff = q - p      // number of elements between addresses

// Pointer indexing
int v = p[0]            // *(p + 0)

// Pointer comparison
bool eq = p == q
bool lt = p < q
bool null_check = p != null

// Increment / Decrement
++p                     // p += 1
p--                     // p -= 1
```

### Rules

- `ptr + int` / `ptr - int` → element count (not bytes)
- `ptr - ptr` → `isize` (elements between)
- `*T + *T` is error (only subtraction)
- `*T + float` is error (offset must be integer)
- `&literal` is error (only variables have addresses)
- `p[N]` works on any `*T` like C

### Null Pointer

```brick
*int p = null
if p != null { }
```

---

## 15. Operators

### Arithmetic

| Operator | Description |
|:--------:|-------------|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |
| `++` | Increment |
| `--` | Decrement |

### Comparison

| Operator | Description |
|:--------:|-------------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |

### Logical

| Operator | Description |
|:--------:|-------------|
| `&&` | Logical AND |
| `\|\|` | Logical OR |
| `!` | Logical NOT |
| `and` | Logical AND (keyword) |
| `or` | Logical OR (keyword) |
| `not` | Logical NOT (keyword) |

### Bitwise

| Operator | Description |
|:--------:|-------------|
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT |
| `<<` | Left shift |
| `>>` | Right shift |

### Assignment

| Operator | Description |
|:--------:|-------------|
| `=` | Assign |
| `+=` | Add and assign |
| `-=` | Subtract and assign |
| `*=` | Multiply and assign |
| `/=` | Divide and assign |

### Other

| Operator | Description |
|:--------:|-------------|
| `.` | Access field or method |
| `()` | Function/method call |
| `[]` | Array/pointer index |
| `@` | Allocate in specific block |
| `->` | Return type annotation |

---

## 16. Strings

```brick
String s = "hello"                   ← creates a String
String name = "Felipe" @game         ← String in a specific block
String empty = ""                    ← empty string
String greeting = "Hello, " + "world!"  ← compile-time concatenation
```

- `String` is a built-in type with `.data` (char pointer) and `.len` (length)
- Strings are block-allocated
- Escape sequences: `\n` (newline), `\t` (tab), `\\` (backslash), `\"` (quote)
- When passed to `*u8` C parameters, `.data` is passed automatically
- Concatenation uses `+` (compile-time only for string literals; runtime concatenation via append not yet supported)

---

## 17. Arrays

### Fixed Arrays

```brick
int[10] arr                          ← fixed array of 10 integers
int[5] vals = {1, 2, 3, 4, 5}       ← with brace initializer
u8[4] bytes = {0xFF, 0x00, 0xAA, 0x55}
f32[4] m = {1.0, 0.0, 0.0, 1.0}     ← matrix init
f32[4][4] matrix                    ← 2D array
```

Array size is fixed at declaration (compile-time constant). Brace initializer sets all elements.

### Dynamic Arrays (T[])

```brick
struct Container {
    int[] items     ← dynamic array (pointer + count + capacity)
}
```

`T[]` as a struct field generates 3 C fields: `T* items; int64_t items_cnt; int64_t items_cap;`

Built-in properties:
- `.len` → current element count (`items_cnt`)
- `.cap` → allocated capacity (`items_cap`)
- `.append(val)` → append an element (auto-grows)

```brick
struct Inventory {
    int[] items
}

fn main() {
    Inventory inv @global
    inv.items.append(10)
    inv.items.append(20)
    print(inv.items.len)     // 2
    print(inv.items.cap)     // 4 (auto-grown)
}
```

### Array Literals in Expressions

```brick
fn sum_3(*i32 a) -> i32 { return a[0] + a[1] + a[2] }

fn main() {
    i32 r = sum_3({10, 20, 30})   // C99 compound literal
}
```

---

## 18. Match Statements

```brick
match value {
    1 { print("one") }
    2, 3 { print("two or three") }     ← multi-pattern
    _ { print("other") }               ← wildcard (default)
}
```

### Match with Guards

```brick
fn test_guard() -> int {
    int val = 5
    int out = 0
    match val {
        5 if val > 3 { out = 1 }       ← guard condition
        5 { out = 2 }                  ← fallback
        _ { out = 3 }
    }
    return out
}
```

Compiles to C `switch` with `if` guards inside cases.

---

## 19. Defer Statements

```brick
fn main() {
    defer { print("cleanup") }
    print("doing work")
    // "cleanup" is called when scope exits
}
```

Deferred bodies execute in LIFO order (last deferred, first executed). They run when the enclosing scope exits, including before `return`.

```brick
fn test_multi() -> int {
    defer { print("first defer") }
    defer { print("second defer") }
    return 42
    // Output: "second defer" then "first defer" then returns 42
}
```

---

## 20. Error Handling

```brick
error("something went wrong")    ← prints message and aborts (panic)
```

No try/catch — fail fast with `error()`. Runtime uses `fprintf(stderr, ...); exit(1)`.

---

## 21. sizeof and alignof

```brick
i64 s = int.sizeof         // 4
s = f64.sizeof             // 8
s = MyStruct.sizeof        // sum of fields + padding
s = my_var.sizeof          // size of variable's type

i64 a = f32.alignof        // 4 (in most platforms)
a = f64.alignof            // 8
a = MyStruct.alignof       // struct alignment
```

Both are compile-time evaluated. Generate `sizeof(T)` / `_Alignof(T)` in C.

---

## 22. Visibility

```brick
public int x               ← visible everywhere (default)
private int y              ← visible only within the package
```

`public` is default. `private` restricts visibility to the current package.

Applicable to: structs, functions, consts, enums, unions, interfaces, type aliases, macros, and struct fields.

---

## 23. I/O (Package IO)

```brick
using IO

fn main() {
    print(42)                    // "42\n"
    print(3.14)                  // "3.140000\n"
    print(true)                  // "true\n"
    print('a')                   // "a\n"
    print("hello")               // "hello\n"
    print()                      // "\n"
    print("x = {0}", 10)         // "x = 10\n"
    print("{0} + {1} = {2}", 1, 2, 3)  // "1 + 2 = 3\n"
}
```

- `using IO` is required
- `print()` always adds `\n` (println semantics)
- Supported: all numeric types, bool, char, String
- Formatting uses `{0}`, `{1}`, etc.

---

## 24. Macros

### Basic Macro

```brick
macro swap(a, b) {
    __tmp = $a
    $a = $b
    $b = __tmp
}

fn main() {
    x = 10; y = 20
    swap(x, y)            // expands to the swap body
    print("{0} {1}", x, y) // "20 10"
}
```

### Explicit $macro() Syntax

```brick
$twice(10)   // explicit $ call — same as twice(10)
```

Both syntaxes work:
| Syntax | Example |
|--------|---------|
| Implicit | `twice(10)` |
| Explicit | `$twice(10)` |

### Declaration

```brick
macro name(param1, param2, ...) {
    // body — any Brick code with $ interpolation
}
```

- Parameters are **untyped** (hold expressions, not values)
- `$name` inserts the argument
- `$(expr)` evaluates at compile time
- `__`-prefixed names get unique gensyms (hygiene)
- `values...` captures remaining arguments as a list (varargs)

### build {} — Compile-Time Computation

```brick
build {
    x = 42
    emit { z = x }        // generates: z = 42
}
```

`build` runs at compile time. Variables don't exist in the final binary.

### emit {} — Code Generation

```brick
macro vec2_add(name) {
    emit {
        fn $name(x1, y1, x2, y2, out_x, out_y) {
            out_x = x1 + x2
            out_y = y1 + y2
        }
    }
}

vec2_add(add_positions)
```

### Hygiene

Variables starting with `__` inside a macro get unique names (e.g., `__tmp` → `__tmp__1`), preventing collisions with user code.

### Varargs

```brick
macro print_all(values...) {
    $values[0]    // first value
    $values[1]    // second value
}
```

### Type Reflection (inside build)

| Expression | Returns | Example |
|-----------|---------|---------|
| `T.name` | Type name as string | `"i32"` |
| `T.size` | Size in bytes | `4` |
| `T.fields` | Field names as strings | `["x", "y"]` |

### Error Handling

| Situation | What happens |
|-----------|-------------|
| Wrong argument count | Compile error |
| `$` outside macro/build | Compile error |
| Recursive macro | Caught after 64 levels |
| I/O inside `build` | Not allowed |

---

## 25. C Interop

### Include C Headers

```brick
include "my_header.h"         // #include "my_header.h"
include "stdio.h" @system     // #include <stdio.h>
include "math.h" and link m   // include + link together

// Or separately:
include "SDL.h"
link SDL2
```

- Without `@system`: `#include "header.h"` (local)
- With `@system`: `#include <header.h>` (system)

### External C Functions

```brick
extern fn sqrt(f64 x) -> f64
extern fn atoi(*u8 str) -> i32
extern fn puts(*u8 s) -> i32
extern fn sin(f64 x) -> f64
extern fn cos(f64 x) -> f64
extern fn pow(f64 b, f64 exp) -> f64
```

- `String` → `*u8` conversion happens automatically
- Pointer types: `*u8` → `char*`, `*void` → `void*`, `*MyStruct` → `MyStruct*`

### Generate C Bindings

```bash
brick bind <header.h>    # generates .brc bindings
```

### Calling from C

```c
// In your C code:
int32_t result = calculate(42);  // calls Brick's export fn
```

---

## 26. Packages

### Declaration

```brick
package SPRITES                    ← declare package
package SPRITES.EFFECTS            ← hierarchical sub-package
```

### Import

```brick
using SPRITES                      ← import everything from package
using SPRITES.EFFECTS              ← import nested sub-package
```

- `using IO` imports the built-in I/O package (for `print()`)

### export / private

```brick
export fn add(int a, int b) -> int { return a + b }
export const PI = 31415
export struct Vec2 { int x; int y }
export enum Color { RED; GREEN; BLUE }
export interface Drawable { fn draw() }

private fn internal_helper() -> int { return 999 }
private const SECRET = 42
```

- Everything is `public` by default
- `export` explicitly marks as public (optional — same as default)
- `private` restricts to current package

### Multi-File Projects

```bash
# Explicit file list
brick build main.brc lib.brc -o program

# Auto-resolution from filesystem
brick build main.brc -I ./packages -o program
```

The compiler auto-resolves `using PACKAGE` by searching:
1. Current directory: `<PACKAGE>.brc`
2. `-I <dir>` directories: `<dir>/<PACKAGE>.brc`
3. `<dir>/<PACKAGE>/<PACKAGE>.brc`
4. `<dir>/<PACKAGE>/main.brc`
5. `BRICK_PATH` env var directories

### Nested Packages

```brick
// File: lib/MATH/VEC2.brc
package MATH.VEC2
export struct Vec2 { f64 x; f64 y }
export fn length(Vec2 v) -> f64 { return v.x * v.x + v.y * v.y }
```

```brick
// File: main.brc
using MATH.VEC2

fn main() {
    Vec2 p = {x = 3.0, y = 4.0}
    f64 len = length(p)
    print("length = {0}", len)  // 25.000000
}
```

```bash
brick build main.brc -I lib -o demo
```

### Visibility Rules

| Declaration | Access |
|------------|--------|
| `public` (default) | Any file that does `using PACKAGE` |
| `private` | Only within the same package file |
| `export fn` | Public + visible to C linker |

---

## 27. Compilation

```bash
# Step 1: Brick compiler produces C code
brick input.brc -o output.c

# Step 2: gcc compiles C + runtime into a binary
gcc -O3 output.c runtime/block_memory.c runtime/io.c -o program -ldl

# Debug build (with GDB support)
gcc -g output.c runtime/block_memory.c runtime/hot_reload.c runtime/io.c -o program -ldl

# One-step build (handles everything)
brick build hello.brc -o hello --release

# Build and run in one step
brick run hello.brc

# Emit C only (no linking)
brick build hello.brc --emit-c-only -o hello.c
```

## 28. Compiler Optimizations

### Constant Folding

Integer and float constant expressions computed at compile time:
```brick
int x = 10 + 20   // generates: int32_t x = 30;
int y = 5 * 7     // generates: int32_t y = 35;
```

### Inline Hints

`__attribute__((always_inline))` + `static inline` on every non-main, non-extern function. Zero call overhead for small functions.

### SIMD Alignment

`__attribute__((aligned(N)))` on float/f64 fields and arrays:
```brick
struct Particles {
    f32[4] positions     // aligned(16) for SSE
    f64[2] velocities    // aligned(32) for AVX
}
```

### Pool Allocator

Types ≤ 64 bytes use `pool_alloc()` instead of `block_alloc()` — O(1) free.

### TLS Blocks

`__thread BlockCtx*` — zero-contention allocations across threads.

### Profile-Guided Optimization (PGO)

```bash
scons profile=pgo-gen   # instrument
./my_game --benchmark   # collect profile
scons profile=pgo-use   # optimize
```

---

## 29. Reserved Keywords

```
package   using     public    private   struct
extends   interface fn        return    if
else      while     for       block     reset
true      false     null      error     int
float     bool      char      String    void
macro     build     emit      export    include
link      extern    const     enum      match
defer     union     type      impl      and
or        not       is        as
```

---

## 30. Complete Example

```brick
package GAME

using IO

block global = 256MB
block game = 64MB
block temp = 8MB

interface Drawable {
    fn draw()
}

interface Damageable {
    fn take_damage(int d)
}

struct Player : Drawable, Damageable {
    i32 hp
    String name
    int[] items

    fn Player(i32 h, String n) {
        hp = h
        name = n
    }

    fn damage(i32 dmg) {
        hp -= dmg
        if hp < 0 { hp = 0 }
        print("{0} took {1} damage, hp={2}", name, dmg, hp)
    }

    fn draw() {
        print("Player: {0} hp={1}", name, hp)
    }
}

fn main() {
    Player p = Player(100, "Felipe") @game

    // Match statement
    match p.hp {
        0 { print("dead") }
        1, 2, 3 { print("low hp") }
        _ { print("ok") }
    }

    // Defer
    defer { print("cleaning up") }

    // Dynamic array
    p.items.append(42)
    print("items: {0}", p.items.len)

    // Polymorphism via interface
    Player p2 = Player(200, "Enemy") @game
    Damageable d = p2
    d.take_damage(50)

    // Pointers
    *int ptr = &p.hp
    int val = *ptr

    // Constants
    const MAX_ENEMIES = 10

    // Type alias
    type HpType = i32

    // Sizeof
    i64 sz = HpType.sizeof

    game.reset()
    global.reset()
}
```

---

## See Also

- [Getting Started](GETTING_STARTED.md) — Install and first project
- [Macros](MACROS.md) — Full macro documentation with examples
- [Architecture](ARCHITECTURE.md) — Compiler pipeline
- [Hot Reload](hot-reload.md) — Live code swapping
- [Optimizations](OPTIMIZATIONS.md) — Performance details
