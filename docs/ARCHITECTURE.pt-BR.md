# Arquitetura do Brick

## Visão Geral

Brick compila arquivos fonte `.brc` para código C, então delega a `gcc` ou `clang` para geração de código de máquina. O compilador é escrito em C++20.

```
.brc → Lexer → Tokens → Parser → AST → Type Checker → C Codegen → .c → gcc -O3 → binário
                           ↕
                    Macro Expander
                    Build Eval
```

## Pipeline

### 1. Lexer (`src/lexer/`)

Tokeniza fonte `.brc` em um fluxo de tokens. Suporta:
- Palavras-chave (`fn`, `struct`, `if`, `while`, `for`, `match`, `defer`, `const`, etc.)
- Literais: inteiro (decimal/hex/binário/octal), float, char, string, bool, null
- Operadores: `+ - * / = == != < > <= >= && || ! & | ^ ~ << >> ++ -- += -= *= /= @`
- Comentários `//`

### 2. Parser (`src/parser/`)

Parser recursivo descendente que produz uma AST. Inclui:
- Declarações: structs, enums, unions, funções, consts, macros, interfaces, impl, type aliases
- Expressões e statements: if, while, for, for-in, return, break, continue, match, defer
- Sistema de pacotes: package, using, export, private, aninhados (MATH.VEC2)
- Sistema de macros: macro, $, build {}, emit {}
- Interop C: include, link, extern fn, @system

### 3. Codegen (`src/codegen/`)

Duas fases:
1. **Type Checker**: Verificação de tipos, widening/narrowing, overflow, visibilidade
2. **C Codegen**: Produz C legível com diretivas `#line` para debug

### 4. Runtime

Componentes:
- `block_memory.c` — Bump allocator por blocos (~3 ciclos), TLS, double-buffer
- `pool_allocator.c` — Pool allocator O(1) para tipos ≤ 64 bytes
- `hot_reload.c` — Hot reload via dlopen (Linux) / LoadLibrary (Windows)
- `io.c` — I/O com print() formatado e tipo String

## Visualizador de Memória

TUI ncurses mostrando estado dos blocos: capacidade, uso, pico, contagem de alocações.

## Extensão VS Code

Syntax highlighting (TextMate), LSP (completions, hover, go-to-def, signature help), webview de memória.

## Debugger

`.gdbinit` + pretty-printers Python para `BlockCtx` e `BrickString`. Comandos: `info blocks`, `block <name>`.

## Test Suite

- **Unit tests**: 198+ testes em `tests/test_codegen.cpp`
- **Integration tests**: 30+ testes em `tests/test_integration.sh`
- **Feature tests**: 16 testes em `tests/features/`

## Performance

| Operação | Tempo |
|----------|-------|
| Block allocation | ~3 ciclos |
| Reset global | ~5 ns (O(1)) |
| Chamada fn (inline) | 0 ciclos |
| Dispatch vtbl | ~2-3 ciclos |
| Pool alloc (≤64B) | O(1) |
