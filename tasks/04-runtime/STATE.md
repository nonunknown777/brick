# Estado Atual - Runtime
# Current State - Runtime

Sessão: 2026-06-18 + 2026-06-20 (atualizada pela task 10)
Session: 2026-06-18 + 2026-06-20 (updated by task 10)

Progresso: 100%
Progress: 100%

Última ação: Auto-export no block_alloc_aligned (throttle 1/16) e block_reset
Last action: Auto-export in block_alloc_aligned (throttle 1/16) and block_reset

## Realizado
## Completed

- runtime/io.h: API pública com MetaCString typedef + io_print_*
- runtime/io.c: implementação via printf (io_print_int, float, string, bool, char, newline, printf)
- extern "C" para compatibilidade C++
- Usa int64_t/double/MetaCString nos tipos
- `block_shm_export()` chamado automaticamente a cada 16 allocs em `block_alloc_aligned()`
- `block_shm_export()` chamado em `block_reset()` para manter attach mode atualizado
- 56/56 testes passando

- runtime/io.h: public API with MetaCString typedef + io_print_*
- runtime/io.c: implementation via printf (io_print_int, float, string, bool, char, newline, printf)
- extern "C" for C++ compatibility
- Uses int64_t/double/MetaCString for types
- `block_shm_export()` called automatically every 16 allocs in `block_alloc_aligned()`
- `block_shm_export()` called in `block_reset()` to keep attach mode updated
- 56/56 tests passing

## Pendências
## Pending

- Nenhuma
- None
