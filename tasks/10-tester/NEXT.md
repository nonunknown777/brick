# Próximo Passo - Tester/Optimizer
# Next Step - Tester/Optimizer

## Sessão 9 — COMPLETA ✅ (C Interop)

- [x] C Interop: extern fn, include, link, *T pointers, String→*u8 auto
- [x] Lexer: EXTERN/INCLUDE/LINK tokens, "and" contextual
- [x] Parser: include_decl, link_decl, extern_decl, *T types
- [x] Type checker: extern_func_defs, pointer type support
- [x] Codegen: include/link emission, *u8→char*, String→.data, no extern prototypes
- [x] Builder: link_flags → -l<lib> in gcc
- [x] CLI: brick bind <header>
- [x] Example: examples/c_math.brc
- [x] Integration test: tests/test_c_interop.brc
- [x] shared-context.md: C interop section
- [x] Testes: 97/97 unitários, 6/6 integração
- [x] STATE.md atualizado para tasks 01, 02, 03, 07, 10

## Próximas ações sugeridas (prioridade)

| Prio | Task | Ação |
|------|------|------|
| 1 | **08-vscoder** | Debug webview: trocar demo data por GDB real + publicar no Marketplace |
| 2 | **09-debugger** | Pretty-printers para novos tipos C (uint8_t, int32_t, float, double, size_t, ptrdiff_t) |
| 3 | **02-parser** | Validação de conflito de nomes (`IO`/`print` vs `using IO;`) |
| 4 | **10-tester** | Smoke test — compilar todos os exemplos/ com `brick build` |
| 5 | **10-tester** | Documentação — `docs/*.md` para leigos (visão geral, tutoriais) |
| 6 | **10-tester** | Otimização — profiling do compilador (hot spots parsing/codegen) |
| 7 | **11-libs** | Próximas bibliotecas: input (teclado/mouse/gamepad), audio, file, net |

## Priorities (English)

| Prio | Task | Action |
|------|------|--------|
| 1 | **08-vscoder** | Debug webview: replace demo data with real GDB + publish to Marketplace |
| 2 | **09-debugger** | Pretty-printers for new C types (uint8_t, int32_t, float, double, size_t, ptrdiff_t) |
| 3 | **02-parser** | Name conflict validation (`IO`/`print` vs `using IO;`) |
| 4 | **10-tester** | Smoke test — compile all examples/ with `brick build` |
| 5 | **10-tester** | Documentation — `docs/*.md` for beginners (overview, tutorials) |
| 6 | **10-tester** | Optimization — compiler profiling (hot spots parsing/codegen) |
| 7 | **11-libs** | Next libraries: input (keyboard/mouse/gamepad), audio, file, net |
