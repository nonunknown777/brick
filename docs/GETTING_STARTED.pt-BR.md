# Primeiros Passos com Brick

> Guia rápido pra qualquer pessoa que queira usar ou contribuir com a linguagem.

## O Que Você Precisa

- **Linux** (qualquer distro)
- **g++** com suporte a C++20 (GCC >= 11 ou Clang >= 14)
- **gcc** pra compilar o código gerado
- **SCons** (`pip install scons`)
- **ncurses** pro visualizador (opcional)

## Compilar o Compilador

```bash
git clone https://github.com/nonunknown777/brick.git
cd brick
scons                     # build release / release build
scons profile=debug       # build debug / debug build
```

> O binário `brick` vai ficar em `build/brick`.

## Compilar e Rodar um Programa Brick

### Forma mais rápida

```bash
brick run examples/hello.brc
```

> Isso compila `.brc` → C → binário e já roda tudinho de uma vez.

### Build pra binário

```bash
brick build examples/hello.brc -o hello
./hello
```

O comando `brick build` cuida do pipeline completo:

1. Compila `.brc` pra C
2. Junta a runtime (alocador de blocos, I/O, hot reload)
3. Roda `gcc -O3` e gera um binário independente

### Compilar só pra C

```bash
brick examples/hello.brc -o hello.c
```

> Útil se você quiser dar uma olhada no código C gerado.

### Modo release (sem overhead de rastreamento)

```bash
brick build examples/hello.brc --release -o hello
./hello
```

> Tira o overhead de rastreamento pra ter performance máxima (sem suporte ao visualizador).

## Rodar Testes

```bash
scons test                # testes unitários / unit tests
tests/test_integration.sh # testes de integração (.brc -> compila -> roda) / integration tests (.brc -> compile -> run)
```

## Visualizar Memória

```bash
brick --visualize examples/hello.brc   # compila, roda e mostra TUI ao vivo / compile, run, show live TUI
brick --attach <pid>                  # conecta visualizador a processo rodando / attach visualizer to running process
```

## Conceitos Principais

1. **Tudo em blocos**: sua memória vive em blocos que você declara
2. **Sem stack**: zero variáveis na stack do C (tudo vai pra blocos)
3. **Bump allocator**: alocação super rápida (só avança um ponteiro)
4. **Reset, não free**: limpa o bloco inteiro, nunca objetos individuais
5. **Hot reload**: troca o código sem parar o programa
6. **Tipos de largura fixa**: i8/i16/i32/i64, u8/u16/u32/u64, f32/f64, usize/isize
7. **Diretivas #line**: depura no código .brc original, não no C gerado

## Estrutura do Projeto

| Diretório | Conteúdo |
|-----------|----------|
| `src/` | Compilador (Lexer, Parser, Codegen) em C++20 |
| `runtime/` | Alocador de blocos + hot reload + IO (C) |
| `visualizer/` | TUI ncurses pra visualização de memória ao vivo |
| `debugger/` | GDB pretty-printers e comandos customizados (Python) |
| `vscode-ext/` | Extensão VS Code (highlight, LSP, visualizador de memória) |
| `tests/` | Testes unitários e de integração |
| `examples/` | Programas .brc de exemplo |
| `docs/` | Site do GitHub Pages |
| `wiki/` | Conteúdo do GitHub Wiki |
| `tasks/` | Divisão das tarefas de desenvolvimento (01-11) |
| `benchmarks/` | Benchmarks de performance |

## Abrir uma Task (pra contribuidores)

> Cada pasta em `tasks/` tem um `run.sh` que abre o opencode focado naquela task.

> Cada task tem `AGENTS.md` (instruções pra IA) e `STATE.md` (onde parou).
