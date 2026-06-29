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

## Observações
- Projeto está maduro e funcional
- Compilador, runtime e todos os exemplos funcionam
- Sistema de macros funcional com expansão aninhada, gensym, detecção de recursão, e `$` interpolation
- Macros podem ser chamadas no nível top-level (gerando declarações via `emit`) ou dentro de funções
- C interop funcional (math.h, stdlib.h)
- Aritmética de ponteiros implementada: `*`, `&`, `+`, `-`, `+=`, `-=`, `++`, `--`, `[]`
- Keywords `and`/`or` como alias para `&&`/`||`
- Todos os testes passam: 11/11 integração + 198 unitários
