// Windows-specific tests for Brick runtime
// Tests: VirtualAlloc block_memory, BRICK_TLS, pool allocator, hot_reload lifecycle
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../runtime/block_memory.h"
#include "../runtime/pool_allocator.h"
#include "../runtime/hot_reload.h"

static int win_passed = 0;
static int win_failed = 0;

static int test_fail_count = 0;

#define TEST(name) do { \
    test_fail_count = 0; \
    printf("  [TEST] %s ... ", #name); \
    fflush(stdout); \
    test_##name(); \
    if (test_fail_count == 0) { \
        printf("PASS\n"); \
        win_passed++; \
    } \
} while(0)

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        test_fail_count++; \
        win_failed++; \
        return; \
    } \
} while(0)

// ─── Block Memory Tests (VirtualAlloc backend) ───────────

void test_create_block() {
    BlockCtx* block = block_create(1);
    CHECK(block != NULL, "block_create returned NULL");
    CHECK(block->capacity == 1024 * 1024, "1MB capacity");
    CHECK(block->used == 0, "used == 0");
    block_destroy(block);
}

void test_alloc() {
    BlockCtx* block = block_create(1);
    int* a = (int*)block_alloc(block, sizeof(int));
    *a = 42;
    CHECK(*a == 42, "int alloc/write/read");

    double* b = (double*)block_alloc(block, sizeof(double));
    *b = 3.14;
    CHECK(*b > 3.0, "double alloc/write/read");

    CHECK(block->used >= sizeof(int) + sizeof(double), "used bytes correct");
    block_destroy(block);
}

void test_reset() {
    BlockCtx* block = block_create(1);
    block_alloc(block, 100);
    CHECK(block->used >= 100, "used >= 100");
    block_reset(block);
    CHECK(block->used == 0, "used == 0 after reset");
    block_destroy(block);
}

void test_stats() {
    BlockCtx* block = block_create(2);
    BlockStats stats = block_stats(block);
    CHECK(stats.total_size == 2 * 1024 * 1024, "2MB total");
    CHECK(stats.allocation_count == 0, "0 allocs");

    block_alloc(block, 512);
    stats = block_stats(block);
    CHECK(stats.used_size >= 512, "used >= 512");
    CHECK(stats.allocation_count == 1, "1 alloc");

    block_destroy(block);
}

void test_alignment() {
    size_t max_align = block_alignment();
    CHECK(max_align == 8 || max_align == 16, "align 8 or 16");

    BlockCtx* block = block_create_bytes(256);
    void* p1 = block_alloc(block, 1);
    CHECK(p1 != NULL, "alloc 1 byte");

    void* p4 = block_alloc_aligned(block, 1, 8);
    CHECK(((uintptr_t)p4 & 0x7) == 0, "8-byte aligned");

    block_destroy(block);
}

void test_create_bytes() {
    BlockCtx* block = block_create_bytes(4096);
    CHECK(block != NULL, "create_bytes returns non-NULL");
    CHECK(block->capacity == 4096, "capacity == 4096");
    block_destroy(block);
}

void test_multiple_blocks() {
    BlockCtx* b1 = block_create(1);
    BlockCtx* b2 = block_create(1);
    void* p1 = block_alloc(b1, 512);
    void* p2 = block_alloc(b2, 256);
    CHECK(p1 != p2, "independent blocks");
    block_reset(b1);
    CHECK(b1->used == 0, "b1 reset");
    CHECK(b2->used >= 256, "b2 unaffected");
    block_destroy(b1);
    block_destroy(b2);
}

void test_freeze_thaw() {
    BlockCtx* block = block_create_bytes(1024);
    block_freeze();
    block_thaw();
    void* p = block_alloc(block, 64);
    CHECK(p != NULL, "alloc after thaw");
    block_destroy(block);
}

void test_stress() {
    BlockCtx* block = block_create(1);
    for (int i = 0; i < 10000; i++) {
        int* p = (int*)block_alloc(block, sizeof(int));
        *p = i;
    }
    BlockStats stats = block_stats(block);
    CHECK(stats.allocation_count == 10000, "10000 allocs");
    CHECK(stats.used_size <= stats.total_size, "used <= total");
    block_destroy(block);
}

void test_peak_used() {
    BlockCtx* block = block_create_bytes(4096);
    block_alloc(block, 100);
    size_t after = block->used;
    block_reset(block);
    block_alloc(block, 50);
    CHECK(block->peak_used >= after, "peak preserved after reset");
    block_destroy(block);
}

void test_zero_size_alloc() {
    BlockCtx* block = block_create_bytes(256);
    void* p1 = block_alloc(block, 0);
    CHECK(p1 != NULL, "zero-size alloc returns non-NULL");
    void* p2 = block_alloc(block, 0);
    CHECK(p2 != NULL, "second zero-size alloc returns non-NULL");
    block_destroy(block);
}

void test_allocation_count() {
    BlockCtx* block = block_create_bytes(1024);
    CHECK(block->allocation_count == 0, "initial count 0");
    block_alloc(block, 16);
    block_alloc(block, 32);
    CHECK(block->allocation_count == 2, "count == 2 after 2 allocs");
    block_reset(block);
    CHECK(block->allocation_count == 2, "count preserved after reset");
    block_destroy(block);
}

// ─── Pool Allocator Tests ────────────────────────────────

void test_pool_create() {
    PoolAllocator* pool = pool_create();
    CHECK(pool != NULL, "pool_create returns non-NULL");
    pool_destroy(pool);
}

void test_pool_add_slot() {
    PoolAllocator* pool = pool_create();
    int r = pool_add_slot(pool, 64, 10);
    CHECK(r == 0, "add slot returns 0");
    pool_destroy(pool);
}

void test_pool_alloc_free() {
    PoolAllocator* pool = pool_create();
    // block_size must accommodate size + PoolBlockHeader overhead
    pool_add_slot(pool, 128, 10);
    void* p = pool_alloc(pool, 64);
    CHECK(p != NULL, "pool_alloc returns non-NULL");
    pool_free(pool, p);
    pool_destroy(pool);
}

void test_pool_stats() {
    PoolAllocator* pool = pool_create();
    pool_add_slot(pool, 128, 10);
    void* p = pool_alloc(pool, 64);
    PoolStats stats = pool_slot_stats(pool, 0);
    CHECK(stats.block_size == 128, "slot block size 128");
    CHECK(stats.used == 1, "1 used after alloc");
    pool_free(pool, p);
    stats = pool_slot_stats(pool, 0);
    CHECK(stats.free == 10, "all free after free");
    pool_destroy(pool);
}

// ─── TLS Tests ───────────────────────────────────────────

BRICK_TLS int win_tls_val = 0;

void test_tls() {
    win_tls_val = 0;
    CHECK(win_tls_val == 0, "TLS initial 0");
    win_tls_val = 42;
    CHECK(win_tls_val == 42, "TLS set to 42");
}

// ─── Hot Reload Engine Lifecycle ─────────────────────────

void test_hr_create_destroy() {
    // Create engine with NULL path (should fail gracefully)
    HotReloadEngine* hr = hr_create("nonexistent.dll");
    // Engine may still be created; check it's non-NULL or NULL as expected
    // We just verify the API doesn't crash
    if (hr) hr_destroy(hr);
    CHECK(1, "hr_create/destroy no crash");
}

int main() {
    printf("=== Windows-Specific Tests ===\n\n");

    printf("--- Block Memory (VirtualAlloc) ---\n");
    TEST(create_block);
    TEST(create_bytes);
    TEST(alloc);
    TEST(reset);
    TEST(stats);
    TEST(alignment);
    TEST(multiple_blocks);
    TEST(freeze_thaw);
    TEST(stress);
    TEST(peak_used);
    TEST(zero_size_alloc);
    TEST(allocation_count);

    printf("\n--- Pool Allocator ---\n");
    TEST(pool_create);
    TEST(pool_add_slot);
    TEST(pool_alloc_free);
    TEST(pool_stats);

    printf("\n--- TLS ---\n");
    TEST(tls);

    printf("\n--- Hot Reload ---\n");
    TEST(hr_create_destroy);

    printf("\n=== Results: %d passed, %d failed ===\n", win_passed, win_failed);
    return win_failed > 0 ? 1 : 0;
}

#else
int main() { return 0; }
#endif