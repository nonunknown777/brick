# Estado Atual
# Current State

Sessão: 4 (C Interop)
Session: 4 (C Interop)

Progresso: 100%
Progress: 100%

Próximo passo: Integração com codegen — verificar exemplos .brc compilam
Next step: Integration with codegen — verify .brc examples compile

Última ação: C Interop — include/link/extern declarações, *T pointer types
Last action: C Interop — include/link/extern declarations, *T pointer types

## Realizado / Completed
### C Interop
- `declaration()`: dispatches `INCLUDE` → `include_decl()`, `LINK` → `link_decl()`, `EXTERN` → `extern_decl()`
- `include_decl()`: parseia `include "header" [and link lib]` → `IncludeDecl` (header + link_lib)
- `link_decl()`: parseia `link libname` → `LinkDecl`
- `extern_decl()`: parseia `extern fn name(params) -> ret` → `FuncDecl` com `is_extern = true` (sem body)
- `parse_type_name()`: aceita prefixo `*` para pointer types (e.g. `*u8`, `*void`, `*MyStruct`)
- `statement()`: dispatch `STAR` case para var decls com pointer type
- `and`: tratado contextualmente (IDENTIFIER "and") apenas em `include_decl()` — não é keyword global

### AST (ast.h)
- `IncludeDecl`: campos `header` (string) + `link_lib` (string, opcional)
- `LinkDecl`: campo `lib` (string)
- `FuncDecl`: campo `is_extern` (bool, default false)

### Testes
- Testes existentes continuam passando (97/97 unitários, 6/6 integração)

- `is_type_keyword()`: added U8..U64, I8..I64, F32/F64, USIZE, ISIZE, BYTE
- `statement()`: var_decl switch includes new types
- `IntLiteral`/`FloatLiteral` in ast.h: `literal_type` field populated from token
- `primary()`: INT_LITERAL/FLOAT_LITERAL passes `literal_type` from token
- Tests: `test_fixed_width_types`, `test_literal_suffixes`, `test_fixed_width_struct_fields`
