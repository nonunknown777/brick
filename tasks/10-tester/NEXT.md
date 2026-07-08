# Task 10 - Tester / Optimizer / Doc - NEXT

## Concluído nesta sessão (23)

### ✅ `export fn` — Funções visíveis para C
- Lexer: `EXPORT` token
- AST: `FuncDecl.is_export`
- Parser: `export_decl()` aplica flag
- Codegen: gera sem `static inline`
- Teste: 4 asserts no `test_export_function`

### ✅ `$macro(args)` — Chamada explícita de macro
- Parser: `$id(args)` em `primary()` cria `MacroCall`
- Expander: `is_macro_callee` + `expand_call` reconhecem `MacroCall`
- Teste: `test_dollar_macro` com `$twice(10)` → `20`
- Compatível com chamada implícita existente

## Pendências / Sugestões

### Testes
- ✅ 214/215 unitários (1 falha pré-existente: `#include math.h` sem `@system`)
- Sugestão: adicionar testes unitários para `resolve_array_sizes()` na type checker
- Sugestão: testar const-size array com struct fields
- Sugestão: testar `export fn` com struct param (gera `*`)

### Otimizações
- string interning (parser)
- profiling do compilador
- inline asm para hot paths
- Benchmark de arrays grandes / multidimensionais

### Documentação
- Atualizar `shared-context.md` com `export` + `$macro()`
- Atualizar `docs/LANGUAGE.md`

### Coordenação
- STATE.md task 10 atualizado nesta sessão
- STATE.md task 03 (codegen) — sem mudanças no type checker
- Notificar tasks 01-09 sobre `export` + `$macro`
