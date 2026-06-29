# Brick Hot Reload

> Swap code without stopping your program.

## Overview

Hot reload lets you edit your `.brc` code, save, and see the result **without restarting** the program. Useful for game jams, live editors, and any situation where downtime is unacceptable.

```
Edit .brc   →  save   →  compile to .so/.dll   →  load   →  function swapped atomically
```

## How It Works

### Architecture

Hot reload is cross-platform, using OS-specific primitives:

| Platform | Dynamic Loading | File Watching | Atomic Swap |
|---|---|---|---|
| **Linux** | `dlopen` + `dlsym` | `inotify` | `__atomic_store_n` |
| **Windows** | `LoadLibraryA` + `GetProcAddress` | `ReadDirectoryChangesW` (overlapped I/O) | `InterlockedExchange` |

The hot reload engine runs in a **separate thread** (`pthread` on Linux, `CreateThread` on Windows), watching the `.so`/`.dll` directory. When it detects a change, it:

1. **Copies** the shared library to a temp file (dlopen caches by path; a copy bypasses the cache)
2. **Loads** the new library via `dlopen` (Linux) or `LoadLibraryA` (Windows)
3. **Atomic swap**: updates all registered function pointers atomically
4. **Closes** the old library with `dlclose` (Linux) or `FreeLibrary` (Windows)
5. Fires the **callback** notification

### States

| State | Meaning |
|---|---|
| `HR_WAITING` | Waiting for initial load |
| `HR_LOADING` | Loading new .so |
| `HR_OK` | Ready |
| `HR_ERROR` | dlopen failure |

## How to Use

### Compile with hot reload support

```bash
brick build game.brc --release -o game
```

The `--release` flag compiles each package as a separate .so.

### Run

```bash
./game
```

The hot reload engine watches the .so files in the background.

### Edit and see results

Edit your `.brc`, save, and the binary reloads automatically. The function swap is **atomic** — there's no moment where the pointer points nowhere.

### Typical development cycle

```bash
# Terminal 1: compile and run
brick build mygame.brc --release -o mygame
./mygame

# Terminal 2: edit code
vim mygame.brc    # make changes
# compile new version
brick build mygame.brc --release -o mygame
# → hot reload detects the new .so and swaps functions automatically
```

## Hot Reload API

### For end users

Just use `brick build --release`. The compiler generates the .so files and the runtime handles the rest.

### For C integration

```c
#include "hot_reload.h"

// 1. Create the engine
HotReloadEngine* hr = hr_create("./build/mylib.so");

// 2. Register functions to be swapped
void (*my_func)(void) = NULL;
hr_register_func(hr, "my_function", (void**)&my_func);

// 3. Load initial version
hr_load_initial(hr);

// 4. Start monitoring (inotify in separate thread)
hr_start_watching(hr);

// 5. (optional) Callback after each reload
hr_set_callback(hr, my_callback);

// ... your program runs, edit the .so, and functions are swapped ...

// 6. Cleanup
hr_destroy(hr);
```

### API Functions

| Function | Description |
|---|---|
| `hr_create(path)` | Creates hot reload engine |
| `hr_register_func(hr, name, ptr)` | Registers function for swap |
| `hr_load_initial(hr)` | Loads initial symbols |
| `hr_start_watching(hr)` | Starts inotify monitoring |
| `hr_reload(hr)` | Forces manual reload |
| `hr_state(hr)` | Returns current state |
| `hr_set_callback(hr, cb)` | Sets post-reload callback |
| `hr_destroy(hr)` | Cleans up resources |

## Implementation Details

### Atomic swap

We use `__atomic_store` with `__ATOMIC_SEQ_CST` to ensure all function pointers are updated atomically. Readers always see a valid pointer.

### Block freeze

During reload, the runtime **freezes all block allocations** (`block_freeze()`). No new allocations happen during the swap. After the swap, blocks are thawed (`block_thaw()`).

### Cache busting

dlopen caches by file path. To ensure a new version is loaded, we copy the .so to a temporary path before calling dlopen.

### Rollback

If dlopen of the new version fails, the system keeps the old handle and the function pointers untouched. The program continues running with the previous version.

### Safety delay

A 50ms `nanosleep` between change detection and reload ensures the .so file write has completed fully.

## Best Practices

- **Stable data structures**: don't change struct layout between versions (ABI must be compatible)
- **Minimal global state**: global variables aren't reloaded — pass state via structs
- **Callbacks**: use `hr_set_callback` to reconfigure state after each reload

## Limitations

| Limitation | Reason | Workaround |
|---|---|---|
| .so/.dll per package | Each package becomes a separate shared library | Organize packages by module |
| Data doesn't reload | Only code (functions) is swapped | Use stable structs |
| ABI must be compatible | Function pointers expect the same signature | Freeze the public interface |

## Comparison

| Brick Hot Reload | Live++ / C++ Modules |
|---|---|
| Atomic function pointer swap | Full TU recompilation |
| No runtime cost when not in use | Instrumentation overhead |
| Pure C, no C++ runtime | Requires C++ runtime |
| Simple and predictable | Complex and state of the art |

## Full Example

See `tests/test_hot_reload.c` and `tests/test_libs_window_hr.c` for complete API usage examples.
