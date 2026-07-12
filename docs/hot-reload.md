# Hot Reload in Brick

Brick supports native hot reload — swap code at runtime without restarting the program. Uses `dlopen`+`inotify` (Linux) or `LoadLibrary`+`ReadDirectoryChangesW` (Windows).

## How It Works

1. **Compile shared library**: `brick build --shared game.brc -o game.so`
2. **Run program**: The hot reload runtime loads `game.so` via `dlopen`
3. **Watch for changes**: `inotify` monitors the `.so` file for modifications
4. **Swap automatically**: When recompiled, the runtime atomically swaps function pointers
5. **Continue execution**: New code runs immediately — no restart needed

## States

| State | Description |
|-------|-------------|
| `HR_WAITING` | Monitoring for file changes |
| `HR_LOADING` | Reloading the shared library |
| `HR_OK` | Code is up to date |
| `HR_ERROR` | Reload failed (e.g., compile error) |

## Setup

### 1. Create Game Code

`game.brc`:

```brick
package GAME
using IO

block game = 64MB

export fn update(f32 dt) {
    print("updating with dt={0}", dt)
}
```

### 2. C Host Program

```c
// main.c
#include "runtime/hot_reload.h"
#include "runtime/block_memory.h"

int main() {
    BlockCtx* game_mem = block_create(64 * 1024 * 1024);
    HotReloadCtx* hr = hr_create("./game.so", game_mem);

    while (running) {
        hr_check(hr);  // reload if changed

        void (*update)(float) = hr_sym(hr, "update");
        if (update) update(0.016f);

        // optional: wait for VSync
    }

    hr_destroy(hr);
    block_destroy(game_mem);
    return 0;
}
```

### 3. Build and Run

```bash
# First build game as shared library
brick build --shared game.brc -o game.so
gcc -O3 main.c runtime/hot_reload.c runtime/block_memory.c runtime/io.c \
    -ldl -o game

# Run — it starts with the initial game.so
./game

# In another terminal, edit game.brc and recompile:
brick build --shared game.brc -o game.so

# The running program automatically picks up the change!
```

## Hot Reload with the Brick CLI

```bash
# Build with hot reload support
brick build game.brc -o game --hot-reload

# Run (hot reload enabled by default)
./game

# Edit game.brc in another terminal
brick build game.brc -o game --hot-reload

# Running ./game automatically picks up changes!
```

## Memory + Hot Reload

Double-buffer blocks enable zero-pause hot reload:

```c
// In C host
block_enable_double_buffer(game_mem);

// When hot reload occurs:
block_swap_buffers(game_mem);  // atomic, ~1 cycle

// Old allocations are still valid until next swap
// New allocations use the fresh buffer
```

## Atomic Function Pointer Swap

The hot reload system ensures:

- **No torn reads**: function pointer swap is atomic (word-sized)
- **No locking**: readers always see a valid pointer
- **No deadlocks**: swap happens between frames, not during execution

```c
// Pseudo-code of the atomic swap:
void hr_swap(HotReloadCtx* hr) {
    void* old_lib = hr->lib;
    hr->lib = dlopen(new_path, RTLD_NOW | RTLD_LOCAL);
    // ... resolve symbols, update table ...
    dlclose(old_lib);
}
```

## Limitations

- Functions must be `export fn` to be visible to the host
- Struct layout must remain compatible across reloads
- Global state in the host C code persists across reloads
- Brick source changes that affect struct layout = restart required
- Only function logic can change safely

## Cross-Platform

| Feature | Linux | Windows |
|---------|-------|---------|
| Dynamic library | `.so` | `.dll` |
| File watching | `inotify` | `ReadDirectoryChangesW` |
| Library loading | `dlopen`/`dlsym` | `LoadLibrary`/`GetProcAddress` |
| Threading | `pthreads` | `CreateThread` |
| Atomic swap | `__sync_bool_compare_and_swap` | `InterlockedExchange` |

## See Also

- [Getting Started](GETTING_STARTED.md) — Setting up your first project
- [Architecture](ARCHITECTURE.md) — How the runtime fits together
