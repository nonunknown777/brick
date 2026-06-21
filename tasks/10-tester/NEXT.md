# Próximo Passo - Tester/Optimizer
# Next Step - Tester/Optimizer

## CLI Visualizer + Release — COMPLETA ✅
## CLI Visualizer + Release — COMPLETE ✅

Tudo implementado e testado:
Everything implemented and tested:

- [x] `meta-c --visualize` — TUI ncurses embedded
- [x] `meta-c --attach <pid>` — attach a processo rodando
- [x] `build-release.sh` documenta ncurses como dependência
- [x] 118/118 testes passando
- [x] `/tmp/meta-c-mem-<pid>.bin` gerado e lido pelo visualizer

- [x] `meta-c --visualize` — embedded ncurses TUI
- [x] `meta-c --attach <pid>` — attach to running process
- [x] `build-release.sh` documents ncurses as dependency
- [x] 118/118 tests passing
- [x] `/tmp/meta-c-mem-<pid>.bin` generated and read by visualizer

## Sugestões futuras (prioridade)
## Future suggestions (priority)

1. **08-vscoder Debug Webview**: trocar dados demo por GDB real (3 abordagens cascata)
2. **02-parser validação**: conflito de nomes `IO`/`print` com `using IO;`
3. **01-lexer**: investigar os 10% faltantes (escapes? hex? comentários?)
4. **Otimização**: profiling do compilador, SIMD no runtime, tuning bump alloc
5. **Documentação**: comentários inline no código, docs *.md pra leigos

1. **08-vscoder Debug Webview**: replace demo data with real GDB (3 cascade approaches)
2. **02-parser validation**: name conflict `IO`/`print` with `using IO;`
3. **01-lexer**: investigate remaining 10% (escapes? hex? comments?)
4. **Optimization**: compiler profiling, SIMD in runtime, bump alloc tuning
5. **Documentation**: inline comments in code, docs *.md for beginners
