# Task 01 - Lexer - STATE

## Status: ✅ COMPLETO

O lexer está completo e funcionando. Todos os 29 testes unitários passam.

## Implementado
- Tokenização completa: keywords, operators, literals (string, char, int, float)
- Literal suffixes: u8/u16/u32/u64, i8/i16/i32/i64, f32/f64, usz/isz
- Auto-semicolon insertion (parser-level)
- Source location tracking (line, col, file)
- Escape sequences em string/char
- Comments (//)
- Error handling (unterminated string/char, invalid escape, unexpected char)

## Arquivos
- `src/lexer/lexer.h` - API pública
- `src/lexer/lexer.cpp` - Implementação (~305 linhas)
- `tests/test_lexer.cpp` - 29 testes

## Observações
- `and` não é keyword — tratado contextualmente no parser
- Token types em `src/shared/types.h`
