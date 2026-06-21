# Próximo Passo - Lexer
# Next Step - Lexer

## Imediato
## Immediate

Nenhuma mudança necessária pra feature `using IO;` / `print()`.
No changes needed for the `using IO;` / `print()` feature.

## Bug corrigido
## Bug fixed

`print(str)` com `String` block-allocada (`@block`) gerava C inválido:
`str.data` → `str->data` (pois `MetaCString*` é ponteiro).
`print(str)` with block-allocated `String` (`@block`) generated invalid C:
`str.data` → `str->data` (because `MetaCString*` is a pointer).

Fix em `src/codegen/codegen.cpp`:
Fix in `src/codegen/codegen.cpp`:

- `gen_print_single`: acesso a `.data`/`.len` agora verifica `pointer_vars`
- `gen_print_single`: access to `.data`/`.len` now checks `pointer_vars`
- `gen_printf_call`: mesmo para format strings com `{N}`
- `gen_printf_call`: same for format strings with `{N}`

## Contexto
## Context

O token `USING` já existe (linha 11 de types.h). `IO` e `print` são
identificadores normais (IDENTIFIER). A string literal já é tokenizada
corretamente, incluindo formatos como `"Hello {0}"` — chaves dentro de
strings não precisam de token especial.
The `USING` token already exists (line 11 of types.h). `IO` and `print` are
normal identifiers (IDENTIFIER). The string literal is already tokenized
correctly, including formats like `"Hello {0}"` — braces inside
strings don't need a special token.

## Pendências
## Pending

Nenhuma — lexer não precisa de alterações para esta feature.
None — lexer doesn't need changes for this feature.
