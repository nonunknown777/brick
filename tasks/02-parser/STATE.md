# Task 02 - Parser - STATE

## Status: ✅ COMPLETO

Parser completo. Todos os 6 testes unitários passam. Consegue parsear todos os exemplos .brc com sucesso.

## Implementado
- Program, Package, Using declarations
- Struct/Interface declarations com extends
- Field declarations com tipos e arrays (parcial)
- Function/method declarations com params e retorno
- Constructor (fn nome_struct)
- Block declaration (`block nome = N KB/MB/GB`)
- Block scope (`block nome { }`)
- Control flow: if/else, while, for
- Expression parsing com precedência completa
- Extern/Include/Link declarations
- Auto-semicolon insertion
- `@` alloc inline operator
- `private` modifier
- Literal suffixes preservados

## Mudanças recentes
- **Bug fix**: triple `advance()` call em `block_decl_or_scope()` (linha 1178) — `std::from_chars` recebia 3 tokens diferentes em vez de 1, resultando em `block_create_bytes(0)`
- Escape processing adicionado: `process_string_escapes(sv)` e `process_char_escape(sv)` — chamados ao construir nós literais (STRING_LITERAL, CHAR_LITERAL, ERROR message)

## Novidades (Sessão 22 — Final features)
- ✅ **Multidimensional arrays**: `[N]` loops em `var_decl()`, `field_decl()`, `var_decl_macro()`, `type_alias_decl()` convertidos de `if` para `while` — aceitam múltiplos `[N][M]`
- ✅ **Const-size arrays**: `[N]` aceita `IDENTIFIER` além de `INT_LITERAL` nos mesmos 4 locais
- ✅ **Disambiguation**: `statement()` agora pula múltiplos `[N]`, `[]`, e `[IDENTIFIER]` ao diferenciar `T[] name`/`T[N] name` de `arr[idx]`
- ✅ **Include relativo**: STRING_LITERAL lexeme não inclui aspas
- ✅ **System include**: `IncludeDecl.is_system` flag; `@system` token sequence parseado

## Arquivos
- `src/parser/ast.h` - Todos os AST nodes (~304 linhas)
- `src/parser/parser.h` - API pública
- `src/parser/parser.cpp` - Implementação (~766 linhas)
- `src/parser/package.h` - Package resolution API
- `src/parser/package.cpp` - Package resolution
- `tests/test_parser.cpp` - 6 testes

## Novidades (Fase 3 — impl Struct : Interface)
- `impl_decl()` adicionado: parseia `impl Struct : Interface { fn ... }`
- `ImplDecl` AST node com struct_name, interface_name, methods
- `IMPL` keyword registrada no lexer
- `parse_macro_body_stmt` — `IMPL` não é relevante dentro de macros (não adicionado)

## Observações
- Array types com tamanho fixo (`T[N]`) e dinâmico (`T[]`) são parseados via `field_decl()`
- `block nome:` (colon syntax para mudar bloco default) não implementado
