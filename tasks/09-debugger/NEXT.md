# Próximo Passo - Debugger
# Next Step - Debugger

## Feature: Tipos Explícitos de Largura Fixa
## Feature: Explicit Fixed-Width Types

### Pretty-printers

Atualizar os pretty-printers GDB (Python) para reconhecer e exibir
corretamente os novos tipos C:
- `uint8_t` / `uint16_t` / `uint32_t` / `uint64_t`
- `int8_t` / `int16_t` / `int32_t` / `int64_t`
- `float` / `double`
- `size_t` / `ptrdiff_t`

A maioria já tem suporte nativo no GDB, mas verificar se os
pretty-printers customizados do projeto (BlockCtx, BrickString)
não são afetados.

### Webview Memory View

Se o webview mostra valores de variáveis, atualizar display types
para incluir os novos tipos primitivos.

## C Interop — verificado
## C Interop — verified

- [x] `#line` directives: include/link/extern não geram código debuggable → sem interferência
- [x] Pretty-printers: nenhum novo tipo introduzido pelo interop — GDB lida nativamente com C types
- [x] GDB commands: `info blocks`, `block`, `block-watch`, `blocks-list` funcionam inalterados
- [x] Webview memory: block discovery via DAP não é afetado por chamadas a funções C
- [x] Config: `.mc` → `.brc`, path `.gdbinit` portável

- [x] `#line` directives: include/link/extern produce no debuggable code → no interference
- [x] Pretty-printers: no new types introduced by interop — GDB handles native C types natively
- [x] GDB commands: `info blocks`, `block`, `block-watch`, `blocks-list` work unchanged
- [x] Webview memory: block discovery via DAP unaffected by C function calls
- [x] Config: `.mc` → `.brc`, `.gdbinit` path portable

## Pendências anteriores
## Previous pending

- Hot reload integration com debugger (parar em swap)
- Testes automatizados de debug (GDB batch mode no CI)
- Suporte a watchpoints em blocos específicos
- Cache nos pretty-printers para performance com muitos blocos
- Se o REPL evaluate dos approaches não funcionar em algumas versões do cppdbg, melhorar fallback
- Verificar se `_current_block` mostra 8MB (deve ser ponteiro reatribuído, não bloco próprio)
