# Task 03 - Codegen - STATE

## Status: ✅ COMPLETO

Codegen completo. 79 testes unitários passam (todos). Geração de C com #line directives, type checking completo, suporte a I/O, C interop, tipos de largura fixa.

## Implementado
- Type checker completo com:
  - Tipos built-in: i8-u64, f32/f64, bool, char, String, void, block, usize/isize
  - Aliases: int=i32, float=f32, byte=u8, short=i16, long=i64, double=f64
  - Widening permitido, narrowing proibido
  - Signed↔Unsigned mesmo rank proibido
  - Int + Float → Float promotion
  - Overflow em literal compile-time detectado
  - Literal suffixes (42u8, 3.14f32, etc.)
  - Pointer types (*T)
  - can_assign completo com regras de null, ponteiros, String→*u8
- Struct inheritance com base field
- Constructor/method codegen
- Block memory: block_create, block_alloc, block_register, block_reset
- Block scope push/pop (_current_block)
- print() I/O com tipos variados e formatação {N}
- C interop: include/link/extern fn, String→*u8 auto-conversion
- #line directives para debug
- error() → fprintf(stderr) + exit(1)
- __brick_init() com shm_export

## Arquivos
- `src/codegen/codegen.h` - API
- `src/codegen/codegen.cpp` - Codegen C (~1133 linhas)
- `src/codegen/type_checker.h` - Type checker API
- `src/codegen/type_checker.cpp` - Type checker (~1012 linhas)
- `tests/test_codegen.cpp` - 79 testes

## Novidades (Fase 2.3 — Dynamic arrays in structs)
- `T[]` em struct fields → expandido para `T* name; int64_t name_cnt; int64_t name_cap;`
- `.len` em `T[]` → acessa `name_cnt` no MEMBER_EXPR handler
- `.cap` em `T[]` → acessa `name_cap`
- Indexação `arr[i]` funciona via pointer C nativo
- `type_size_estimate("T[]")` = 24 bytes

## Novidades (Fase 3 — impl Struct : Interface)
- Nenhuma mudança no codegen: o type checker adiciona métodos do `impl` à struct
- O codegen itera `sd->methods` e gera as funções normalmente (`StructName_method`)
- `IMPL_DECL` é ignorado nos loops do codegen (cai em `default: break`)

## Novidades (Fase 3.2 + Fase 4 — vtbl + polimento)
- Vtbl struct gerada para cada interface: `typedef struct Interface_Vtbl { ret (*name)(void*, ...); }`
- Interface wrapper struct: `typedef struct Interface { void* data; const Interface_Vtbl* vtbl; }`
- Wrapper functions: `static void Struct_Interface_method(void* self, ...) { Struct_method((Struct*)self, ...); }`
- Static vtable instances: `static const Interface_Vtbl Struct_Interface_vtable = { .method = Struct_Interface_method };`
- Interface method calls: `obj.vtbl->method(obj.data, args)`
- `.sizeof` built-in: `int.sizeof`, `f64.sizeof`, `StructName.sizeof`, `var.sizeof` → `sizeof(T)` em C
- `@packed` struct → `__attribute__((packed))` em C
- `@align(N)` struct → `__attribute__((aligned(N)))` em C
- Const expr codegen: `const type name = expr;` → `static const C_type name = expr;`
- Struct init: `Vec3 v = {1.0, 2.0, 3.0}` → já gerado via ArrayLiteral initializer syntax
- Include relativo: `include "local.h"` → `#include "local.h"` (fix no INCLUDE_DECL handler)
- Append built-in para T[]: `arr[arr_cnt++] = val` (inline no CALL_EXPR handler)
- `normalize_type()` no type checker agora aceita uppercase `F64`/`F32`/`I32`/etc.
- `map_type()` e `normalize_type_name()` no codegen também aceitam uppercase variants

## Novidades (Sessão 22 — Final features)
- ✅ **Multidimensional arrays**: `split_array_type()` já extraía `[N][M]` como sufixo único; `map_type()` já ignorava brackets; `type_size_estimate()` corrigido para multiplicar todas as dimensões
- ✅ **Const-size arrays**: type checker resolve `f32[N]` → `f32[16]` via `resolve_array_sizes()` antes do codegen; codegen vê apenas tipos resolvidos
- ✅ **Dynamic array local vars**: `gen_var_declaration()` expande `T[]` → `T* items; int64_t items_cnt=0; int64_t items_cap=0;`
- ✅ **Append com growth**: movido para `gen_expr_stmt()`, gera `if(cnt>=cap){cap*=2; items=realloc(...);} items[items_cnt++] = val;`
- ✅ **VTBL em dynamic arrays**: `can_assign()` permite struct→interface; codegen gera `(Drawable){.data=&arg, .vtbl=&Circle_Drawable_vtable}`
- ✅ **Ordem vtbl**: wrapper/vtable gerados ANTES das funções standalone para símbolos ficarem visíveis
- ✅ **alignof**: `.alignof` emite `_Alignof(...)` em C
- ✅ **Named struct init**: `{x=0, y=0, z=-5}` → `.field = value` em C (designated initializers)
- ✅ **System include**: `include "h" @system` → `#include <h>`

## Observações
- `fn main()` em Brick → `int main()` em C (return 0 implícito)
- Struct params viram pointers em C (block-allocated semantics)
- Generated C compila com `gcc -O3 -Wall -Werror` (verificado em test)
