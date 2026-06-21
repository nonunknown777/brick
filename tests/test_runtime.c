#include "../runtime/block_memory.h"
#include <stdio.h>
#include <assert.h>
#include <sys/wait.h>
#include <unistd.h>

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

void test_alignment() {
    size_t align = block_alignment();
    assert(align == 8 || align == 16);

    BlockCtx* block = block_create_bytes(256);

    // Allocate 1 byte, check the returned address is aligned
    // Aloca 1 byte, verifica se o endereco retornado esta alinhado
    void* p1 = block_alloc(block, 1);
    assert(((uintptr_t)p1 & (align - 1)) == 0);

    // Allocate 3 more bytes, address should still be aligned
    // Aloca mais 3 bytes, endereco deve continuar alinhado
    void* p2 = block_alloc(block, 3);
    assert(((uintptr_t)p2 & (align - 1)) == 0);

    // Allocate 0 bytes should return valid aligned pointer
    // Alocar 0 bytes deve retornar ponteiro alinhado valido
    void* p3 = block_alloc(block, 0);
    assert(((uintptr_t)p3 & (align - 1)) == 0);

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

int main() {
    test_create_block();
    test_alloc();
    test_reset();
    test_overflow();
    test_stats();
    test_alignment();
    test_multiple_blocks();

    printf("\nAll runtime tests passed!\n");
    return 0;
}
