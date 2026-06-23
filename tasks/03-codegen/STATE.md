# Estado Atual - Codegen
# Current State - Codegen

Sessão: 2026-06-23 (C Interop)
Session: 2026-06-23 (C Interop)

Progresso: 100%
Progress: 100%

Última ação: C Interop — extern fn + include/link + *T pointers + String→*u8
Last action: C Interop — extern fn + include/link + *T pointers + String→*u8

## Realizado (C Interop)
## Completed (C Interop)

### Type Checker
- `extern_func_defs` map (string name → FuncDecl*) armazena funções externas
- `check_expression()`: resolve return type via extern_func_defs para chamadas
- `is_type_known("*T")`: aceita pointer types (prefixo `*`)
- `can_assign()`: permite String→*u8, null→*T, *T↔*T

### Codegen
- `#include <header>` emitido para IncludeDecl
- `link_lib` propagado como `-l<lib>` via `link_flags` em `CodegenResult`
- `*u8` → `char*` (match BrickString.data), `*T` → `T*` via `map_type()`
- String→`*u8` auto-conversão em CALL_EXPR (extrai `.data`)
- Protótipos C NÃO emitidos — headers fornecem declarações (evita `char*` vs `const char*`)

### LSP
- EXTERN/INCLUDE/LINK tokens adicionados ao switch de `token_type_name()`
- INCLUDE_DECL/LINK_DECL adicionados ao switch de `collect_symbols()`

### Testes
- 97/97 unitários passando (79 codegen + 15 window + 3 window HR)
- Integração: 6/6 passando (5 antigos + test_c_interop.brc)
