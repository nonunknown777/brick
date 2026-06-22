# Estado Atual - Tester/Optimizer
# Current State - Tester/Optimizer

Sessão: 6 (testes gerais + documentação shared-context)
Session: 6 (general tests + shared-context docs)

Progresso: 100%
Progress: 100%

Última ação: Testes completos (unit+integration+benchmarks), shared-context.md atualizado
Last action: Full test suite (unit+integration+benchmarks), shared-context.md updated

## Realizado nesta sessão
## Completed this session

### Integração Visualizer (codegen + runtime)
### Visualizer Integration (codegen + runtime)

- **Codegen**: `block_register(name, "name")` emitido após cada `block_create_bytes()` em `gen_block_init()`
- **Codegen**: `block_shm_export()` emitido ao final de `__meta_c_init()`
- **Runtime**: auto-export no `block_alloc_aligned()` (throttle 1/16 allocs) para manter attach mode atualizado
- **Runtime**: `block_shm_export()` em `block_reset()` para refleter resets no visualizer
- **Build**: `-DMETA_C_TRACK_BLOCKS` passado ao gcc em `meta-c build`/`meta-c run`
- **Visualizer**: loop principal simplificado (removida duplicação de `block_snapshot`)
- **Test fix**: stubs de `block_register`, `block_shm_export` no test_codegen para compilar com `-Werror`

- **Codegen**: `block_register(name, "name")` emitted after each `block_create_bytes()` in `gen_block_init()`
- **Codegen**: `block_shm_export()` emitted at the end of `__meta_c_init()`
- **Runtime**: auto-export in `block_alloc_aligned()` (throttle 1/16 allocs) to keep attach mode updated
- **Runtime**: `block_shm_export()` in `block_reset()` to reflect resets in visualizer
- **Build**: `-DMETA_C_TRACK_BLOCKS` passed to gcc in `meta-c build`/`meta-c run`
- **Visualizer**: simplified main loop (removed `block_snapshot` duplication)
- **Test fix**: stubs for `block_register`, `block_shm_export` in test_codegen to compile with `-Werror`

### CLI --visualize / --attach
### CLI --visualize / --attach

- `meta-c --visualize` — inicia TUI ncurses em embedded mode (lê `block_snapshot`)
- `meta-c --attach <pid>` — attacha processo rodando, lê `/tmp/meta-c-mem-<pid>.bin`
- `print_usage()` atualizado com os novos comandos
- `#include "memvis.h"` + guarda `#ifdef META_C_TRACK_BLOCKS`

- `meta-c --visualize` — starts ncurses TUI in embedded mode (reads `block_snapshot`)
- `meta-c --attach <pid>` — attaches to running process, reads `/tmp/meta-c-mem-<pid>.bin`
- `print_usage()` updated with new commands
- `#include "memvis.h"` + `#ifdef META_C_TRACK_BLOCKS` guard

### Release (build-release.sh)
### Release (build-release.sh)

- ncurses documentado como dependência nos metadados do release
- Visualizador incluso no binário `meta-c` (via `libmeta_visualizer.a` + `-lncurses`)

- ncurses documented as dependency in release metadata
- Visualizer included in `meta-c` binary (via `libmeta_visualizer.a` + `-lncurses`)

### Testes
### Tests

- 118/118 testes passando (0 falhas)
- `scons test` — lexer, parser, runtime, codegen, window lib, hot reload
- `meta-c build examples/hello.mc` → executável funcional com shm export
- `meta-c --attach <pid>` → TUI mostra blocos reais, refresh 500ms

- 118/118 tests passing (0 failures)
- `scons test` — lexer, parser, runtime, codegen, window lib, hot reload
- `meta-c build examples/hello.mc` → functional executable with shm export
- `meta-c --attach <pid>` → TUI shows real blocks, 500ms refresh

### Implementação: Runtime Improvements para Tipos Explícitos
### Implementation: Runtime Improvements for Explicit Types

1. **io.h/io.c** — 12 funções type-specific (`io_print_u8..io_print_isize`)
   com format specifiers PRI exatos. Elimina widening implícito
   (antes `u8` virava `(long long)`, agora `PRIu8` direto).
2. **MetaCString.len** — `int64_t` → `size_t` (consistência com runtime)
3. **block_memory.c** — `block_alloc()` com alinhamento adaptativo:
   size→align(1), 2→(2), 4→(4), 8+→(8). Zero waste pra u8/u16/u32/f32.
4. **codegen.cpp** — `gen_print_single` dispatch direto pra funções
   type-specific; `gen_printf_call` usa PRI macros; inclui `<inttypes.h>`.
5. **test_codegen.cpp** — Atualizado checks pra `io_print_i32/f32/u8`
6. **test_runtime.c** — Atualizado test_alignment pra alinhamento adaptativo
7. **79/79 testes passando** (antes 76, +3 novos de fixed-width types)

### Sessão 6 — Testes Gerais + shared-context.md
### Session 6 — General Tests + shared-context.md

1. **Testes unitários**: lexer 29, parser 6, codegen 79, runtime 14, hot reload 5, window 15, window HR 3 = **151 testes passando (0 falhas)**
2. **Testes de integração**: 5/5 passando (compile + gcc + run .mc → binary)
3. **Benchmarks**: compilação 100 structs em 5ms; bump alloc 1M allocs 64B em 2ms (19.5× mais rápido que malloc)
4. **shared-context.md**: documentação completa dos novos tipos de largura fixa (u8..u64, i8..i64, f32/f64, usize/isize, aliases, sufixos, regras de tipo)
5. **Verificação de estado**: todas as tasks 01-09 lidas — interfaces consistentes, sem blockers

### Sessão 7 — Documentação + Site + Exemplos
### Session 7 — Documentation + Site + Examples

1. **README.md**: reescrito em inglês — `meta-c` CLI (sem gcc manual), exemplos com interface + tipos fixos, GitHub Pages link fixo (`nonunknown777.github.io/meta-c`), repo URL `nonunknown777/meta-c`
2. **README.pt-BR.md**: tradução completa em português
3. **docs/GETTING_STARTED.md**: convertido para inglês, usa `meta-c build`/`meta-c run` em vez de gcc manual
4. **docs/LANGUAGE.md**: seção de tipos atualizada com fixed-width types (i8..i64, u8..u64, f32/f64, usize/isize), aliases, sufixos, regras de tipo
5. **docs/ARCHITECTURE.md**: convertido para inglês
6. **docs/index.html**: links apontam para GitHub Wiki (`/wiki/...`) em vez de `.md` local, Quick Start usa `build/meta-c run`, adicionada seção fixed-width types
7. **examples/types_and_interfaces.mc**: novo exemplo completo com u32, f32, f64, i32, u8, interfaces (Drawable, Area), laços while, reset de blocos
8. **wiki/Home.md**: exemplo atualizado com tipos fixos + interface
9. **wiki/Getting-Started.md**: usa `build/meta-c` CLI em vez de gcc manual
10. **wiki/Language-Reference.md**: seção de tipos reescrita com fixed-width types + mapping to C atualizado
11. **wiki/Hot-Reload.md**: compilação via `meta-c build` como primário
12. **wiki/Performance.md**: benchmark numbers atualizados com resultados reais, release build via `meta-c build --release`

### Planejamento: Tipos Explícitos de Largura Fixa
### Planning: Explicit Fixed-Width Types

**Decisões de design:**

| Item | Decisão |
|---|---|
| Nomenclatura | Padrão: u8/u16/u32/u64, i8/i16/i32/i64, f32/f64, usize/isize |
| `int`/`float` | Apelidos pra `i32`/`f32` (quebra compat: antes eram i64/f64) |
| `char`/`byte` | Apelidos pra `u8` |
| `short` | Apelido pra `i16` |
| `long` | Apelido pra `i64` |
| Literal sem sufixo | Tipo flexível — se couber no destino, permite |
| Literal com sufixo | `42u8`, `3.14f32`, `42usz` |
| Overflow compile-time | Erro |
| Widening (i8→i16) | ✅ Permite |
| Narrowing (i64→i32) | ❌ Erro |
| Signed↔Unsigned mesmo rank | ❌ Erro |
| Int→Float | Int promove pra float sempre |
| Expressões mistas | Promoção ao tipo que couber ambos operandos |

**Tasks envolvidas:**

| Task | Ação |
|---|---|
| **01-lexer** | Novos tokens + sufixos de literal |
| **02-parser** | `is_type_keyword()` + `literal_type` em IntLiteral/FloatLiteral |
| **03-codegen** | `map_type()` + `can_assign()` regras + `promote_types()` + inferência literal |
| **04-runtime** | ✅ Implementado — type-specific io_print, MetaCString.len size_t, alignment adaptativo |
| **05-hotreload** | Sem mudanças |
| **06-visualizer** | Sem mudanças |
| **07-builder** | Sem mudanças |
| **08-vscoder** | Syntax highlighting + language service |
| **09-debugger** | Pretty-printers para novos tipos C |
| **10-tester** | Testes + docs |

NEXT.md atualizados: 01, 02, 03, 08, 09

### Estado final das tasks:
### Final task status:

- 01-lexer:   90% — planejada feature tipos explícitos (NEXT.md)
- 02-parser:  95% — planejada feature tipos explícitos (NEXT.md)
- 03-codegen:100% — planejada feature tipos explícitos (NEXT.md)
- 04-runtime:100% — runtime improvements implementados (io_print type-specific, MetaCString size_t, alignment adaptativo)
- 05-hotreload:100% — sem mudanças
- 06-visualizer:100% — sem mudanças
- 07-builder:100% — sem mudanças
- 08-vscoder: 95% — planejada feature tipos explícitos (NEXT.md)
- 09-debugger:100% — planejada feature tipos explícitos (NEXT.md)

- 01-lexer:   90% — explicit types feature planned (NEXT.md)
- 02-parser:  95% — explicit types feature planned (NEXT.md)
- 03-codegen:100% — explicit types feature planned (NEXT.md)
- 04-runtime:100% — runtime improvements done (type-specific io_print, MetaCString size_t, adaptive alignment)
- 05-hotreload:100% — no changes
- 06-visualizer:100% — no changes
- 07-builder:100% — no changes
- 08-vscoder: 95% — explicit types feature planned (NEXT.md)
- 09-debugger:100% — explicit types feature planned (NEXT.md)
