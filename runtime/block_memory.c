#define _GNU_SOURCE
#include "block_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>

#define DEFAULT_ALIGNMENT 8

static atomic_int block_frozen_flag = 0;

// ─── Global Block Registry ───────────────────────────────────
// ─── Registro Global de Blocos ────────────────────────────────
#ifdef META_C_TRACK_BLOCKS

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#define REGISTRY_MAX META_C_MAX_BLOCKS

typedef struct {
    char      name[META_C_BLOCK_NAME_MAX];
    BlockCtx* ctx;
    int       active;
} RegistryEntry;

static RegistryEntry     registry[REGISTRY_MAX];
static int               registry_initialized = 0;
static pthread_mutex_t   registry_mutex = PTHREAD_MUTEX_INITIALIZER;

static void registry_init(void) {
    if (!registry_initialized) {
        memset(registry, 0, sizeof(registry));
        registry_initialized = 1;
    }
}

void block_register(BlockCtx* ctx, const char* name) {
    registry_init();
    pthread_mutex_lock(&registry_mutex);

    int slot = -1;
    for (int i = 0; i < REGISTRY_MAX; i++) {
        if (!registry[i].active) { slot = i; break; }
    }

    if (slot >= 0) {
        strncpy(registry[slot].name, name, META_C_BLOCK_NAME_MAX - 1);
        registry[slot].name[META_C_BLOCK_NAME_MAX - 1] = '\0';
        registry[slot].ctx = ctx;
        registry[slot].active = 1;
    }

    pthread_mutex_unlock(&registry_mutex);
}

void block_unregister(BlockCtx* ctx) {
    registry_init();
    pthread_mutex_lock(&registry_mutex);

    for (int i = 0; i < REGISTRY_MAX; i++) {
        if (registry[i].active && registry[i].ctx == ctx) {
            registry[i].active = 0;
            break;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
}

BlockCtx* block_find(const char* name) {
    registry_init();
    pthread_mutex_lock(&registry_mutex);

    BlockCtx* found = NULL;
    for (int i = 0; i < REGISTRY_MAX; i++) {
        if (registry[i].active && strcmp(registry[i].name, name) == 0) {
            found = registry[i].ctx;
            break;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
    return found;
}

size_t block_snapshot(BlockInfo* out, size_t max_count) {
    registry_init();
    pthread_mutex_lock(&registry_mutex);

    size_t written = 0;
    for (int i = 0; i < REGISTRY_MAX && written < max_count; i++) {
        if (registry[i].active) {
            strncpy(out[written].name, registry[i].name, META_C_BLOCK_NAME_MAX - 1);
            out[written].name[META_C_BLOCK_NAME_MAX - 1] = '\0';
            out[written].capacity        = registry[i].ctx->capacity;
            out[written].used            = registry[i].ctx->used;
            out[written].peak_used       = registry[i].ctx->peak_used;
            out[written].allocation_count = registry[i].ctx->allocation_count;
            written++;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
    return written;
}

// ─── Shared Memory Export ────────────────────────────────────
// ─── Exportacao de Memoria Compartilhada ──────────────────────

static void shm_path(char* buf, size_t bufsize) {
    snprintf(buf, bufsize, "/tmp/meta-c-mem-%d.bin", (int)getpid());
}

int block_shm_export(void) {
    registry_init();

    char path[256];
    shm_path(path, sizeof(path));

    pthread_mutex_lock(&registry_mutex);

    int count = 0;
    for (int i = 0; i < REGISTRY_MAX; i++)
        if (registry[i].active) count++;

    size_t file_size = sizeof(MetaCShmHeader) + (size_t)count * sizeof(BlockInfo);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { pthread_mutex_unlock(&registry_mutex); return -1; }

    ftruncate(fd, (off_t)file_size);

    MetaCShmHeader header;
    header.magic   = META_C_SHM_MAGIC;
    header.version = 1;
    header.pid     = (int32_t)getpid();
    header.block_count = (uint32_t)count;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    header.timestamp_us = (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;

    write(fd, &header, sizeof(header));

    int written = 0;
    for (int i = 0; i < REGISTRY_MAX && written < count; i++) {
        if (registry[i].active) {
            BlockInfo info;
            strncpy(info.name, registry[i].name, META_C_BLOCK_NAME_MAX - 1);
            info.name[META_C_BLOCK_NAME_MAX - 1] = '\0';
            info.capacity        = registry[i].ctx->capacity;
            info.used            = registry[i].ctx->used;
            info.peak_used       = registry[i].ctx->peak_used;
            info.allocation_count = registry[i].ctx->allocation_count;
            write(fd, &info, sizeof(info));
            written++;
        }
    }

    close(fd);
    pthread_mutex_unlock(&registry_mutex);
    return 0;
}

#endif // META_C_TRACK_BLOCKS
     // META_C_TRACK_BLOCKS

static void error(const char* msg) {
    fprintf(stderr, "Meta-C runtime error: %s\n", msg);
    exit(1);
}

BlockCtx* block_create(size_t megabytes) {
    return block_create_bytes(megabytes * 1024 * 1024);
}

BlockCtx* block_create_bytes(size_t bytes) {
    BlockCtx* ctx = (BlockCtx*)malloc(sizeof(BlockCtx));
    if (!ctx) error("out of memory");

    ctx->data = (uint8_t*)malloc(bytes);
    if (!ctx->data) {
        free(ctx);
        error("out of memory");
    }

    ctx->capacity = bytes;
    ctx->used = 0;
    ctx->peak_used = 0;
    ctx->allocation_count = 0;

    return ctx;
}

void* block_alloc(BlockCtx* ctx, size_t size) {
    return block_alloc_aligned(ctx, size, DEFAULT_ALIGNMENT);
}

void* block_alloc_aligned(BlockCtx* ctx, size_t size, size_t alignment) {
    // Spin-wait if frozen (hot reload in progress)
    // Espera ocupada se congelado (hot reload em andamento)
    while (atomic_load_explicit(&block_frozen_flag, memory_order_acquire)) {
        // spin — will be very brief (nanoseconds)
        // giro — sera muito breve (nanossegundos)
        __asm__ volatile("pause");
    }

    // Align current position
    // Alinha posicao atual
    size_t current = ctx->used;
    size_t aligned = (current + alignment - 1) & ~(alignment - 1);

    // Check overflow
    // Verifica estouro
    if (aligned + size > ctx->capacity) {
        error("block overflow: out of memory in block");
    }

    ctx->used = aligned + size;
    ctx->allocation_count++;

    if (ctx->used > ctx->peak_used) {
        ctx->peak_used = ctx->used;
    }

    // Auto-export shm every 16 allocations (for visualizer attach mode)
    // Auto-exporta shm a cada 16 alocacoes (para modo attach do visualizer)
    static unsigned int _alloc_counter = 0;
    if ((++_alloc_counter & 0xF) == 0) {
        block_shm_export();
    }

    return ctx->data + aligned;
}

void block_reset(BlockCtx* ctx) {
    ctx->used = 0;
    // Note: allocation_count is NOT reset here,
    // Nota: allocation_count NAO e resetado aqui,
    // so we can track total allocations across resets
    // para que possamos rastrear alocacoes totais entre resets
    block_shm_export();
}

void block_destroy(BlockCtx* ctx) {
    if (ctx) {
        free(ctx->data);
        free(ctx);
    }
}

BlockStats block_stats(BlockCtx* ctx) {
    BlockStats stats;
    stats.total_size = ctx->capacity;
    stats.used_size = ctx->used;
    stats.free_size = ctx->capacity - ctx->used;
    stats.peak_used = ctx->peak_used;
    stats.allocation_count = ctx->allocation_count;

    // Fragmentation: 0% for bump allocator (no holes)
    // Fragmentacao: 0% para bump allocator (sem buracos)
    stats.fragmentation_percent = 0.0f;

    return stats;
}

size_t block_alignment(void) {
    return DEFAULT_ALIGNMENT;
}

void block_freeze(void) {
    atomic_store_explicit(&block_frozen_flag, 1, memory_order_release);
}

void block_thaw(void) {
    atomic_store_explicit(&block_frozen_flag, 0, memory_order_release);
}
