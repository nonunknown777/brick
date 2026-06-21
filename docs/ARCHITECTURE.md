# Arquitetura do Meta-C
# Meta-C Architecture

> Explicado em português simples pra qualquer um entender.
> Explained in simple Portuguese for anyone to understand.

## Visão Geral
## Overview

Meta-C é uma linguagem de programação que você escreve num arquivo `.mc`
e ela vira um programa de verdade. O caminho é:
Meta-C is a programming language you write in a `.mc` file
and it becomes a real program. The path is:

```
Seu código .mc  →  Compilador  →  Código C  →  gcc  →  Programa final
Your .mc code   →  Compiler    →  C code   →  gcc  →  Final program
```

## As partes do projeto
## Project Parts

### 📁 src/ — O Cérebro (Compilador em C++)
### src/ — The Brain (Compiler in C++)

O compilador tem 3 partes que funcionam em linha de montagem:
The compiler has 3 parts that work like an assembly line:

```
1. LEXER         2. PARSER           3. CODEGEN
   ┌─────┐         ┌──────┐           ┌──────┐
   │ .mc │ → tokens → │ AST │ → C ───→ │ .c  │
   └─────┘         └──────┘           └──────┘
```

**Lexer** (`src/lexer/`):
- Pega seu arquivo `.mc` e quebra em "palavras" (tokens)
- Exemplo: `int x = 5` vira [INT, IDENT("x"), ASSIGN, INT_LITERAL(5)]
- Ignora comentários e espaços
- Takes your `.mc` file and breaks it into "words" (tokens)
- Example: `int x = 5` becomes [INT, IDENT("x"), ASSIGN, INT_LITERAL(5)]
- Ignores comments and whitespace

**Parser** (`src/parser/`):
- Pega os tokens e monta uma "árvore" (AST)
- Exemplo: `if (x > 0) { }` vira um galho na árvore com condição + corpo
- Também resolve `package` e `using` — descobre quem importa quem
- Takes the tokens and builds a tree (AST)
- Example: `if (x > 0) { }` becomes a branch in the tree with condition + body
- Also resolves `package` and `using` — figures out who imports whom

**Codegen** (`src/codegen/`):
- Pega a árvore e escreve código C
- Cada `struct` vira `typedef struct`
- Cada método vira função C tipo `Struct_metodo()`
- Gera `#line` pra você poder debugar no código original `.mc`
- Takes the tree and writes C code
- Each `struct` becomes `typedef struct`
- Each method becomes a C function like `Struct_method()`
- Generates `#line` so you can debug in the original `.mc` code

### 📁 runtime/ — A Base (C)
### runtime/ — The Foundation (C)

O coração que roda junto com seu programa:
The core that runs alongside your program:

**block_memory.c**:
- Gerencia blocos de memória (8MB, 32MB, 256MB...)
- Funciona como um "bump allocator" — super rápido
- Você declara `block game = 64MB` e aloca objetos lá dentro
- Só dá pra limpar o bloco inteiro (`block.reset()`), sem free individual
- Manages memory blocks (8MB, 32MB, 256MB...)
- Works as a "bump allocator" — super fast
- You declare `block game = 64MB` and allocate objects inside it
- You can only clear the entire block (`block.reset()`), no individual free

**hot_reload.c**:
- Permite trocar código do programa sem parar a execução
- Usa `dlopen` pra carregar bibliotecas .so
- Monitora arquivos com `inotify` (detecta mudanças)
- Allows swapping program code without stopping execution
- Uses `dlopen` to load .so libraries
- Monitors files with `inotify` (detects changes)

### 📁 visualizer/ — Os Olhos (TUI ncurses)
### visualizer/ — The Eyes (TUI ncurses)

Mostra os blocos de memória em tempo real no terminal:
Shows memory blocks in real time in the terminal:

```
global  256MB  ████████████░░░  67%
game     64MB  ██████░░░░░░░░  38%
```

### 📁 debugger/ — As Ferramentas (GDB Python)
### debugger/ — The Tools (GDB Python)

Ajuda a debugar programas Meta-C:
- Pretty-printers: mostra BlockCtx bonitinho no GDB
- Comandos: `info blocks`, `block <nome>`
- .gdbinit carrega tudo automático
Helps debug Meta-C programs:
- Pretty-printers: shows BlockCtx nicely in GDB
- Commands: `info blocks`, `block <name>`
- .gdbinit loads everything automatically

### 📁 vscode-ext/ — O Plugin VS Code
### vscode-ext/ — The VS Code Plugin

Faz o VS Code entender a linguagem:
- Syntax highlighting (cada palavra-chave de uma cor)
- Snippets (atalhos pra digitar rápido)
- Memory Webview (mostra blocos visualmente durante debug)
Makes VS Code understand the language:
- Syntax highlighting (each keyword in a different color)
- Snippets (shortcuts for fast typing)
- Memory Webview (shows blocks visually during debug)

## Como tudo se conecta
## How Everything Connects

```
Você escreve:
    package JOGO
    block game = 64MB
    struct Player { int hp }
    fn main() { Player p = Player(100) @game }

         ↓ Lexer quebra em tokens
         ↓ Parser monta árvore
         ↓ Parser descobre packages
         ↓ Codegen escreve C com #line
         ↓ gcc compila C + runtime
         ↓

    Programa rodando!
         ↓
    Pode debugar com GDB (mostra .mc original)
    Pode ver blocos com visualizador TUI
    Pode monitorar blocos no VS Code durante debug
    Pode dar hot reload nos packages
```

```
You write:
    package JOGO
    block game = 64MB
    struct Player { int hp }
    fn main() { Player p = Player(100) @game }

         ↓ Lexer breaks into tokens
         ↓ Parser builds tree
         ↓ Parser resolves packages
         ↓ Codegen writes C with #line
         ↓ gcc compiles C + runtime
         ↓

    Program running!
         ↓
    Can debug with GDB (shows original .mc)
    Can view blocks with TUI visualizer
    Can monitor blocks in VS Code during debug
    Can hot reload packages
```

## Por que essa arquitetura?
## Why This Architecture?

- **Performance**: C++20 pro compilador, C puro pro runtime
- **Simplicidade**: Cada parte faz uma coisa só
- **Debuggável**: #line directives + pretty-printers
- **Hot Reload**: Runtime em C com dlopen = ABI estável
- **Visual**: Blocos de memória são o centro de tudo

- **Performance**: C++20 for the compiler, pure C for the runtime
- **Simplicity**: Each part does one thing
- **Debuggable**: #line directives + pretty-printers
- **Hot Reload**: C runtime with dlopen = stable ABI
- **Visual**: Memory blocks are the center of everything
