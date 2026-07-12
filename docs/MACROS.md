# Brick Macro System

Macros provide compile-time code generation. They are hygienic, typed in output only, and integrate with `build {}` and `emit {}` for powerful metaprogramming.

## Basic Macro

```brick
macro swap(a, b) {
    __tmp = $a
    $a = $b
    $b = __tmp
}

fn main() {
    x = 10; y = 20
    swap(x, y)             // expands body with x→$a, y→$b
    print("{0} {1}", x, y) // "20 10"
}
```

The `__tmp` variable inside the macro gets a **unique name** (`__tmp__1`) to prevent collisions with user variables (hygiene).

## Macro Definition

```brick
macro name(param1, param2, ...) {
    // body — any Brick code with $ interpolation
}
```

- Parameters are **untyped** — they hold expressions (not evaluated values)
- The body is parsed as regular Brick code
- `$name` inserts the argument as-is
- `__`-prefixed names get unique gensyms

## $ Interpolation

Use `$param` to insert a parameter into the generated code:

```brick
macro assert_eq(a, b) {
    if $a != $b {
        error("assertion failed")
    }
}

fn main() {
    assert_eq(3 + 4, 7)   // expands to: if 3 + 4 != 7 { error(...) }
}
```

### $macro() Explicit Syntax

Macros can be called with explicit `$` syntax:

```brick
$swap(x, y)          // same as swap(x, y)
$assert_eq(5, 5)     // same as assert_eq(5, 5)
```

Both syntaxes are equivalent. The `$macro()` form makes macro calls visually distinct from function calls.

## Hygiene (Gensym)

Variables starting with `__` inside macros get unique identifiers:

```brick
macro twice(x) {
    __result = $x * 2     // __result → __result__1
}

fn main() {
    __result = 0          // user's __result (NOT the macro's)
    twice(5)              // uses __result__1
    print("{0}", __result)// "0" — no collision!
}
```

Without hygiene, nested or repeated macro calls would shadow variables.

## Varargs (args...)

```brick
macro print_all(values...) {
    $values[0]    // first argument
    $values[1]    // second argument
}

fn main() {
    print_all(10, 20, 30)  // expands to:
                           // 10
                           // 20
                           // (30 is unused)
}
```

Varargs capture zero or more arguments into a list-like parameter.

## build {} — Compile-Time Computation

`build {}` blocks execute at compile time. Variables inside are temporary and don't exist in the final binary.

```brick
build {
    x = 42                          // temporary variable
    emit { z = x }                  // generates: z = 42
}
```

Inside `build`:
- All regular operations (arithmetic, assignment, conditionals)
- `emit { ... }` to generate code
- Type reflection: `T.name`, `T.size`, `T.fields`
- I/O is NOT allowed (prevents side effects)

## emit {} — Code Generation

`emit {}` generates code into the surrounding scope:

```brick
macro vec2_op(name, op) {
    emit {
        fn $name(x1, y1, x2, y2, out_x, out_y) {
            $op(x1, x2, out_x)
            $op(y1, y2, out_y)
        }
    }
}

macro add_fields(a, b, out) {
    emit {
        $out = $a + $b
    }
}

vec2_op(add_vec2, add_fields)

fn main() {
    add_vec2(1.0, 2.0, 3.0, 4.0, &rx, &ry)
}
```

## build + emit — Combined

```brick
macro create_struct(name, fields...) {
    build {
        __fields = $fields
        __body = "    i32 " ++ __fields[0]  // build-time string ops
        emit {
            struct $name {
                $fields[0]    // First field
                $fields[1]    // Second field
            }
        }
    }
}

create_struct(Point, x, y)
// Generates:
// struct Point {
//     i32 x
//     i32 y
// }
```

The `build` block processes `$fields` at compile time, then `emit` generates the struct declaration.

## Type Reflection (inside build)

| Expression | Returns | Example Output |
|-----------|---------|---------------|
| `T.name` | Type name as string | `"i32"` |
| `T.size` | Size in bytes | `4` |
| `T.fields` | Field names as string array | `["x", "y"]` |

```brick
macro inspect(T) {
    build {
        name = T.name
        size = T.size
        emit {
            print("type: {0}, size: {1}", name, size)
        }
    }
}

inspect(MyStruct)
// Generates: print("type: {0}, size: {1}", "MyStruct", 16)
```

## Macro Rules

| Situation | Behavior |
|-----------|----------|
| Wrong argument count | Compile error |
| `$` outside macro/build | Compile error |
| Recursive macro > 64 levels | Caught, compile error |
| I/O inside `build` | Not allowed (compile error) |
| `$` in non-macro code | Compile error |
| `$(expr)` | Compile-time eval of expr |

## Patterns

### Generic Data Structure

```brick
macro create_list(T) {
    emit {
        struct List_$T {
            $T[] items
        }

        fn List_$T.push(self, $T value) {
            self.items.append(value)
        }

        fn List_$T.get(self, i32 index) -> $T {
            return self.items[index]
        }
    }
}

create_list(int)

fn main() {
    List_int my_list @global
    my_list.push(42)
    int val = my_list.get(0)
}
```

### Performance Wrapper

```brick
macro measure(name, body) {
    build {
        __start = clock()   // build-time simulation
        $body
        __elapsed = clock() - __start
        emit {
            print("'{0}' took {1} cycles", name, __elapsed)
        }
    }
}

macro dbg(expr) {
    emit {
        print("debug: {0} = ", $expr)
        print($expr)
    }
}

fn main() {
    dbg(3 + 5)    // prints: debug: 3 + 5 = 8
}
```

## See Also

- [Language Reference](LANGUAGE.md#24-macros) — Macro syntax in detail
- [Architecture](ARCHITECTURE.md) — How macro expansion fits in the pipeline
