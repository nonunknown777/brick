# Próximo Passo - Debugger
# Next Step - Debugger

## Imediato
## Immediate

Nada — task completa. Descoberta dinâmica testada no GDB 17 batch.
Nothing — task complete. Dynamic discovery tested in GDB 17 batch.

## Pendências
## Pending

Nenhuma.
None.

## Para release futura
## For future release

- Hot reload integration com debugger (parar em swap)
- Testes automatizados de debug (GDB batch mode no CI)
- Suporte a watchpoints em blocos específicos
- Cache nos pretty-printers para performance com muitos blocos
- Se o REPL evaluate dos approaches não funcionar em algumas versões do cppdbg, melhorar fallback
- Verificar se `_current_block` mostra 8MB (deve ser ponteiro reatribuído, não bloco próprio)

- Hot reload integration with debugger (stop on swap)
- Automated debug tests (GDB batch mode in CI)
- Watchpoint support on specific blocks
- Cache in pretty-printers for performance with many blocks
- If REPL evaluate of approaches doesn't work in some cppdbg versions, improve fallback
- Verify if `_current_block` shows 8MB (should be reassigned pointer, not own block)
