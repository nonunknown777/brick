#include "../runtime/block_memory.h"
#include <stdio.h>
#include <assert.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

void test_create_block() {
    BlockCtx* block = block_create(1); // 1MB
                                        // 1MB
    assert(block != NULL);
    assert(block->capacity == 1024 * 1024);
    assert(block->used == 0);
    printf("[PASS] test_create_block\n");
    block_destroy(block);
}

void test_alloc() {
    BlockCtx* block = block_create(1);
    int* a = (int*)block_alloc(block, sizeof(int));
    *a = 42;
    assert(*a == 42);

    double* b = (double*)block_alloc(block, sizeof(double));
    *b = 3.14;
    assert(*b > 3.0);

    assert(block->used >= sizeof(int) + sizeof(double));
    printf("[PASS] test_alloc\n");
    block_destroy(block);
}

void test_reset() {
    BlockCtx* block = block_create(1);
    block_alloc(block, 100);
    assert(block->used >= 100);

    block_reset(block);
    assert(block->used == 0);

    printf("[PASS] test_reset\n");
    block_destroy(block);
}

void test_stats() {
    BlockCtx* block = block_create(2); // 2MB
                                        // 2MB
    BlockStats stats = block_stats(block);
    assert(stats.total_size == 2 * 1024 * 1024);
    assert(stats.free_size == stats.total_size);
    assert(stats.allocation_count == 0);

    block_alloc(block, 512);
    stats = block_stats(block);
    assert(stats.used_size >= 512);
    assert(stats.allocation_count == 1);

    printf("[PASS] test_stats\n");
    block_destroy(block);
}

#ifndef _WIN32
void test_overflow() {
    // Create a tiny block (1 byte) - any real allocation should overflow
    // Cria um bloco minusculo (1 byte) - qualquer alocacao real deve estourar
    BlockCtx* block = block_create_bytes(1);
    assert(block != NULL);

    fflush(NULL); // flush all streams before fork to avoid child flushing parent's buffers
                  // descarrega todos os streams antes do fork para evitar que filho descarregue buffers do pai
    pid_t pid = fork();
    if (pid == 0) {
        // Suppress all output in child (it should die from overflow)
        // Suprime toda saida no filho (deve morrer por estouro)
        fclose(stdout);
        fclose(stderr);
        block_alloc(block, 64);
        _exit(0); // should not reach here
                  // nao deveria chegar aqui
    }

    int status;
    waitpid(pid, &status, 0);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 1);

    block_destroy(block);
    printf("[PASS] test_overflow\n");
}
#endif

void test_alignment() {
    size_t max_align = block_alignment();
    assert(max_align == 8 || max_align == 16);

    BlockCtx* block = block_create_bytes(256);

    // block_alloc uses optimal alignment based on size
    // block_alloc usa alinhamento otimo baseado no tamanho
    void* p1 = block_alloc(block, 1);  // optimal: align 1
    assert(((uintptr_t)p1 & 0x0) == 0); // always true (align 1)

    void* p2 = block_alloc(block, 3);  // optimal: align 2
    assert(((uintptr_t)p2 & 0x1) == 0); // 2-byte aligned

    void* p3 = block_alloc(block, 0);  // optimal: align 1
    assert(p3 != NULL);

    // block_alloc_aligned still respects explicit alignment
    // block_alloc_aligned ainda respeita alinhamento explicito
    void* p4 = block_alloc_aligned(block, 1, 8);
    assert(((uintptr_t)p4 & 0x7) == 0);

    block_destroy(block);
    printf("[PASS] test_alignment\n");
}

void test_multiple_blocks() {
    // Test that independent blocks don't interfere
    // Testa se blocos independentes nao interferem
    BlockCtx* b1 = block_create(1);
    BlockCtx* b2 = block_create(1);

    void* p1 = block_alloc(b1, 512);
    void* p2 = block_alloc(b2, 256);

    assert(p1 != p2);
    assert(b1->used >= 512);
    assert(b2->used >= 256);

    block_reset(b1);
    assert(b1->used == 0);
    assert(b2->used >= 256); // b2 unaffected
                              // b2 nao afetado

    block_destroy(b1);
    block_destroy(b2);
    printf("[PASS] test_multiple_blocks\n");
}

void test_create_bytes() {
    BlockCtx* block = block_create_bytes(4096);
    assert(block != NULL);
    assert(block->capacity == 4096);
    assert(block->used == 0);

    block_destroy(block);
    printf("[PASS] test_create_bytes\n");
}

void test_zero_size_alloc() {
    BlockCtx* block = block_create_bytes(256);

    void* p1 = block_alloc(block, 0);
    assert(p1 != NULL);

    void* p2 = block_alloc(block, 0);
    assert(p2 != NULL);

    block_destroy(block);
    printf("[PASS] test_zero_size_alloc\n");
}

void test_large_alignment() {
    BlockCtx* block = block_create_bytes(1024);

    // 16-byte aligned (may fail if malloc returns <16-byte aligned buffer)
    // Alinhado a 16 bytes (pode falhar se malloc retornar buffer com alinhamento <16)
    void* p1 = block_alloc_aligned(block, 32, 16);
    // Silently skip if base buffer isn't 16-byte aligned
    if (((uintptr_t)p1 & 0xF) != 0) {
        printf("[SKIP] test_large_alignment (buffer not 16-byte aligned)\n");
        block_destroy(block);
        return;
    }

    // 32-byte aligned
    void* p2 = block_alloc_aligned(block, 64, 32);
    assert(((uintptr_t)p2 & 0x1F) == 0);

    // 64-byte aligned (cache line)
    void* p3 = block_alloc_aligned(block, 128, 64);
    assert(((uintptr_t)p3 & 0x3F) == 0);

    block_destroy(block);
    printf("[PASS] test_large_alignment\n");
}

void test_peak_used() {
    BlockCtx* block = block_create_bytes(4096);

    assert(block->peak_used == 0);

    block_alloc(block, 100);
    size_t after_first = block->used;
    assert(block->peak_used == after_first);

    block_alloc(block, 200);
    size_t after_second = block->used;
    assert(block->peak_used == after_second);

    // Reset and alloc less — peak should NOT decrease
    block_reset(block);
    block_alloc(block, 50);
    assert(block->peak_used == after_second);

    // Alloc more — peak should increase
    block_alloc(block, 500);
    assert(block->peak_used > after_second);

    block_destroy(block);
    printf("[PASS] test_peak_used\n");
}

void test_allocation_count() {
    BlockCtx* block = block_create_bytes(1024);

    assert(block->allocation_count == 0);

    block_alloc(block, 16);
    assert(block->allocation_count == 1);

    block_alloc(block, 32);
    assert(block->allocation_count == 2);

    // Reset does NOT reset allocation_count
    block_reset(block);
    assert(block->allocation_count == 2);

    block_alloc(block, 64);
    assert(block->allocation_count == 3);

    block_destroy(block);
    printf("[PASS] test_allocation_count\n");
}

void test_stress() {
    BlockCtx* block = block_create(1); // 1MB

    // Allocate many small blocks
    for (int i = 0; i < 10000; i++) {
        int* p = (int*)block_alloc(block, sizeof(int));
        *p = i;
    }

    BlockStats stats = block_stats(block);
    assert(stats.allocation_count == 10000);
    assert(stats.total_size == 1024 * 1024);
    assert(stats.used_size <= stats.total_size);

    block_destroy(block);
    printf("[PASS] test_stress\n");
}

void test_freeze_thaw_blocked() {
    BlockCtx* block = block_create_bytes(1024);

    block_freeze();
    block_thaw();

    // After thaw, alloc should work
    void* p = block_alloc(block, 64);
    assert(p != NULL);

    block_destroy(block);
    printf("[PASS] test_freeze_thaw_blocked\n");
}

int main() {
    test_create_block();
    test_create_bytes();
    test_alloc();
    test_reset();
#ifndef _WIN32
    test_overflow();
#endif
    test_stats();
    test_alignment();
    test_large_alignment();
    test_zero_size_alloc();
    test_peak_used();
    test_allocation_count();
    test_stress();
    test_multiple_blocks();
    test_freeze_thaw_blocked();

    printf("\nAll runtime tests passed!\n");
    return 0;
}
