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

## Observações
- `fn main()` em Brick → `int main()` em C (return 0 implícito)
- Struct params viram pointers em C (block-allocated semantics)
- Generated C compila com `gcc -O3 -Wall -Werror` (verificado em test)
