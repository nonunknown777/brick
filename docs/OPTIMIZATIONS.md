# Brick Optimizations

## Compiler Optimizations

### Constant Folding

Arithmetic on compile-time constants is evaluated at compile time:

```brick
const X = 10 + 20     // folded to const X = 30
int y = 5 * 7         // generates: int32_t y = 35
```

Supports integer and float operations. Prevents runtime computation of known values.

### Inline Hints

Every `fn` (except `export fn` and `extern fn`) is generated as:

```c
__attribute__((always_inline)) static inline return_type func_name(params);
```

This gives the C compiler maximum flexibility to inline functions, eliminating call overhead for small functions.

### SIMD Alignment

The compiler generates `__attribute__((aligned(N)))` for float/f64 fields and arrays:

```c
struct Particles {
    float positions[4] __attribute__((aligned(16)));   // SSE
    double velocities[2] __attribute__((aligned(32)));  // AVX
};
```

- `f32[4]` → aligned(16) for SSE/NEON
- `f64[2]` → aligned(32) for AVX
- `f32` fields → aligned(16) if adjacent to other floats
- Enables auto-vectorization in `gcc -O3`

### Dead Code Elimination

The compiler flags unreachable code:

```brick
fn test() -> int {
    return 42
    error("unreachable")    // compile warning
}
```

The C compiler further eliminates dead code at `-O3`.

## Runtime Optimizations

### Block Allocator (~3 cycles)

The bump allocator is a pointer bump + size check:

```c
static inline void* block_alloc(BlockCtx* ctx, int64_t size) {
    void* ptr = ctx->ptr;
    ctx->ptr += size;
    if (ctx->ptr > ctx->end) {
        error("block overflow");
    }
    return ptr;
}
```

- **No free list** — just advance pointer
- **No fragmentation** — reset releases everything
- **~3 CPU cycles** per allocation
- **~5 ns** for full block reset (O(1))

### Pool Allocator (O(1) free)

For types ≤ 64 bytes, the pool allocator provides:

```c
void* pool_alloc(PoolAllocator* pool, int64_t size);
void  pool_free(PoolAllocator* pool, void* ptr);
```

- O(1) allocation and free
- Configurable slots (up to 8)
- Ideal for small, frequently allocated types (particles, vectors, small structs)

| Allocator | Allocation | Free | Reset | Fragmentation |
|-----------|-----------|------|-------|---------------|
| Block (bump) | ~3 cycles | N/A | ~5 ns | None |
| Pool | O(1) | O(1) | O(1) | Internal only |
| malloc | ~100 cycles | ~50 cycles | N/A | Yes |

### Thread-Local Storage (TLS) Blocks

```c
__thread BlockCtx* block_tls;
block_set_tls(ctx);     // thread-local block
void* ptr = block_alloc(block_get_tls(), size);
```

- Zero-contention allocations across threads
- No mutex, no atomic ops
- Each thread has its own bump allocator

### Double-Buffer Hot Reload

```c
block_enable_double_buffer(ctx);
// ... allocate in active buffer ...
block_swap_buffers(ctx);  // atomic pointer swap (~1 cycle)
```

- Zero pause — swap is an atomic pointer exchange
- Old buffer is reclaimed incrementally
- New allocations immediately use the new buffer

## Code Generation Optimizations

### #line Directives

Every line of generated C includes a `#line` directive pointing to the `.brc` source:

```c
#line 1 "game.brc"
#include <stdint.h>
#include <stddef.h>
#line 5 "game.brc"
int32_t main() {
    // ...
```

This allows:
- GDB debugging in `.brc` files
- `gcc -g` preserves source mapping
- Stack traces reference original source

### Type Width Optimization

- `i1`–`i8` → `int8_t` (no wasted bits for non-bitfield)
- `u1`–`u8` → `uint8_t`
- `bool` → `uint8_t` with explicit 0/1
- `usize` → `size_t` (matches pointer width)

### String Representation

`BrickString` is a contiguous struct:

```c
typedef struct {
    char* data;
    int64_t len;
} BrickString;
```

- No null terminator needed (but added for C interop)
- `len` enables fast length checks
- Passes by value (2 registers) — no heap allocation

### Interface Dispatch (vtbl)

```c
typedef struct {
    void (*draw)(void*);
} DrawableVtbl;

typedef struct {
    void* data;
    const DrawableVtbl* vtbl;
} Drawable;
```

- Static vtbl instances (no per-object overhead)
- Single vpointer per interface reference
- Dispatch: load vtbl → call through pointer → ~2-3 cycles

## Build Optimization

### Profile-Guided Optimization (PGO)

```bash
scons profile=pgo-gen    # build instrumented binary
./program --benchmark    # collect profile data
scons profile=pgo-use    # rebuild with profile
```

- First pass: instrumented with `-fprofile-generate`
- Run: collects branch/loop frequency
- Second pass: recompile with `-fprofile-use`
- Improves branch prediction, inlining decisions

### Release Build

```bash
scons profile=release    # default: -O3 -march=native -flto
```

- `-O3`: full optimization
- `-march=native`: CPU-specific instructions
- `-flto`: link-time optimization (cross-module inlining)
- `-fomit-frame-pointer` (when debug off)
- Strips debug symbols

## Performance Benchmarks

| Operation | Brick | Plain C | Ratio |
|-----------|-------|---------|-------|
| int add | 1 cycle | 1 cycle | 1.0× |
| function call (inline) | 0 cycles | 0 cycles | 1.0× |
| struct field access | 1 cycle | 1 cycle | 1.0× |
| block alloc | 3 cycles | n/a | — |
| array index | 1 cycle | 1 cycle | 1.0× |
| match/switch | same as C | same | 1.0× |
| vtbl dispatch | 2-3 cycles | same | 1.0× |

## See Also

- [Language Reference](LANGUAGE.md) — Complete syntax reference
- [Architecture](ARCHITECTURE.md) — Compiler pipeline details
