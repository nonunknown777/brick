#ifndef _WIN32
#define _POSIX_C_SOURCE 199309L
#endif
#include "hot_reload.h"
#include "block_memory.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdatomic.h>

#if defined(_WIN32)
#  include <windows.h>
#  define HR_DLOPEN(path)        ((void*)LoadLibraryA(path))
#  define HR_DLSYM(handle, name) ((void*)GetProcAddress((HMODULE)(handle), (name)))
#  define HR_DLCLOSE(handle)     (void)(FreeLibrary((HMODULE)(handle)) ? 0 : 1)
#  define HR_DLERROR()           ("GetLastError")
#  define HR_THREAD_T            HANDLE
#  define HR_MUTEX_T             CRITICAL_SECTION
#  define HR_MUTEX_INIT(m)       InitializeCriticalSection((m))
#  define HR_MUTEX_LOCK(m)       EnterCriticalSection((m))
#  define HR_MUTEX_UNLOCK(m)     LeaveCriticalSection((m))
#  define HR_MUTEX_DESTROY(m)    DeleteCriticalSection((m))
#  define HR_THREAD_CREATE(t, f, a) ((*(t) = CreateThread(NULL, 0, (f), (a), 0, NULL)) != NULL)
#  define HR_THREAD_JOIN(t)      WaitForSingleObject((t), INFINITE); CloseHandle(t)
#  define HR_SLEEP(ms)           Sleep(ms)
#  define HR_ATOMIC_STORE(p, v)  InterlockedExchange((long*)(p), (long)(v))
#  define HR_PATH_SEP            '\\'
#  define HR_SO_EXT              ".dll"
#else
#  include <dlfcn.h>
#  include <pthread.h>
#  include <unistd.h>
#  include <sys/inotify.h>
#  define HR_DLOPEN(path)        dlopen(path, RTLD_NOW | RTLD_LOCAL)
#  define HR_DLSYM(handle, name) dlsym(handle, name)
#  define HR_DLCLOSE(handle)     dlclose(handle)
#  define HR_DLERROR()           dlerror()
#  define HR_THREAD_T            pthread_t
#  define HR_MUTEX_T             pthread_mutex_t
#  define HR_MUTEX_INIT(m)       pthread_mutex_init((m), NULL)
#  define HR_MUTEX_LOCK(m)       pthread_mutex_lock((m))
#  define HR_MUTEX_UNLOCK(m)     pthread_mutex_unlock((m))
#  define HR_MUTEX_DESTROY(m)    pthread_mutex_destroy((m))
#  define HR_THREAD_CREATE(t, f, a) (pthread_create((t), NULL, (f), (a)) == 0)
#  define HR_THREAD_JOIN(t)      pthread_join((t), NULL)
#  define HR_SLEEP(ms)           usleep((ms) * 1000)
#  define HR_ATOMIC_STORE(p, v)  __atomic_store_n((p), (v), __ATOMIC_SEQ_CST)
#  define HR_PATH_SEP            '/'
#  define HR_SO_EXT              ".so"
#endif

#define MAX_SYMBOLS 256

typedef struct {
    char  name[128];
    void** func_ptr;
} SymbolEntry;

struct HotReloadEngine {
    char          dll_path[1024];
    char          dll_basename[256];
    void*         handle_current;
    SymbolEntry   symbols[MAX_SYMBOLS];
    int           symbol_count;
    atomic_int    state;
    HR_THREAD_T   watch_thread;
    volatile int  running;
    hr_callback_t callback;
    HR_MUTEX_T    mutex;
    char          watch_dir[1024];
#if defined(_WIN32)
    HANDLE        watch_handle;
    HANDLE        watch_event;
#else
    int           inotify_fd;
    int           watch_fd;
#endif
};

static void error(const char* msg) {
    fprintf(stderr, "Brick hot reload error: %s\n", msg);
    exit(1);
}

HotReloadEngine* hr_create(const char* dll_path) {
    HotReloadEngine* hr = (HotReloadEngine*)calloc(1, sizeof(HotReloadEngine));
    if (!hr) error("out of memory");

    strncpy(hr->dll_path, dll_path, sizeof(hr->dll_path) - 1);

    // Extract basename
    const char* sep = strrchr(dll_path, '/');
#if defined(_WIN32)
    {
        const char* sep2 = strrchr(dll_path, '\\');
        if (sep2 && (!sep || sep2 > sep)) sep = sep2;
    }
#endif
    strncpy(hr->dll_basename, sep ? sep + 1 : dll_path, sizeof(hr->dll_basename) - 1);

    // Extract watch directory
    if (sep) {
        size_t dirlen = (size_t)(sep - dll_path);
        memcpy(hr->watch_dir, dll_path, dirlen);
        hr->watch_dir[dirlen] = '\0';
    } else {
#if defined(_WIN32)
        GetCurrentDirectoryA(sizeof(hr->watch_dir), hr->watch_dir);
#else
        strcpy(hr->watch_dir, ".");
#endif
    }

    hr->state = HR_WAITING;
    hr->running = 0;
    hr->callback = NULL;
#if defined(_WIN32)
    hr->watch_handle = INVALID_HANDLE_VALUE;
    hr->watch_event = NULL;
#else
    hr->inotify_fd = -1;
    hr->watch_fd = -1;
#endif
    HR_MUTEX_INIT(&hr->mutex);

    return hr;
}

int hr_load_initial(HotReloadEngine* hr) {
    HR_MUTEX_LOCK(&hr->mutex);

    hr->handle_current = HR_DLOPEN(hr->dll_path);
    if (!hr->handle_current) {
        fprintf(stderr, "hr: dlopen/LoadLibrary failed: %s\n", HR_DLERROR());
        hr->state = HR_ERROR;
        HR_MUTEX_UNLOCK(&hr->mutex);
        return -1;
    }

    hr->state = HR_OK;

    for (int i = 0; i < hr->symbol_count; i++) {
        void* sym = HR_DLSYM(hr->handle_current, hr->symbols[i].name);
        if (sym) {
            *(hr->symbols[i].func_ptr) = sym;
        } else {
            fprintf(stderr, "hr: symbol '%s' not found\n", hr->symbols[i].name);
        }
    }

    HR_MUTEX_UNLOCK(&hr->mutex);
    return 0;
}

int hr_register_func(HotReloadEngine* hr, const char* name, void** func_ptr) {
    if (hr->symbol_count >= MAX_SYMBOLS) return -1;

    strncpy(hr->symbols[hr->symbol_count].name, name,
            sizeof(hr->symbols[hr->symbol_count].name) - 1);
    hr->symbols[hr->symbol_count].func_ptr = func_ptr;
    hr->symbol_count++;
    return 0;
}

int hr_reload(HotReloadEngine* hr) {
    HR_MUTEX_LOCK(&hr->mutex);
    hr->state = HR_LOADING;

    block_freeze();

    // Copy the library to a temp path so LoadLibrary/dlopen gets a fresh load
    char tmp_path[1088];
    snprintf(tmp_path, sizeof(tmp_path), "%s.%d", hr->dll_path, (int)time(NULL));

    FILE* src = fopen(hr->dll_path, "rb");
    void* new_handle = NULL;
    if (src) {
        FILE* dst = fopen(tmp_path, "wb");
        if (dst) {
            char buf[65536];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
                fwrite(buf, 1, n, dst);
            fclose(dst);

            new_handle = HR_DLOPEN(tmp_path);
            remove(tmp_path);
        }
        fclose(src);
    }

    if (!new_handle) {
        fprintf(stderr, "hr: reload failed\n");
        hr->state = hr->handle_current ? HR_OK : HR_ERROR;
        block_thaw();
        if (hr->callback) hr->callback(hr->dll_path);
        HR_MUTEX_UNLOCK(&hr->mutex);
        return -1;
    }

    // Swap function pointers atomically
    for (int i = 0; i < hr->symbol_count; i++) {
        void* sym = HR_DLSYM(new_handle, hr->symbols[i].name);
        if (sym) {
            HR_ATOMIC_STORE(hr->symbols[i].func_ptr, sym);
        } else {
            fprintf(stderr, "hr: symbol '%s' not found in new library\n",
                    hr->symbols[i].name);
        }
    }

    // Close old handle
    if (hr->handle_current) {
        HR_DLCLOSE(hr->handle_current);
    }
    hr->handle_current = new_handle;
    hr->state = HR_OK;

    block_thaw();
    if (hr->callback) hr->callback(hr->dll_path);

    HR_MUTEX_UNLOCK(&hr->mutex);
    return 0;
}

#if defined(_WIN32)

static DWORD WINAPI watch_thread_fn_win32(LPVOID arg) {
    HotReloadEngine* hr = (HotReloadEngine*)arg;

    char dir_path[1024];
    strcpy(dir_path, hr->watch_dir);

    HANDLE hDir = CreateFileA(
        dir_path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        return 1;
    }

    hr->watch_handle = hDir;

    char notify_buf[4096];
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));
    hr->watch_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    overlapped.hEvent = hr->watch_event;

    // Start first overlapped read
    DWORD bytes_returned;
    ReadDirectoryChangesW(
        hDir, notify_buf, sizeof(notify_buf), FALSE,
        FILE_NOTIFY_CHANGE_LAST_WRITE,
        &bytes_returned, &overlapped, NULL
    );

    while (hr->running) {
        DWORD wait = WaitForSingleObject(hr->watch_event, 500);
        if (wait == WAIT_OBJECT_0) {
            DWORD bytes;
            if (GetOverlappedResult(hDir, &overlapped, &bytes, FALSE)) {
                FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)notify_buf;
                if (event->Action == FILE_ACTION_MODIFIED) {
                    // Check if this is our DLL
                    int dll_len = (int)strlen(hr->dll_basename);
                    int name_len = (int)(event->FileNameLength / sizeof(wchar_t));
                    if (name_len == dll_len) {
                        // Compare wide char basename with our DLL name
                        // Convert our DLL name to wide for comparison
                        wchar_t wname[256];
                        MultiByteToWideChar(CP_UTF8, 0, hr->dll_basename, -1, wname, 256);
                        if (_wcsnicmp(event->FileName, wname, dll_len) == 0) {
                            HR_SLEEP(50);
                            hr_reload(hr);
                        }
                    }
                }

                // Reset for next read
                memset(&overlapped, 0, sizeof(overlapped));
                overlapped.hEvent = hr->watch_event;
                ReadDirectoryChangesW(
                    hDir, notify_buf, sizeof(notify_buf), FALSE,
                    FILE_NOTIFY_CHANGE_LAST_WRITE,
                    &bytes_returned, &overlapped, NULL
                );
            }
            ResetEvent(hr->watch_event);
        }
    }

    CancelIo(hDir);
    CloseHandle(hDir);
    hr->watch_handle = INVALID_HANDLE_VALUE;
    return 0;
}

#else // Linux

static void* watch_thread_fn(void* arg) {
    HotReloadEngine* hr = (HotReloadEngine*)arg;

    char buf[4096] BRICK_ALIGNED(8);

    while (hr->running) {
        ssize_t len = read(hr->inotify_fd, buf, sizeof(buf));
        if (len <= 0) continue;

        size_t i = 0;
        while (i < (size_t)len) {
            struct inotify_event* event = (struct inotify_event*)&buf[i];

            if ((event->mask & IN_CLOSE_WRITE) &&
                event->len > 0 &&
                strcmp(event->name, hr->dll_basename) == 0) {
                struct timespec ts = {0, 50000000};
                nanosleep(&ts, NULL);
                hr_reload(hr);
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }

    return NULL;
}
#endif

int hr_start_watching(HotReloadEngine* hr) {
#if defined(_WIN32)
    // Windows: watch thread is created during hr_create
    hr->running = 1;
    if (!HR_THREAD_CREATE(&hr->watch_thread, watch_thread_fn_win32, hr)) {
        hr->running = 0;
        return -1;
    }
    return 0;
#else
    hr->inotify_fd = inotify_init1(IN_NONBLOCK);
    if (hr->inotify_fd < 0) {
        perror("inotify_init");
        return -1;
    }

    hr->watch_fd = inotify_add_watch(hr->inotify_fd, hr->watch_dir, IN_CLOSE_WRITE);
    if (hr->watch_fd < 0) {
        perror("inotify_add_watch");
        return -1;
    }

    hr->running = 1;
    if (!HR_THREAD_CREATE(&hr->watch_thread, watch_thread_fn, hr)) {
        perror("pthread_create");
        hr->running = 0;
        return -1;
    }

    return 0;
#endif
}

HotReloadState hr_state(HotReloadEngine* hr) {
    return (HotReloadState)atomic_load(&hr->state);
}

void hr_set_callback(HotReloadEngine* hr, hr_callback_t cb) {
    hr->callback = cb;
}

void hr_destroy(HotReloadEngine* hr) {
    if (hr->running) {
        hr->running = 0;
        HR_THREAD_JOIN(hr->watch_thread);
    }

#if defined(_WIN32)
    if (hr->watch_handle != INVALID_HANDLE_VALUE) {
        CancelIo(hr->watch_handle);
        CloseHandle(hr->watch_handle);
    }
    if (hr->watch_event) CloseHandle(hr->watch_event);
#else
    if (hr->inotify_fd >= 0) close(hr->inotify_fd);
#endif
    if (hr->handle_current) HR_DLCLOSE(hr->handle_current);

    HR_MUTEX_DESTROY(&hr->mutex);
    free(hr);
}
