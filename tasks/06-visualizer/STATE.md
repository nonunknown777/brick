# Task 06 - Visualizer - STATE

## Status: ✅ COMPLETO

Visualizador TUI ncurses implementado. Anexa a processo via shared memory.

## Implementado
- attach a PID via shared memory (/tmp/brick-mem-<pid>.bin)
- Exibição de blocos (nome, capacity, used, peak)
- Fork child process para visualização
- --visualize e --attach CLI flags
- MemVisConfig com defaults

## Arquivos
- `visualizer/` - ncurses TUI
- Integração via `main.cpp` (BRICK_TRACK_BLOCKS ifdef)
- Shared memory protocol: BrickShmHeader + BlockInfo array

## Observações
- Requer -DBRICK_TRACK_BLOCKS (debug mode)
- Requer ncurses (pkg-config)
- Auto-export shm a cada 16 alocações no runtime
