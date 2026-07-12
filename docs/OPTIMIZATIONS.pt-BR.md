# Otimizações do Brick

## Otimizações do Compilador

### Constant Folding

Aritmética em constantes de compilação é avaliada em tempo de compilação:

```brick
const X = 10 + 20     // dobrado para const X = 30
int y = 5 * 7         // gera: int32_t y = 35
```

### Inline Hints

Toda `fn` (exceto `export fn` e `extern fn`) é gerada como:

```c
__attribute__((always_inline)) static inline tipo_retorno nome_func(params);
```

### Alinhamento SIMD

```c
struct Particles {
    float positions[4] __attribute__((aligned(16)));   // SSE
    double velocities[2] __attribute__((aligned(32)));  // AVX
};
```

## Otimizações do Runtime

### Block Allocator (~3 ciclos)

```c
static inline void* block_alloc(BlockCtx* ctx, int64_t size) {
    void* ptr = ctx->ptr;
    ctx->ptr += size;
    if (ctx->ptr > ctx->end) error("block overflow");
    return ptr;
}
```

### Pool Allocator (O(1))

Para tipos ≤ 64 bytes, o pool allocator fornece alocação e liberação O(1).

| Alocador | Alocação | Free | Reset | Fragmentação |
|----------|----------|------|-------|--------------|
| Block (bump) | ~3 ciclos | N/A | ~5 ns | Nenhuma |
| Pool | O(1) | O(1) | O(1) | Interna apenas |
| malloc | ~100 ciclos | ~50 ciclos | N/A | Sim |

### TLS Blocks

```c
__thread BlockCtx* block_tls;
block_set_tls(ctx);
```

### Double-Buffer

```c
block_enable_double_buffer(ctx);
block_swap_buffers(ctx);  // swap atômico (~1 ciclo)
```

## Benchmarks

| Operação | Brick | C Puro | Razão |
|----------|-------|--------|-------|
| Soma int | 1 ciclo | 1 ciclo | 1.0× |
| Chamada função (inline) | 0 ciclos | 0 ciclos | 1.0× |
| Acesso campo struct | 1 ciclo | 1 ciclo | 1.0× |
| Block alloc | 3 ciclos | n/a | — |
| Dispatch vtbl | 2-3 ciclos | igual | 1.0× |

## Veja Também

- [Referência da Linguagem](LANGUAGE.pt-BR.md) — Referência completa
- [Arquitetura](ARCHITECTURE.pt-BR.md) — Pipeline do compilador
