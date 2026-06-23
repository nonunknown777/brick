# Otimizações do Brick

> Notas sobre performance e como o Brick busca ser o mais rápido possível.

## Filosofia de Performance

Brick é construído em 3 camadas, cada uma com suas otimizações:

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

## Bump Allocator

O Block Memory allocator é a alma da performance do Brick:

```
Alocação normal (malloc):   ~80 ciclos
Alocação bump (Brick):      ~3 ciclos  ← 27x mais rápido
```

Como funciona:

- O bloco é um pedaço contíguo de memória
- Alocar = avançar um ponteiro (não procura espaço livre)
- Resetar = voltar o ponteiro pro início (1 ciclo)
- Sem fragmentação interna
- Cache-friendly (dados ficam pertinho uns dos outros)

## Otimizações aplicadas

### No compilador (src/)
- Parser descendente recursivo (sem backtracking, O(n))
- AST nodes sem smart pointers pesados
- Codegen escreve stringstream contínuo (sem alocações múltiplas)
- Lookup de keywords com unordered_map (O(1) amortizado)

### No código gerado
- Structs planas (sem vtable oculta)
- Métodos viram funções C normais (call direto, sem dispatch)
- #line directives não geram código extra (só informação pro debugger)
- Atribuição direta sem construção/destruição C++

### Na runtime (C)
- Bump allocator com alinhamento configurável
- block_reset é só `ctx->used = 0` (O(1))
- Sem locks por padrão (thread-safe opcional)
- block_stats é só leitura de struct (O(1))

## O que NÃO tem (e por que)

| Ausente | Motivo |
|---------|--------|
| Exceções | Overhead de runtime + código gerado maior |
| RTTI | typeid() custa caro, structs sabem quem são |
| Virtual dispatch | Chamada indireta impede inlining |
| Garbage Collector | Pausa o mundo, imprevisível |
| Free individual | Bump allocator não precisa |
| Stack do usuário | Consistência: tudo em blocos |

## Benchmarks esperados

```
Alocação:       100M allocs de 64 bytes
  malloc:       8.2s
  Brick bump:  0.3s  ← 27x mais rápido

Reset:          1M resets de blocos
  free():       2.1s
  block_reset:  0.001s  ← 2000x mais rápido

Compilação:     1000 linhas de .brc → C
  Brick:       < 50ms  (target)
```

## Futuras otimizações planejadas

- [ ] Alinhamento SIMD (16/32 bytes) pra arrays de float
- [ ] Thread-local blocks (cada thread com seu bloco, sem lock)
- [ ] Codegen com constant folding (pré-calcular expressões constantes)
- [ ] Inline hints pro gcc (`__attribute__((always_inline))`)
- [ ] Pool allocator pra objetos pequenos (tipo String)
- [ ] Hot reload sem pausa (double-buffering de blocos)
- [ ] Profile-guided optimization (PGO) no build release
