# Task: Visualizer TUI (Meta-C)

## Função
## Role

Você é o especialista em VISUALIZADOR do Meta-C.
Responsabilidade: TUI ncurses mostrando blocos de memória em tempo real.
You are the VISUALIZER specialist for Meta-C.
Responsibility: ncurses TUI showing memory blocks in real time.

## Regras de Ouro
## Golden Rules

1. AO INICIAR: leia STATE.md, NEXT.md e shared-context.md
2. ANTES DE SAIR: atualize estado
3. Código em: /mnt/Novo_volume/meta-c/visualizer/
4. Dependência: ncurses (libncurses-dev no Arch)

1. ON START: read STATE.md, NEXT.md and shared-context.md
2. BEFORE LEAVING: update state
3. Code in: /mnt/Novo_volume/meta-c/visualizer/
4. Dependency: ncurses (libncurses-dev on Arch)

## Interface

```cpp
#ifndef META_C_MEMVIS_H
#define META_C_MEMVIS_H

#include "../runtime/block_memory.h"

namespace meta_c {

struct MemVisConfig {
    int refresh_ms = 500;
    bool follow_fork = false;
};

// Inicia a TUI (bloqueante)
void run_visualizer(MemVisConfig config);

// Modo servidor: anexa a um processo por socket/pipe
void run_visualizer_attach(pid_t target_pid, MemVisConfig config);

}

#endif
```

## Funcionalidades
## Features

- Lista de blocos ativos com nome e tamanho
- Barra de uso visual: ████████░░░░ 67%
- Métricas: total, usado, livre, pico
- Contagem de alocações
- Modo real-time (refresh a cada 500ms)
- Destaque de blocos perto do limite (>80%)
- Navegação: setas pra detalhar um bloco
- Sair: 'q'

- Active block list with name and size
- Visual usage bar: ████████░░░░ 67%
- Metrics: total, used, free, peak
- Allocation count
- Real-time mode (refresh every 500ms)
- Highlight blocks near limit (>80%)
- Navigation: arrow keys to detail a block
- Quit: 'q'

## Modos de Operação
## Operation Modes

1. **Standalone**: visualizador independente que lê /tmp/meta-c-mem-*.bin
2. **Attach**: anexa a PID via shared memory ou pipe nomeado
3. **Embedded**: runtime chama callback e visualizador roda em thread separada

1. **Standalone**: independent visualizer that reads /tmp/meta-c-mem-*.bin
2. **Attach**: attaches to PID via shared memory or named pipe
3. **Embedded**: runtime calls callback and visualizer runs in separate thread

## Layout da TUI
## TUI Layout

```
┌─────────────────────────────────────────────┐
│  META-C Memory Visualizer  █ refresh: 500ms │
├─────────────────────────────────────────────┤
│  global  256MB  ████████████░░░  67%  172MB │
│  game     64MB  ██████░░░░░░░░  38%   24MB │
│  temp     32MB  ██░░░░░░░░░░░░  12%    4MB │
│  assets  128MB  ██████████████ 100%  128MB  │ ⚠
├─────────────────────────────────────────────┤
│  [global]                                     │
│  allocations: 1,423                           │
│  peak: 180MB                                  │
│  avg size: 126KB                              │
│  last alloc: Player (64B)                     │
├─────────────────────────────────────────────┤
│  '↑↓' select  '→' detail  'r' reset  'q' quit│
└─────────────────────────────────────────────┘
```
