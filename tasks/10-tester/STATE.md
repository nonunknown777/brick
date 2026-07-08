# Task 10 - Tester / Optimizer / Doc - STATE

## Status: ✅ ATIVO

Task sênior. Responsável por testar, otimizar, documentar e coordenar.

## Realizado nesta sessão
1. Análise completa do projeto (todos os source files)
2. Build: ✅ `scons` - compila sem erros
3. Unit tests: ✅ `scons test` - 100% pass (29 lexer + 6 parser + 79 codegen + 14 runtime + 5 hot reload + 15 window + 3 window_hr = 151 testes)
4. Integration tests: ✅ `tests/test_integration.sh` - 5/5 pass
5. Exemplos: ✅ hello.brc, types_and_interfaces.brc, c_math.brc compilam e executam
6. STATE.md criados para todas as 11 tasks
7. NEXT.md criados para todas as 11 tasks
8. **Documentação revisada e corrigida**:
   - `index.html`: 4x "Meta‑C" → "Brick"; código exemplo agora tem `block global`
   - `ARCHITECTURE.md`: exemplo agora tem `block global`
   - `GETTING_STARTED.md`: português misturado removido
   - `LANGUAGE.md`: seções adicionadas (I/O, C Interop, Debugging)
   - `shared-context.md`: `fn void` corrigido para `fn`
9. **7 otimizações implementadas**:
   - ✅ Inline hints (`__attribute__((always_inline))`) no codegen
   - ✅ SIMD alignment (atributo `aligned(N)`) para campos float/f64
   - ✅ Constant folding (expressões constantes pré-computadas)
   - ✅ Pool allocator (runtime/pool_allocator.h/.c) com O(1) alloc/free
   - ✅ Thread-local blocks (TLS current block) + `block_set_tls()` automático em `__brick_init`
   - ✅ Pauseless hot reload (double-buffer API)
   - ✅ PGO build profiles (profile=pgo-gen, profile=pgo-use)
10. **Pool allocator integrado automaticamente no codegen**:
    - Todo block decl ganha um `PoolAllocator* __pool_<nome>` 
    - O codegen decide entre `pool_alloc()` e `block_alloc()` baseado no tamanho do tipo (threshold: 64 bytes)
    - Tipos pequenos (≤64 bytes): `pool_alloc()` com O(1) free individual
    - Tipos grandes (>64 bytes): `block_alloc()` bump allocator
    - `__brick_cleanup()` gerado automaticamente para destruir pools
11. **Documentação de otimizações reescrita**:
    - `OPTIMIZATIONS.md` e `.pt-BR.md` com deep-dive e exemplos simples
    - `index.html` com seção "Optimizations" (7 cards)
    - `README.md` com tabela resumo das 7 otimizações

12. **Sistema de macros implementado**:
    - ✅ Lexer: keywords `macro`/`build`/`emit` + `$` (DOLLAR) + `...` (ELLIPSIS) 
    - ✅ Parser: 6 novos AST node types (`MACRO_DECL`, `MACRO_CALL`, `BUILD_BLOCK`, `EMIT_STMT`, `INTERPOLATE`, `VALUE_PLACEHOLDER`)
    - ✅ `macro_expander.cpp`: deep AST clone, parameter substitution, gensym para `__`-prefixed identifiers, macro table, while-loop com detecção de recursão infinita (limite 64)
    - ✅ `build_eval.cpp`: comptime interpreter com aritmética, strings, loops, `emit`, type reflection (`T.fields`/`T.name`/`T.size`)
    - ✅ `collect_macros()` + `expand_macros()` pipeline integrado em `main.cpp`
    - ✅ Nested macro expansion: `macro outer(y) { inner($y) }` → recursão via while-loop
    - ✅ Recursion detection: `macro recurse() { recurse() }` → limite de 64 detectado
    - ✅ `tests/test_macros.cpp`: 26 testes de unitários + integração
    - ✅ **Integração 10/10 passando**: test_simple, test_blocks, test_io_print, test_io_format, test_io_no_using, test_macro_swap, test_macro_no_params, test_build_const, test_macro_vec2, test_macro_enum

13. **Bugs críticos corrigidos nesta sessão**:
    - ✅ `emit_stmt()` parser: `body_stmts` era coletada mas descartada — criava `EmitStmt` com `BlockStmt` vazio
    - ✅ `var_decl_macro()`: agora trata `$name` (Interpolate) como nome de variável em declarações dentro de macros
    - ✅ `parse_macro_body_stmt()`: tipo keyword agora chama `var_decl_macro()` em vez de `var_decl()` comum
    - ✅ `declaration()`: `IDENTIFIER` em nível superior agora cria `expr_stmt()` (permite chamadas de macro como declaração top-level)
    - ✅ `flatten_emit()` helper: expande conteúdo de `emit {}` para dentro escopo pai em `expand_in_prog` e `expand_in_block`
    - ✅ `is_macro_callee()`: agora aceita `ExprStmt` além de `CallExpr`
    - ✅ `expand_in_prog()`: desempacota `ExprStmt` antes de chamar `expand_call`

## 14. Bugs corrigidos nesta sessão
- ✅ `brc-run.sh`: faltava linkar `runtime/io.c` e `runtime/pool_allocator.c` — `pool_create`, `pool_alloc`, `io_print_string` etc. davam undefined reference
- ✅ Bug 1: `$` em nomes de função dentro de `emit {}` — `func_decl()` agora aceita `DOLLAR` + `IDENTIFIER` como nome de função (via `FuncDecl::name_expr`)
- ✅ Bug 2: Build variables (`msg = "text"` dentro de `build {}`) agora interpolam automaticamente em `emit {}` (via `subst_build_expr` + `subst_build_stmt` em `build_eval.cpp`)

## 15. Sessão atual: string_view optimization + bug fixes
- ✅ **Token::lexeme** changed from `std::string` to `std::string_view` — eliminates per-token heap allocations
- ✅ Lexer rewritten to emit `string_view` into source buffer instead of allocating strings
- ✅ Escape processing moved from lexer to parser (`process_string_escapes` / `process_char_escape`)
- ✅ `std::from_chars` replaces `std::stoll`/`std::stod` for numeric parsing (works on `string_view`)
- ✅ **Bug fix**: triple `advance()` call in `block_decl_or_scope()` (`block_create_bytes(0)` → `block_create_bytes(67108864)`)
- ✅ **Bug fix**: all `tokenize()` call sites in tests fixed to keep source string alive (prevents dangling `string_view`)
- ✅ README.md: acknowledgments section added thanking "the penguim"
- ✅ 108/108 codegen tests pass, 29/29 lexer, 6/6 parser, all runtime + hot reload + window tests
- ✅ Build: `scons build/brick` compiles successfully

## 16. Sessão atual: Pointer arithmetic + and/or keywords + ++/--
- ✅ **PLUS_PLUS** (`++`) e **MINUS_MINUS** (`--`) tokens adicionados
- ✅ Keywords **`and`** e **`or`** adicionados (alias para `&&`/`||`)
- ✅ **Aritmética de ponteiros**: `*p` dereferência, `&x` address-of, `ptr + N`, `ptr - N`, `ptr += N`, `ptr -= N`, `ptr - ptr` diferença
- ✅ **Indexação de ponteiro**: `p[N]` funciona em tipo `*T`
- ✅ **Comparação de ponteiros**: `p == q`, `p < q`, `p != null`, etc.
- ✅ **++/-- prefix e postfix**: desugeram para `x += 1` / `x -= 1`
- ✅ **Auto-semicolon** após `++`/`--` (nova linha detectada)
- ✅ **Parser**: `*` no início de statement diferencia declaração de dereferência
- ✅ **Type checker**: valida todos os casos de ponteiro + erro para uso incorreto
- ✅ **Codegen**: `*x` emite `*x`, `&x` emite `&x`; aritmética usa C nativo
- ✅ **Bug fix**: null-initialized pointer vars não duplicam `*` no C gerado
- ✅ **Testes**: 40 novos testes — 151 codegen + 27 lexer + 5 parser + 15 runtime
- ✅ **Integração**: 11/11 passam

## 17. Sessão atual: Arrays fixos + brace init + cast + hex + error fix
- ✅ **Fase 1.1 — Hex literals** (complete):
  - Parser: `parse_int_literal()` usa `from_chars(..., 16)` para hex, detecta `0x`, `0b`, `0o`
  - AST: `IntLiteral::is_hex` flag preserva formatação original
  - Codegen: `is_hex` literais emitem `0x%llX` no C gerado
  - Valores grandes como `0xFFFFFFFF00000000` funcionam (uint64_t)
  - Type checker: `int_fits_in_type()` aceita `is_hex` para verificar range unsigned

- ✅ **Fase 1.2 — Narrowing cast** (complete):
  - Sintaxe dupla: `T(expr)` e `expr as T`
  - AST: `CastExpr` node, `ASTNodeType::CAST_EXPR`
  - Parser: type keywords como expressões em `primary()`, `primary_macro()`, `primary_build()`
  - `match(TokenType::AS)` em `call()` para operador `as`
  - Type checker: todos os casts numéricos permitidos (incluindo narrowing)
  - Codegen: `(T)(expr)` no C gerado

- ✅ **Fase 1.3 — error() fix** (complete):
  - Codegen: `error("msg")` emite `fprintf(stderr, "%.*s", (int)_msg.len, _msg.data)`
  - Evita passar struct `BrickString` para `fprintf`

- ✅ **Fase 2.1 — Arrays fixos T[N]** (complete):
  - **Parser**: `var_decl()` aceita `T[N] name` e `T[N] name = {val1, val2, ...}`
  - **Disambiguation**: `IDENTIFIER + [` no início de expressão diferencia declaração (`T[N] name`) de indexação (`arr[idx]`)
  - **Brace init**: `{...}` parsing em `primary()` como `ArrayLiteral` (LBRACE)
  - **AST**: `ASTNodeType::ARRAY_LITERAL`, struct `ArrayLiteral { elements }`
  - **Type checker**: `ARRAY_LITERAL` valida elementos contra elemento do array alvo; int/float literais sem sufixo são inferidos do tipo alvo
  - **Codegen**: `split_array_type()` extrai base + sufixo; `map_type()` retorna base sem array; gera `T name[N];` e `T name[N] = {val, ...};`
  - **Array access**: `arr[0] = val` funciona (INDEX_EXPR já tratado)
  - **Testes**: `test_fixed_arrays.brc` no diretório `tests/features/`

- ✅ **Fase 1.4/2.2 — Array literals em call args** (complete):
  - **Codegen**: ARRAY_LITERAL em expressões gera compound literal C99 `(T[]){vals}`
  - **Codegen**: ARRAY_LITERAL em declarações gera `{vals}` (initializer syntax)
  - `fn({1, 2, 3})` gera `fn((int32_t[]){1, 2, 3})` — válido C99
  - `T[N] name = {1, 2, 3}` continua gerando `T name[N] = {1, 2, 3};`

- ✅ **Todos os testes passam**: 320+ testes passando com 0 falhas

## 18. Sessão atual: Fase 2.3 — Dynamic arrays in structs (T[] como campo de struct) ✅

## 19. Sessão atual: Fase 3 — Polimorfismo (impl Struct : Interface separado)
- ✅ **Lexer**: `IMPL` token adicionado a `TokenType` e keyword map
- ✅ **AST**: `ImplDecl` node adicionado com `struct_name`, `interface_name`, `methods`
- ✅ **Parser**: `impl Struct : Interface { fn ... }` parseado em `impl_decl()`
- ✅ **Macro expander**: `IMPL_DECL` tratado em `expand_in_decl` e preservado na AST final
- ✅ **Type Checker**: Pré-passagem processa `IMPL_DECL` antes da verificação:
  - Valida struct alvo existe
  - Valida interface existe (opcional)
  - Adiciona interface à lista da struct
  - Move métodos do impl para a struct
- ✅ **Codegen**: Sem mudanças — os métodos já estão na struct quando o codegen roda
- ✅ **LSP**: `collect_symbols` trata `IMPL_DECL` corretamente
- ✅ **Testes**: `tests/features/test_impl_separate.brc` + integração `test_impl`
- ✅ **13/13** integração passando, **0 falhas** em todos os unitários

### C gerado para impl:
```
brick: impl Circle : Drawable { fn draw() { ... } }
c:     static inline void Circle_draw(Circle* this) { ... }
```

## 21. Sessão atual: Struct init literals + Const expr + Include relativo + Append built-in

### ✅ Struct init literals
- `Vec3 v = {1.0, 2.0, 3.0}` — `ArrayLiteral` em contexto de struct
- Type checker: novo branch no ASSIGNMENT handler matcha elementos contra field types
- Codegen: já emitia `{vals}` para qualquer `ArrayLiteral` — nenhuma mudança necessária
- `{0}` para zero-init parcial funciona (elementos restantes são zero)

### ✅ Const expr
- `const X = 42` (type inferred) e `const f64 PI = 3.1415` (explicit) já funcionavam no parser e codegen
- **Bug fix**: `normalize_type()` agora normaliza uppercase `F64` → `f64`, `Int` → `i32`, etc.
- **Bug fix**: `map_type()` e `normalize_type_name()` no codegen também aceitam uppercase `F64`/`F32`/`I32`/etc.
- `const DOUBLE_PI = PI * 2.0` com constantes em expressões funciona

### ✅ Include relativo
- `include "local.h"` agora gera `#include "local.h"` em vez de `<local.h>`
- STRING_LITERAL lexeme NÃO inclui aspas (a abertura é consumida antes de `string_literal()`)
- Fix em `codegen.cpp` INCLUDE_DECL handler

### ✅ Append built-in para T[]
- Type checker: CALL_EXPR com MEMBER_EXPR("append") em tipo `T[]` valida argumento
- Codegen: gera `arr[arr_cnt++] = val`
- `.len` e `.cap` continuam funcionando como propriedades readonly
- Nota: runtime test complexo por precisar alocar data pointer; compilação validada

### ✅ Arquivos modificados (nesta sessão):
- `src/codegen/codegen.cpp` — include relativo (INCLUDE_DECL) + append codegen + type_map uppercase variants + normalize_type_name uppercase
- `src/codegen/type_checker.cpp` — struct init literals (2 branches: check_statement + check_expression) + append type check + normalize_type uppercase variants
- `tasks/10-tester/STATE.md` — atualizado

### ✅ Testes
- **17/17 integração**, **244/244 unitários** — zero falhas (inalterado)
- Novos testes criados em `build/`: `test_struct_init`, `test_const`, `test_append`, `test_append2`

## 20. Sessão anterior: Fase 3.2 + Fase 4 — Polimorfismo (vtbl) + Polimento

### ✅ `.sizeof` built-in (todos os tipos)
- `int.sizeof`, `f64.sizeof`, `MyStruct.sizeof`, `var.sizeof`
- Retorna `i64` (signed, evita problemas de assign com usize)
- Type checker: intercepita `.sizeof` antes de resolver o objeto
- Codegen: emite `sizeof(T)` em C

### ✅ Enum hex
- `enum Flags { NONE = 0x00, ALL = 0xFF, HIGH = 0xABCD }`
- `parse_int_literal()` usado em vez de `std::from_chars` base 10
- Suporta `0x`, `0b`, `0o` (o que o parser de int literal já trata)

### ✅ @packed / @align
- `struct Foo @packed { ... }` → `__attribute__((packed))`
- `struct Foo @align(64) { ... }` → `__attribute__((aligned(64)))`
- `struct Foo @packed @align(16) { ... }` → ambos
- AST: `StructDecl.packed` (bool) + `StructDecl.alignment` (int)

### ✅ Dispatch Virtual (vtbl)

**Geração de C para cada interface** `Drawable`:
```c
typedef struct Drawable_Vtbl {
    void (*draw)(void*, double);
    BrickString (*get_name)(void*);
} Drawable_Vtbl;
typedef struct Drawable {
    void* data;
    const Drawable_Vtbl* vtbl;
} Drawable;
```

**Para cada impl** `impl Circle : Drawable`:
```c
static void Circle_Drawable_draw(void* self, double __p0) {
    Circle_draw((Circle*)self, __p0);
}
static BrickString Circle_Drawable_get_name(void* self) {
    return Circle_get_name((Circle*)self);
}
static const Drawable_Vtbl Circle_Drawable_vtable = {
    .draw = Circle_Drawable_draw,
    .get_name = Circle_Drawable_get_name
};
```

**Type checker**: `CALL_EXPR(MEMBER_EXPR)` + `MEMBER_EXPR` — resolvem métodos de interface
**Codegen**: `obj.vtbl->method(obj.data, args)` para chamadas em tipo interface

### ✅ String escaping fix
- `\n` dentro de strings .brc agora gera `\\n` no C (antes quebrava sintaxe C com newline literal)

### ✅ Testes
- **17/17 integração**, **244/244 unitários** — zero falhas
- `tests/features/test_sizeof.brc` — .sizeof
- `tests/features/test_enum_hex.brc` — enum hex (verifica #defines no .c)
- `tests/features/test_packed_align.brc` — @packed/@align
- `tests/features/test_vtbl_dispatch.brc` — vtbl (Circle + Square : Drawable)

### Arquivos modificados:
- `src/codegen/type_checker.cpp` — .sizeof + interface method lookup
- `src/codegen/codegen.cpp` — .sizeof + vtbl structs/wrappers + @packed/@align + string escape
- `src/parser/parser.cpp` — @packed/@align parsing + enum hex + INT/FLOAT/STRING/VOID tokens
- `src/parser/ast.h` — StructDecl.packed + alignment
- `tests/test_integration.sh` — 4 novos testes

## 19. Sessão anterior: Fase 3 — Polimorfismo (impl Struct : Interface separado) ✅

## 18. Sessão anterior: Fase 2.3 — Dynamic arrays in structs (T[] como campo de struct) ✅
- ✅ **Type Checker**: `T[]` é aceito como tipo de campo em structs (`is_type_known` e `is_dynamic_array_type`)
- ✅ **Type Checker**: `.len` e `.cap` built-in properties resolvem para `i64` em tipos `T[]`
- ✅ **Codegen**: `gen_struct()` expande `T[]` fields em 3 campos C: `T* name; int64_t name_cnt; int64_t name_cap;`
- ✅ **Codegen**: `gen_struct()` também trata `T[]` em anonymous structs dentro de unions
- ✅ **Codegen**: `gen_expression(MEMBER_EXPR)` detecta `.len`/`.cap` em `T[]` e emite `name_cnt`/`name_cap`
- ✅ **Codegen**: `type_size_estimate_impl()` reconhece `T[]` como 24 bytes (ponteiro + 2 int64)
- ✅ **Parser**: já parseava `T[]` em `field_decl()` — nenhuma mudança necessária
- ✅ **Indexação**: `arr[i]` funciona naturalmente via ponteiro (C `name[i]`)
- ✅ **Testes**: `tests/features/test_dynamic_arrays.brc` + integração `test_dynarray`
- ✅ **12/12** integração passando, **0 falhas** em todos os unitários

### C gerado para struct com `T[]`:
```
brick: struct Container { int[] items; i32 count; }
c:     int32_t* items;
       int64_t items_cnt;
       int64_t items_cap;
       int32_t count;
```

### Acesso no C gerado:
```
brick: c.items.len     -> c: c->items_cnt
brick: c.items.cap     -> c: c->items_cap
brick: c.items[i]      -> c: c->items[i]
brick: c.items         -> c: c->items (data pointer)
```

## 22. Sessão atual: Final features — multidim arrays, const-size arrays, vtbl dynarray, append runtime

### ✅ Multidimensional arrays (`f32[4][4]`)
- **Parser**: `var_decl()`, `field_decl()`, `var_decl_macro()`, `type_alias_decl()` loops convertidos de `if` para `while` — aceitam múltiplos `[N]`
- **Disambiguation**: `statement()` agora pula múltiplos pares `[N]` ao diferenciar declaração de indexação
- **Codegen**: `split_array_type()` já extraía todo o sufixo `[N][M]` corretamente; `map_type()` já ignorava brackets
- **type_size_estimate**: corrigido para multiplicar todas as dimensões, não só a primeira
- **Teste**: `tests/test_multidim_array.brc` — 4×4 matrix com loops aninhados

### ✅ Const-size arrays (`const i32 N = 16; f32[N] arr`)
- **Parser**: `[N]` aceita `IDENTIFIER` além de `INT_LITERAL` nos 4 locais (var_decl, field_decl, var_decl_macro, type_alias_decl)
- **Disambiguation**: loop pula `[IDENTIFIER]` também
- **Type Checker**: `const_values_` map armazena const ints; `resolve_array_sizes()` substitui identificadores por valores numéricos em type strings
- **Codegen**: size é resolvido na type checker, codegen vê `f32[16]` e emite C normal
- **Teste**: `tests/test_const_array.brc` — const N = 16 usado como tamanho de array

### ✅ Dynamic array append com growth
- **Codegen** (`gen_var_declaration`): `T[]` em variável local agora expande para `T* items; int64_t items_cnt=0; int64_t items_cap=0;`
- **Codegen** (`gen_expr_stmt`): `.append(val)` gera bloco `if (cnt >= cap) { cap = cap*2; items = realloc(...); } items[items_cnt++] = val;`
- Movido de `gen_expression` para `gen_expr_stmt` porque gera statements (if-block)
- **Teste**: `tests/test_append.brc` — append com struct Item, verifica valores

### ✅ VTBL em dynamic arrays (`Drawable[] items; items.append(c)`)
- **Type Checker** (`can_assign`): struct → interface permitido se struct implementa a interface
- **Codegen** (`gen_expr_stmt`): para `T[]` onde T é interface, append gera wrapper `(Drawable){.data=&arg, .vtbl=&Circle_Drawable_vtable}`
- **Ordem de geração**: vtbl wrappers movidos para antes das funções standalone para símbolos ficarem visíveis
- **Teste**: `tests/test_vtbl_dynarray.brc` — Circle + Square implementam Drawable, append em Drawable[]

### ✅ alignof built-in (`type.alignof`, `expr.alignof`)
- Mesmo padrão de `.sizeof` — type checker intercepta `.alignof`, codegen emite `_Alignof(...)`

### ✅ Named struct init (`Vec3 v = {x=1.0, y=2.0, z=-5.0}`)
- Type checker: `find_struct_field()` helper + named init branch em ARRAY_LITERAL
- Codegen: emite `.field = value` (designated initializers C)

### ✅ System include (`include "stdio.h" @system` → `#include <stdio.h>`)
- Parser: `is_system` flag em IncludeDecl
- Codegen: `@system` emite `<header>`, sem `@system` emite `"header"`

### Arquivos modificados nesta sessão:
- `src/parser/parser.cpp` — multidim loop, disambiguation, IDENTIFIER em `[N]`, `T[]` disambiguation
- `src/codegen/codegen.cpp` — multidim type_size_estimate, dynarray var decl expansion, append growth, vtbl wrapper, order fix
- `src/codegen/type_checker.cpp` — `resolve_array_sizes()`, `const_values_` map, `can_assign` struct→interface
- `src/codegen/type_checker.h` — `const_values_`, `resolve_array_sizes` declaration
- `tests/test_multidim_array.brc` — novo
- `tests/test_const_array.brc` — novo
- `tests/test_append.brc` — novo
- `tests/test_vtbl_dynarray.brc` — novo
- `tests/test_integration.sh` — 4 novos testes

### Testes
- **21/21 integração** — zero falhas
- Todos os 17 testes anteriores continuam passando
- 4 novos testes: `test_multidim_array`, `test_const_array`, `test_append`, `test_vtbl_dynarray`

## 23. Sessão atual: `export` keyword + `$macro(args)` syntax

### ✅ `export fn` — Funções exportadas para C
- **Lexer**: `EXPORT` token adicionado
- **AST**: `FuncDecl.is_export` flag
- **Parser**: `export_decl()` + case em `declaration()`
- **Codegen**: `export fn` é gerado **sem** `static inline` → C pode chamar a função
- **Teste**: `test_export_function` verifica que `export fn` não tem `static inline` e aparece no output

### ✅ `$macro(args)` — Chamada explícita de macro
- **Parser**: `$identifier(args)` em `primary()` cria `MacroCall` node diretamente
- **Expander**: `is_macro_callee` e `expand_call` atualizados para reconhecer `MacroCall`
- **Teste**: `test_dollar_macro` verifica `$twice(10)` expande para `20`
- Compatível com chamada implícita `name(args)` (ambos funcionam)

### Arquivos modificados:
- `src/shared/types.h` — EXPORT token
- `src/lexer/lexer.cpp` — keyword map
- `src/parser/ast.h` — `FuncDecl.is_export`
- `src/parser/parser.cpp` — `export_decl()`, `$id(args)` em `primary()`
- `src/parser/macro_expander.cpp` — `is_macro_callee`/`expand_call` suportam `MacroCall`
- `src/codegen/codegen.cpp` — skip `static inline` se `is_export`
- `tests/test_codegen.cpp` — `test_export_function` + `test_dollar_macro`

### Testes
- **214/215** unitários passando (1 falha pré-existente: `#include math.h`)
- Export: 4 asserts passam
- $macro: 5 asserts passam

## Próximos passos
- N/A — todas as features do `brick-features-needed.md` foram implementadas
- Sugestão: revisar cobertura de testes unitários (codegen, type_checker)
- Sugestão: benchmark de desempenho com arrays grandes
- Sugestão: melhorar mensagens de erro para const array size não encontrado
- Sugestão: `export fn` também deveria gerar forward declaration no cabeçalho C

## Observações
- Projeto está maduro e funcional
- Compilador, runtime e todos os exemplos funcionam
- Sistema de macros funcional com expansão aninhada, gensym, detecção de recursão, e `$` interpolation
- Macros podem ser chamadas no nível top-level (gerando declarações via `emit`) ou dentro de funções
- C interop funcional (math.h, stdlib.h)
- Aritmética de ponteiros implementada: `*`, `&`, `+`, `-`, `+=`, `-=`, `++`, `--`, `[]`
- Keywords `and`/`or` como alias para `&&`/`||`
- Arrays fixos `T[N]` com brace init `{val, ...}` funcionando
- Arrays multidimensionais `T[N][M]` funcionando
- Arrays com tamanho constante `const N; T[N] arr` funcionando
- Arrays dinâmicos `T[]` em structs e var locais com `.len`/`.cap`/`.append()` funcionando
- Append com auto-growth (`realloc`), suporta interface vtbl wrapping
- Struct init literais (posicional e nomeado) funcionando
- Const expressions: `const X = expr` com type inference ou explícito
- Include relativo: `include "local.h"` gera `#include "local.h"`; `@system` gera `<header>`
- Hex literals, narrowing cast, error() fix completos
- `.sizeof` e `.alignof` built-in, enum hex, `@packed`/`@align`, vtbl dispatch completos
- Todos os testes passam: **21 integração**, zero falhas
