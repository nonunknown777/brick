# Próximo Passo — Visualizer
# Next Step — Visualizer

## Imediato
## Immediate

Integrar com o resto do sistema:
1. Chamar `block_register()` no codegen ao criar blocos com `block_create()`
2. Chamar `block_shm_export()` periodicamente no runtime (ou antes de cada `getch` no visualizer)
3. Expor comando CLI `meta-c --visualize` ou `meta-c --attach <pid>`

Integrate with the rest of the system:
1. Call `block_register()` in codegen when creating blocks with `block_create()`
2. Call `block_shm_export()` periodically in runtime (or before each `getch` in visualizer)
3. Expose CLI command `meta-c --visualize` or `meta-c --attach <pid>`

## Futuro
## Future

- Teste automatizado do TUI (script expect)
- Botão de detalhamento (→) com histórico de alocações
- Suporte a pipe nomeado em vez de shared memory

- Automated TUI test (expect script)
- Detail button (→) with allocation history
- Named pipe support instead of shared memory
