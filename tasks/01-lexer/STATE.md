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

## Mudanças recentes
- **Token::lexeme** mudou de `std::string` para `std::string_view` (src/shared/types.h)
- Lexer reescrito para emitir `string_view` apontando direto pro buffer de source — zero alocação por token
- Escape sequences agora são processadas no parser, não no lexer (string/char literal armazenam raw content)
- `std::from_chars` usado para parse numérico (funciona com `string_view`)

## Observações
- `and` não é keyword — tratado contextualmente no parser
- Token types em `src/shared/types.h`
- Atenção: `string_view` depende do source string permanecer vivo — test infrastructure adaptada com `shared_ptr<string>`
