# Estado Atual
# Current State

Sessão: 5 (C Interop verification)
Session: 5 (C Interop verification)

Progresso: 100%
Progress: 100%

Próximo passo: Pendente — ver NEXT.md
Next step: Pending — see NEXT.md

Última ação: Verificação de compatibilidade com C Interop
Last action: C Interop compatibility verification

Pendências: NOVA FEATURE — Tipos Explícitos de Largura Fixa (ver NEXT.md)
Pending: NEW FEATURE — Explicit Fixed-Width Types (see NEXT.md)

## Features verificadas (C Interop batch)
## Features verified (C Interop batch)

- [x] `#line` directives funcionam com chamadas `extern fn` (via `gen_statement` → `emit_line`)
- [x] `#line` para `include`/`link`/`extern fn` declarações não geram código debuggable — ok
- [x] Pretty-printers existentes cobrem todos os tipos: `BlockCtx`, `BrickString`, ponteiros alocados em bloco
- [x] Nenhum novo tipo introduzido pelo C interop — tipos C nativos (int, double, char*) são nativos do GDB
- [x] Comandos GDB (`info blocks`, `block`, `block-watch`, `blocks-list`) funcionam com C interop
- [x] Webview memory view (DAP-based block discovery) não é afetado
- [x] `.vscode/launch.json` e `tasks.json` atualizados: `.mc` → `.brc`, path do `.gdbinit` portável

- [x] `#line` directives work with `extern fn` calls (via `gen_statement` → `emit_line`)
- [x] `#line` for `include`/`link`/`extern fn` declarations produce no debuggable code — ok
- [x] Existing pretty-printers cover all types: `BlockCtx`, `BrickString`, block-allocated pointers
- [x] No new types introduced by C interop — native C types (int, double, char*) are native to GDB
- [x] GDB commands (`info blocks`, `block`, `block-watch`, `blocks-list`) work with C interop
- [x] Webview memory view (DAP-based block discovery) is unaffected
- [x] `.vscode/launch.json` and `tasks.json` updated: `.mc` → `.brc`, `.gdbinit` path portable

## Corrigido nesta sessão
## Fixed this session

### C Interop compatibility
### Compatibilidade C Interop

- First pass do codegen (includes/links/externs) não interfere com `#line` tracking
- `gen_extern_prototype()` existe mas não é chamada (dead code) — protótipos vêm dos headers C incluídos
- `map_type()` agora suporta `*T` ponteiros: `*u8` → `char*`, `*i32` → `int32_t*`, `*void` → `void*`
- `*T` no type checker: `is_type_known("*T")` recursivo, `can_assign` com regras de ponteiro
- `String` → `*u8` auto-conversão: `.data` passado automaticamente em chamadas `extern fn`

- Codegen first pass (includes/links/externs) does not interfere with `#line` tracking
- `gen_extern_prototype()` exists but is never called (dead code) — prototypes come from included C headers
- `map_type()` now supports `*T` pointers: `*u8` → `char*`, `*i32` → `int32_t*`, `*void` → `void*`
- `*T` in type checker: `is_type_known("*T")` recursive, `can_assign` with pointer rules
- `String` → `*u8` auto-conversion: `.data` passed automatically in `extern fn` calls

### Config files (.mc → .brc)
### Arquivos de config (.mc → .brc)

- `.vscode/launch.json`: path do `.gdbinit` corrigido de absoluto para `${workspaceFolder}`
- `.vscode/launch.json`: todas strings `detail` e `preLaunchTask` migradas de `.mc` para `.brc`
- `.vscode/tasks.json`: todas labels e strings migradas de `.mc` para `.brc`

- `.vscode/launch.json`: `.gdbinit` path fixed from absolute to `${workspaceFolder}`
- `.vscode/launch.json`: all `detail` and `preLaunchTask` strings migrated from `.mc` to `.brc`
- `.vscode/tasks.json`: all labels and strings migrated from `.mc` to `.brc`

## Implementado (sessão anterior: GDB 17 compat + dynamic discovery)
## Implemented (previous session: GDB 17 compat + dynamic discovery)

- GDB 17 API fixes: `global_block` property, `-t` flag, event-based stop handler
- Dynamic block discovery (no hardcoded list)
- Comando GDB `blocks-list`
- Real-time: webview atualiza a cada pause/step/breakpoint
- Frontend sem demo data

- GDB 17 API fixes: `global_block` property, `-t` flag, event-based stop handler
- Dynamic block discovery (no hardcoded list)
- GDB command `blocks-list`
- Real-time: webview updates on every pause/step/breakpoint
- Frontend without demo data

## Implementado (sessões anteriores)
## Implemented (previous sessions)

- #line directives no codegen
- GDB pretty-printers (BlockCtx, BrickString, BlockAlloc)
- GDB custom commands (info blocks, block, block-watch)
- .gdbinit com auto-load
- VS Code debug config (launch.json + tasks.json)
- VS Code webview panel (Brick Memory)
- Block Data Provider para debug

- #line directives in codegen
- GDB pretty-printers (BlockCtx, BrickString, BlockAlloc)
- GDB custom commands (info blocks, block, block-watch)
- .gdbinit with auto-load
- VS Code debug config (launch.json + tasks.json)
- VS Code webview panel (Brick Memory)
- Block Data Provider for debugging
