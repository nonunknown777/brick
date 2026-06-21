# Começando com Meta-C
# Getting Started with Meta-C

> Guia rápido pra quem quer usar ou contribuir com a linguagem.
> Quick guide for anyone who wants to use or contribute to the language.

## O que você precisa
## What You Need

- **Linux** (Arch recomendado, mas qualquer um serve)
- **g++** com suporte a C++20
- **gcc** pra compilar o código gerado
- **SCons** (`sudo pacman -S scons` no Arch)
- **ncurses** pro visualizador (`sudo pacman -S ncurses`)
- **Konsole** (terminal do KDE) — ou adapta os scripts pro seu terminal
- **Linux** (Arch recommended, but any distro works)
- **g++** with C++20 support
- **gcc** to compile the generated code
- **SCons** (`sudo pacman -S scons` on Arch)
- **ncurses** for the visualizer (`sudo pacman -S ncurses`)
- **Konsole** (KDE terminal) — or adapt the scripts to your terminal

## Build do compilador
## Building the Compiler

```bash
git clone <url-do-repo> meta-c
cd meta-c
scons                     # build release
scons profile=debug       # build debug
```

O compilador `meta-c` vai estar em `build/meta-c`.
The `meta-c` compiler will be at `build/meta-c`.

## Compilar um programa Meta-C
## Compiling a Meta-C Program

```bash
# Compila .mc → .c
meta-c examples/hello.mc -o build/hello.c

# Compila o C gerado + runtime → programa final
gcc -O3 build/hello.c runtime/block_memory.c runtime/hot_reload.c -o build/hello -ldl

# Roda
./build/hello
```

Com debug:
With debug:

```bash
meta-c examples/hello.mc -o build/hello.c
gcc -g build/hello.c runtime/block_memory.c runtime/hot_reload.c -o build/hello -ldl
gdb ./build/hello
```

## Rodar testes
## Running Tests

```bash
scons test                # testes unitários / unit tests
tests/test_integration.sh # testes de integração / integration tests
```

## Abrir o workspace
## Opening the Workspace

```bash
./run-all.sh              # abre um Konsole com todas as tasks
                          # opens a Konsole with all tasks
```

Ou abrir uma task específica:
Or open a specific task:

```bash
./tasks/04-runtime/run.sh # só a task de runtime / runtime task only
```

## Estrutura do projeto pra contribuir
## Project Structure for Contributors

| Pasta | O que tem | Linguagem |
| Folder | Contents | Language |
|-------|-----------|:---------:|
| `src/` | Compilador (lexer, parser, codegen) | C++20 |
| `src/` | Compiler (lexer, parser, codegen) | C++20 |
| `runtime/` | Biblioteca de blocos + hot reload | C |
| `runtime/` | Block library + hot reload | C |
| `visualizer/` | TUI que mostra blocos | C++ (ncurses) |
| `visualizer/` | TUI that shows blocks | C++ (ncurses) |
| `debugger/` | GDB pretty-printers | Python |
| `vscode-ext/` | Plugin VS Code | JSON/JS/TS |
| `vscode-ext/` | VS Code extension | JSON/JS/TS |
| `tasks/` | 10 agents do opencode | .md |
| `tasks/` | 10 opencode agents | .md |
| `tests/` | Testes unitários e integração | C++/C/bash |
| `tests/` | Unit and integration tests | C++/C/bash |
| `examples/` | Código .mc de exemplo | Meta-C |
| `examples/` | Example .mc code | Meta-C |
| `docs/` | Documentação | .md |
| `docs/` | Documentation | .md |

## Abrindo uma task no opencode
## Opening a Task in opencode

Cada pasta em `tasks/` tem um `run.sh` que abre um terminal com o opencode
já focado naquela task. O `AGENTS.md` de cada task diz pra IA o que ela
precisa fazer. O `STATE.md` diz onde a task parou na última sessão.
Each folder in `tasks/` has a `run.sh` that opens a terminal with opencode
already focused on that task. The `AGENTS.md` of each task tells the AI what
it needs to do. `STATE.md` says where the task stopped in the last session.

## Conceitos principais
## Core Concepts

1. **Tudo em blocos**: sua memória vive em blocos que você declara
2. **Sem stack**: zero variáveis na pilha do C (tudo vai pra blocos)
3. **Bump allocator**: alocação super rápida (só aumenta um ponteiro)
4. **Reset, não free**: limpa o bloco inteiro, nunca objeto individual
5. **Hot reload**: troca código sem parar o programa
6. **#line directives**: debugar no código .mc original, não no C gerado

1. **Everything in blocks**: your memory lives in blocks you declare
2. **No stack**: zero variables on the C stack (everything goes to blocks)
3. **Bump allocator**: super fast allocation (just advances a pointer)
4. **Reset, not free**: clears the entire block, never individual objects
5. **Hot reload**: swap code without stopping the program
6. **#line directives**: debug in the original .mc code, not the generated C
