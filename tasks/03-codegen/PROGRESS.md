## Progress

### Done
### Feito

- **Type checker** (`type_checker.h/.cpp`): validação de tipos, resolução de nomes, checagem de construtor, herança com lookup recursivo em `declare_inherited_fields`
- **Type checker** (`type_checker.h/.cpp`): type validation, name resolution, constructor checking, inheritance with recursive lookup in `declare_inherited_fields`
- **Codegen** (`codegen.cpp`): geração C para structs com extends + métodos, funções, expressões, if/else, while, for, return, block_create em init separado, block_alloc, MetaCString, null literal, block scope push/pop, `this->field` em métodos
- **Codegen** (`codegen.cpp`): C generation for structs with extends + methods, functions, expressions, if/else, while, for, return, block_create in separate init, block_alloc, MetaCString, null literal, block scope push/pop, `this->field` in methods
- **Parser**: `IdentExpr.declared_type` para declarações com tipo (`int x = 5`)
- **Parser**: `IdentExpr.declared_type` for typed declarations (`int x = 5`)
- **44 testes passando** (incluindo compilação `gcc -O3 -Wall -Werror`)
- **44 tests passing** (including `gcc -O3 -Wall -Werror` compilation)
- **SConstruct**: corrigido com `PROJECT_ROOT` para compilação C
- **SConstruct**: fixed with `PROJECT_ROOT` for C compilation

### Issues corrigidas
### Fixed issues

| Problema | Causa | Solução |
|----------|-------|---------|
| `&&`/`||` não compilava | Faltava AND/OR no grammar | Adicionado no parser |
| `extends` falhava | Type checker não herdava fields | `declare_inherited_fields()` recursivo |
| `this->field` não gerado | Codegen tratava field como var local | Struct context + `current_struct_fields` |
| `int x = null` gerava `null x = NULL` | Parser perdia type annotation | `IdentExpr.declared_type` + pointer type |
| `block_create()` global = erro C | C não aceita function call em init | `__meta_c_init()` separada |
| Stubs duplicados no compile test | generated code já define métodos | Stubs movidos + include removido |

| Problem | Cause | Solution |
|---------|-------|----------|
| `&&`/`||` didn't compile | Missing AND/OR in grammar | Added in parser |
| `extends` failed | Type checker didn't inherit fields | Recursive `declare_inherited_fields()` |
| `this->field` not generated | Codegen treated field as local var | Struct context + `current_struct_fields` |
| `int x = null` generated `null x = NULL` | Parser lost type annotation | `IdentExpr.declared_type` + pointer type |
| `block_create()` global = C error | C doesn't accept function call in init | Separate `__meta_c_init()` |
| Duplicate stubs in compile test | Generated code already defines methods | Stubs moved + include removed |
