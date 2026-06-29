#include "../runtime/hot_reload.h"
#include "../runtime/block_memory.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#endif

void test_block_freeze_thaw(void) {
    // Just verify freeze/thaw don't crash and block_alloc still works
    // Apenas verifica se freeze/thaw nao crasham e block_alloc ainda funciona
    BlockCtx* block = block_create_bytes(1024);

    block_freeze();
    // In a real scenario this would spin, but we test the API contract
    // Em um cenario real isso giraria, mas testamos o contrato da API
    block_thaw();

    void* p = block_alloc(block, 64);
    assert(p != NULL);
    assert(block->used >= 64);

    block_destroy(block);
    printf("[PASS] test_block_freeze_thaw\n");
}

#ifndef _WIN32

// Helper: compile a .c file into a .so via system()
// Helper: compila um arquivo .c em .so via system()
static int compile_so(const char* src_path, const char* so_path) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "gcc -shared -fPIC -o %s %s 2>/dev/null",
             so_path, src_path);
    return system(cmd);
}

// Helper: write a simple library source
// Helper: escreve uma fonte de biblioteca simples
static void write_lib_src(const char* path, int version) {
    FILE* f = fopen(path, "w");
    assert(f);
    fprintf(f,
        "#include <stdio.h>\n"
        "int get_value(void) { return %d; }\n"
        "const char* get_name(void) { return \"v%d\"; }\n",
        version, version);
    fclose(f);
}

static int (*g_get_value)(void) = NULL;
static const char* (*g_get_name)(void) = NULL;

void test_create_and_load(void) {
    const char* src_path = "/tmp/mc_test_lib.c";
    const char* so_path  = "/tmp/mc_test_lib.so";

    unlink(so_path);
    write_lib_src(src_path, 1);
    assert(compile_so(src_path, so_path) == 0);

    HotReloadEngine* hr = hr_create(so_path);
    assert(hr != NULL);
    assert(hr_state(hr) == HR_WAITING);

    hr_register_func(hr, "get_value", (void**)&g_get_value);
    hr_register_func(hr, "get_name",  (void**)&g_get_name);

    assert(hr_load_initial(hr) == 0);
    assert(hr_state(hr) == HR_OK);

    assert(g_get_value() == 1);
    assert(strcmp(g_get_name(), "v1") == 0);

    hr_destroy(hr);
    unlink(so_path);
    unlink(src_path);
    printf("[PASS] test_create_and_load\n");
}

void test_reload(void) {
    const char* src_path = "/tmp/mc_test_lib.c";
    const char* so_path  = "/tmp/mc_test_lib.so";

    unlink(so_path);
    write_lib_src(src_path, 1);
    assert(compile_so(src_path, so_path) == 0);

    HotReloadEngine* hr = hr_create(so_path);
    hr_register_func(hr, "get_value", (void**)&g_get_value);
    hr_register_func(hr, "get_name",  (void**)&g_get_name);
    assert(hr_load_initial(hr) == 0);
    assert(g_get_value() == 1);

    // Recompile with version 2
    // Recompila com versao 2
    write_lib_src(src_path, 2);
    assert(compile_so(src_path, so_path) == 0);

    // Reload
    // Recarrega
    assert(hr_reload(hr) == 0);
    assert(hr_state(hr) == HR_OK);

    // Should reflect the new version
    // Deve refletir a nova versao
    assert(g_get_value() == 2);
    assert(strcmp(g_get_name(), "v2") == 0);

    hr_destroy(hr);
    unlink(so_path);
    unlink(src_path);
    printf("[PASS] test_reload\n");
}

void test_rollback(void) {
    const char* src_path = "/tmp/mc_test_lib.c";
    const char* so_path  = "/tmp/mc_test_lib.so";

    unlink(so_path);
    write_lib_src(src_path, 1);
    assert(compile_so(src_path, so_path) == 0);

    HotReloadEngine* hr = hr_create(so_path);
    hr_register_func(hr, "get_value", (void**)&g_get_value);
    assert(hr_load_initial(hr) == 0);
    assert(g_get_value() == 1);

    // Remove the .so, then try reload — should fail and keep old version
    // Remove o .so, entao tenta reload — deve falhar e manter versao antiga
    unlink(so_path);
    assert(hr_reload(hr) == -1);
    // State should still be OK (we have the old handle)
    // Estado deve continuar OK (temos o handle antigo)
    assert(hr_state(hr) == HR_OK);
    // Old function pointer should still work
    // Ponteiro de funcao antigo deve continuar funcionando
    assert(g_get_value() == 1);

    hr_destroy(hr);
    unlink(src_path);
    printf("[PASS] test_rollback\n");
}

void test_state_transitions(void) {
    HotReloadEngine* hr = hr_create("/tmp/nonexistent.so");
    assert(hr != NULL);
    assert(hr_state(hr) == HR_WAITING);
    // No initial .so exists, so load fails
    // Nenhum .so inicial existe, entao a carga falha
    assert(hr_load_initial(hr) == -1);
    assert(hr_state(hr) == HR_ERROR);
    hr_destroy(hr);
    printf("[PASS] test_state_transitions\n");
}

#endif // _WIN32

int main() {
    test_block_freeze_thaw();
#ifndef _WIN32
    test_create_and_load();
    test_reload();
    test_rollback();
    test_state_transitions();
#endif

    printf("\nAll hot reload tests passed!\n");
    return 0;
}
