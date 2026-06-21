# Task: Builder (Meta-C)

## Função
## Role

Você é o especialista em BUILD do Meta-C.
Responsabilidade: sistema de build SCons para o compilador e runtime.
You are the BUILD specialist for Meta-C.
Responsibility: SCons build system for the compiler and runtime.

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md
2. ANTES DE SAIR: atualize estado
3. O SConstruct principal está na raiz do projeto

1. ON START: read STATE.md, NEXT.md and shared-context.md
2. BEFORE LEAVING: update state
3. The main SConstruct is at the project root

## SConstruct (raiz)
## SConstruct (root)

O SConstruct já foi criado. Sua função é:
- Manter e evoluir o build à medida que novos arquivos são adicionados
- Garantir dependências corretas entre módulos
- Suportar profiles: release, debug, sanitize

The SConstruct has already been created. Its role is:
- Maintain and evolve the build as new files are added
- Ensure correct dependencies between modules
- Support profiles: release, debug, sanitize

## Comandos
## Commands

```bash
scons                    # build release (-O3)
scons profile=debug      # debug (-g -O0)
scons profile=sanitize   # ASAN+UBSan
scons test               # build + roda testes
scons -j$(nproc)         # build paralelo
```

```bash
scons                    # release build (-O3)
scons profile=debug      # debug build (-g -O0)
scons profile=sanitize   # ASAN+UBSan
scons test               # build + run tests
scons -j$(nproc)         # parallel build
```

## Profiles
## Profiles

| Profile   | CXXFLAGS                        | Uso               |
|-----------|----------------------------------|-------------------|
| release   | -std=c++20 -O3 -Wall             | produção          |
| debug     | -std=c++20 -g -O0 -DDEBUG        | desenvolvimento   |
| sanitize  | -std=c++20 -g -O1 -fsanitize=... | caça-bugs         |

| Profile   | CXXFLAGS                        | Usage             |
|-----------|----------------------------------|-------------------|
| release   | -std=c++20 -O3 -Wall             | production        |
| debug     | -std=c++20 -g -O0 -DDEBUG        | development       |
| sanitize  | -std=c++20 -g -O1 -fsanitize=... | bug hunting       |

## Módulos
## Modules

- meta_shared: tipos compartilhados (interface)
- meta_lexer: biblioteca do lexer
- meta_parser: biblioteca do parser
- meta_codegen: biblioteca do codegen
- meta_runtime: biblioteca runtime em C
- meta_visualizer: TUI ncurses
- meta-c: CLI principal

- meta_shared: shared types (interface)
- meta_lexer: lexer library
- meta_parser: parser library
- meta_codegen: codegen library
- meta_runtime: runtime library in C
- meta_visualizer: ncurses TUI
- meta-c: main CLI

## Responsabilidades
## Responsibilities

- Adicionar novos módulos ao SConstruct conforme surgirem
- Garantir que `scons test` rode todos os testes
- Detecção automática de compilador (gcc vs clang)
- Cross-compilação preparada pra futuro (flags extras)
- Instalação via `scons --prefix=/usr install`
- Caching de objetos compilados

- Add new modules to SConstruct as they appear
- Ensure `scons test` runs all tests
- Automatic compiler detection (gcc vs clang)
- Cross-compilation ready for the future (extra flags)
- Installation via `scons --prefix=/usr install`
- Compiled object caching
