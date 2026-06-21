# Progresso - Hot Reload
# Progress - Hot Reload

- [x] hr_create: criar engine, preparar paths
- [x] hr_create: create engine, prepare paths
- [x] hr_register_func: mapear nome → ponteiro para função
- [x] hr_register_func: map name → function pointer
- [x] hr_load_initial: dlopen + dlsym para carga inicial
- [x] hr_load_initial: dlopen + dlsym for initial load
- [x] hr_reload: dlmopen + swap atômico + rollback
- [x] hr_reload: dlmopen + atomic swap + rollback
- [x] inotify: monitorar diretório, filtrar por basename
- [x] inotify: monitor directory, filter by basename
- [x] Thread separada (pthread) para monitoramento
- [x] Separate thread (pthread) for monitoring
- [x] Rollback em caso de falha (mantém handle anterior)
- [x] Rollback on failure (keeps previous handle)
- [x] Estados: WAITING, LOADING, OK, ERROR
- [x] States: WAITING, LOADING, OK, ERROR
- [x] Callback de notificação
- [x] Notification callback
- [x] Integração com block_memory: freeze/thaw durante swap
- [x] Integration with block_memory: freeze/thaw during swap
- [x] Testes: create, load, reload, rollback, states, freeze/thaw
- [x] Tests: create, load, reload, rollback, states, freeze/thaw
- [x] Build com SCons
- [x] Build with SCons
