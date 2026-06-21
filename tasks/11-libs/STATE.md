# Current State

## Session: 1 (active)
## Sessão: 1 (ativa)

Progress: 50%
Progresso: 50%

Next step: Hot reload support implemented — function pointer table, .so build, 3 HR tests passing
Próximo passo: Suporte a hot reload implementado — tabela de ponteiros de função, build .so, 3 testes HR passando

Last action: Created window_hr.h/c with MetaWindowFuncTable + meta_window_hr_init/start
Última ação: Criado window_hr.h/c com MetaWindowFuncTable + meta_window_hr_init/start

Blockers: None
Bloqueios: Nenhum

## Delivered
## Entregue

- window.h: BlockCtx* API with forward declaration
- window_internal.h: char title[256] (no strdup/free)
- window_linux/window_win32.c: block_alloc instead of calloc
- window_hr.h/c: HotReloadEngine integration, atomic function swap
- SCons: .so build for dlopen, test_libs_window_hr, -rdynamic linker flag
- tests: 15 base tests + 3 HR tests — all passing, ASan clean
- hot_reload.c: copy-to-temp approach instead of dlmopen (fixes symbol visibility)

- window.h: API BlockCtx* com forward declaration
- window_internal.h: char title[256] (sem strdup/free)
- window_linux/window_win32.c: block_alloc em vez de calloc
- window_hr.h/c: integração HotReloadEngine, swap atômico de função
- SCons: build .so para dlopen, test_libs_window_hr, flag linker -rdynamic
- testes: 15 testes base + 3 testes HR — todos passando, ASan limpo
- hot_reload.c: abordagem copy-to-temp em vez de dlmopen (corrige visibilidade de símbolo)
