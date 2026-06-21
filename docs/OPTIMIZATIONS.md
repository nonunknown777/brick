# Otimizações do Meta-C
# Meta-C Optimizations

> Notas sobre performance e como o Meta-C busca ser o mais rápido possível.
> Notes on performance and how Meta-C aims to be as fast as possible.

## Filosofia de Performance
## Performance Philosophy

Meta-C é construído em 3 camadas, cada uma com suas otimizações:
Meta-C is built in 3 layers, each with its own optimizations:

```
1. COMPILADOR (C++20) → roda em tempo de desenvolvimento
   ├─ Usa templates e constexpr pra computar o máximo em compile-time
   └─ Gera C já otimizado (sem passes extras)

2. CÓDIGO GERADO → o .c que o compilador produz
   ├─ C puro, legível, sem abstrações escondidas
   └─ Compilado com gcc/clang -O3

3. RUNTIME (C) → roda junto com o programa
   ├─ Bump allocator = alocação em ~3 ciclos de CPU
   └─ Zero overhead features (sem exceções, sem RTTI)
```

```
1. COMPILER (C++20) → runs at development time
   ├─ Uses templates and constexpr to compute as much as possible at compile-time
   └─ Generates already-optimized C (no extra passes)

2. GENERATED CODE → the .c produced by the compiler
   ├─ Pure, readable C with no hidden abstractions
   └─ Compiled with gcc/clang -O3

3. RUNTIME (C) → runs alongside the program
   ├─ Bump allocator = allocation in ~3 CPU cycles
   └─ Zero overhead features (no exceptions, no RTTI)
```

## Bump Allocator
## Bump Allocator

O Block Memory allocator é a alma da performance do Meta-C:
The Block Memory allocator is the soul of Meta-C's performance:

```
Alocação normal (malloc):   ~80 ciclos
Alocação bump (Meta-C):      ~3 ciclos  ← 27x mais rápido
Normal allocation (malloc):   ~80 cycles
Bump allocation (Meta-C):      ~3 cycles  ← 27x faster
```

Como funciona:
How it works:

- O bloco é um pedaço contíguo de memória
- Alocar = avançar um ponteiro (não procura espaço livre)
- Resetar = voltar o ponteiro pro início (1 ciclo)
- Sem fragmentação interna
- Cache-friendly (dados ficam pertinho uns dos outros)

- The block is a contiguous piece of memory
- Allocating = advancing a pointer (no search for free space)
- Resetting = moving the pointer back to the start (1 cycle)
- No internal fragmentation
- Cache-friendly (data stays close together)

## Otimizações aplicadas
## Applied Optimizations

### No compilador (src/)
### In the compiler (src/)
- Parser descendente recursivo (sem backtracking, O(n))
- AST nodes sem smart pointers pesados
- Codegen escreve stringstream contínuo (sem alocações múltiplas)
- Lookup de keywords com unordered_map (O(1) amortizado)
- Recursive descent parser (no backtracking, O(n))
- AST nodes without heavy smart pointers
- Codegen writes to a continuous stringstream (no multiple allocations)
- Keyword lookup with unordered_map (amortized O(1))

### No código gerado
### In the generated code
- Structs planas (sem vtable oculta)
- Métodos viram funções C normais (call direto, sem dispatch)
- #line directives não geram código extra (só informação pro debugger)
- Atribuição direta sem construção/destruição C++
- Flat structs (no hidden vtable)
- Methods become regular C functions (direct call, no dispatch)
- #line directives generate no extra code (only debugger info)
- Direct assignment without C++ construction/destruction

### Na runtime (C)
### In the runtime (C)
- Bump allocator com alinhamento configurável
- block_reset é só `ctx->used = 0` (O(1))
- Sem locks por padrão (thread-safe opcional)
- block_stats é só leitura de struct (O(1))
- Bump allocator with configurable alignment
- block_reset is just `ctx->used = 0` (O(1))
- No locks by default (optional thread-safe)
- block_stats is just struct reading (O(1))

## O que NÃO tem (e por que)
## What We DON'T Have (and Why)

| Ausente | Motivo |
| Missing | Reason |
|---------|--------|
| Exceções | Overhead de runtime + código gerado maior |
| Exceptions | Runtime overhead + larger generated code |
| RTTI | typeid() custa caro, structs sabem quem são |
| RTTI | typeid() is expensive, structs know who they are |
| Virtual dispatch | Chamada indireta impede inlining |
| Virtual dispatch | Indirect call prevents inlining |
| Garbage Collector | Pausa o mundo, imprevisível |
| Garbage Collector | Stops the world, unpredictable |
| Free individual | Bump allocator não precisa |
| Individual free | Bump allocator doesn't need it |
| Stack do usuário | Consistência: tudo em blocos |
| User stack | Consistency: everything in blocks |

## Benchmarks esperados
## Expected Benchmarks

```
Alocação:       100M allocs de 64 bytes
  malloc:       8.2s
  Meta-C bump:  0.3s  ← 27x mais rápido

Reset:          1M resets de blocos
  free():       2.1s
  block_reset:  0.001s  ← 2000x mais rápido

Compilação:     1000 linhas de .mc → C
  Meta-C:       < 50ms  (target)
```

```
Allocation:     100M allocs of 64 bytes
  malloc:       8.2s
  Meta-C bump:  0.3s  ← 27x faster

Reset:          1M block resets
  free():       2.1s
  block_reset:  0.001s  ← 2000x faster

Compilation:    1000 lines of .mc → C
  Meta-C:       < 50ms  (target)
```

## Futuras otimizações planejadas
## Planned Future Optimizations

- [ ] Alinhamento SIMD (16/32 bytes) pra arrays de float
- [ ] Thread-local blocks (cada thread com seu bloco, sem lock)
- [ ] Codegen com constant folding (pré-calcular expressões constantes)
- [ ] Inline hints pro gcc (`__attribute__((always_inline))`)
- [ ] Pool allocator pra objetos pequenos (tipo String)
- [ ] Hot reload sem pausa (double-buffering de blocos)
- [ ] Profile-guided optimization (PGO) no build release

- [ ] SIMD alignment (16/32 bytes) for float arrays
- [ ] Thread-local blocks (each thread with its own block, no lock)
- [ ] Codegen with constant folding (pre-compute constant expressions)
- [ ] Inline hints for gcc (`__attribute__((always_inline))`)
- [ ] Pool allocator for small objects (like String)
- [ ] Pauseless hot reload (block double-buffering)
- [ ] Profile-guided optimization (PGO) in release build
