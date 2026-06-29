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

## Arquivos
- `src/parser/ast.h` - Todos os AST nodes (~304 linhas)
- `src/parser/parser.h` - API pública
- `src/parser/parser.cpp` - Implementação (~766 linhas)
- `src/parser/package.h` - Package resolution API
- `src/parser/package.cpp` - Package resolution
- `tests/test_parser.cpp` - 6 testes

## Observações
- Array types com tamanho fixo (`T[N]`) são parseados mas codegen pode não suportar totalmente
- `block nome:` (colon syntax para mudar bloco default) não implementado
