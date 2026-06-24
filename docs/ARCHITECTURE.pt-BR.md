# Arquitetura do Brick

> Explicado de forma simples para qualquer um entender.

## Visão Geral

Brick é uma linguagem de programação que você escreve num arquivo `.brc` e ela vira um programa de verdade. O caminho é:

```
Seu código .brc → Compilador → Código C →  gcc  →  Programa final
```

## Partes do Projeto

### src/ — O Cérebro (Compilador em C++20)

O compilador tem 4 partes em uma linha de montagem:

```
1. LEXER         2. PARSER           3. SISTEMA MACRO   4. CODEGEN
   ┌─────┐         ┌──────┐           ┌──────────┐        ┌──────┐
   │ .brc │ → tokens → │ AST │ → expand → │ .c  │
   └─────┘         └──────┘           └──────────┘        └──────┘
                                            ↓
                                    collect + build + expand
```

**Lexer** (`src/lexer/`):
- Pega seu arquivo `.brc` e quebra em tokens (pedaços)
- Exemplo: `int x = 5` vira [INT, IDENT("x"), ASSIGN, INT_LITERAL(5)]
- Ignora comentários e espaços em branco

**Parser** (`src/parser/`):
- Pega os tokens e monta uma AST (Árvore Sintática Abstrata)
- Exemplo: `if (x > 0) { }` vira um galho com condição + corpo
- Também resolve `package` e `using` — descobre as importações
- Lida com declarações `macro`, `build` e `emit`

**Sistema de Macros** (`src/parser/` — `macro_expander.cpp`, `build_eval.cpp`):
- Coleta todas as declarações `macro` em uma tabela
- Avalia blocos `build {}` em tempo de compilação, executando seus `emit`
- Expande cada `macro_call(...)` clonando o corpo e substituindo parâmetros `$`
- Gera nomes únicos para variáveis que começam com `__` (gensym)
- Detecta recursão infinita (máximo de 64 níveis)

**Codegen** (`src/codegen/`):
- Anda pela AST expandida (sem macros) e escreve código C
- Cada `struct` vira `typedef struct`
- Cada método vira `StructName_method()`
- Type-checking: valida tipos, dá erro se algo não bater
- Gera diretivas `#line` pra você debugar no `.brc`, não no C

### runtime/ — A Fundação (C)

O núcleo que roda junto com seu programa:

**block_memory.c**:
- Gerencia blocos de memória (8MB, 32MB, 256MB...)
- Usa um bump allocator — super rápido (~3 ciclos de CPU por alocação)
- Declare `block game = 64MB` e aloque objetos dentro dele
- Limpe o bloco inteiro com `block.reset()` — sem free individual
- Suporte a freeze/thaw para hot reload

**io.c**:
- Funções de print específicas por tipo: `io_print_i8`, `io_print_u32`, `io_print_f64`, etc.
- Usa especificadores de formato PRI exatos de `<inttypes.h>`
- Print formatado com argumentos posicionais: `print("x={0}", 42)`

**hot_reload.c**:
- Troca código sem parar seu programa
- Usa `dlopen` para carregar bibliotecas .so
- Monitora arquivos com `inotify` (detecta mudanças)
- Troca atômica de ponteiros de função

### visualizer/ — Os Olhos (TUI ncurses)

Mostra blocos de memória em tempo real no terminal:

```
global  256MB  ████████████░░░  67%
game     64MB  ██████░░░░░░░░  38%
```

### debugger/ — As Ferramentas (GDB Python)

Ajuda a debugar programas Brick:
- Pretty-printers: mostra BlockCtx bonitinho no GDB
- Comandos: `info blocks`, `block <name>`
- .gdbinit carrega tudo automaticamente

### vscode-ext/ — A Extensão VS Code

Faz o VS Code entender a linguagem:
- Syntax highlighting (destaque de sintaxe)
- Servidor LSP (completar, hover, diagnósticos)
- Memory Webview (mostra blocos visualmente durante debug)

## Como Tudo se Conecta

```
Você escreve:
    package GAME
    block game = 64MB
    struct Player { int hp }
    fn main() { Player p = Player(100) @game }

         ↓ Lexer quebra em tokens
         ↓ Parser monta árvore + resolve pacotes
         ↓ Sistema de macros coleta, executa build e expande
         ↓ Codegen escreve C com diretivas #line
         ↓ gcc compila C + runtime
         ↓

    Programa rodando!
         ↓
    Debug com GDB (mostra fonte .brc)
    Veja blocos com visualizador TUI
    Monitore no VS Code memory view
    Hot reload de pacotes
```

## Por Que Essa Arquitetura?

- **Performance**: C++20 no compilador, C puro no runtime, gcc -O3 no binário
- **Simplicidade**: Cada parte faz uma coisa bem feita
- **Debuggável**: Diretivas #line + pretty-printers + visualizador TUI
- **Hot Reload**: Runtime C com dlopen = ABI estável
- **Foco em memória**: Blocos são o centro de tudo — visíveis, mensuráveis, controláveis
