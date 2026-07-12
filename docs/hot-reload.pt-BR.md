# Hot Reload no Brick

Brick suporta hot reload nativo — troque código em tempo de execução sem reiniciar o programa. Usa `dlopen`+`inotify` (Linux) ou `LoadLibrary`+`ReadDirectoryChangesW` (Windows).

## Como Funciona

1. **Compilar biblioteca compartilhada**: `brick build --shared jogo.brc -o jogo.so`
2. **Executar programa**: O runtime hot reload carrega `jogo.so` via `dlopen`
3. **Observar mudanças**: `inotify` monitora o arquivo `.so` por modificações
4. **Trocar automaticamente**: Quando recompilado, o runtime troca atomicamente os ponteiros de função
5. **Continuar execução**: Novo código roda imediatamente — sem reinicialização

## Estados

| Estado | Descrição |
|--------|-----------|
| `HR_WAITING` | Monitorando mudanças no arquivo |
| `HR_LOADING` | Recarregando a biblioteca |
| `HR_OK` | Código atualizado |
| `HR_ERROR` | Recarga falhou (ex: erro de compilação) |

## Configuração

### 1. Criar Código do Jogo

`jogo.brc`:

```brick
package JOGO
using IO

block jogo = 64MB

export fn atualizar(f32 dt) {
    print("atualizando com dt={0}", dt)
}
```

### 2. Programa Host C

```c
// main.c
#include "runtime/hot_reload.h"
#include "runtime/block_memory.h"

int main() {
    BlockCtx* jogo_mem = block_create(64 * 1024 * 1024);
    HotReloadCtx* hr = hr_create("./jogo.so", jogo_mem);

    while (rodando) {
        hr_check(hr);  // recarrega se mudou

        void (*atualizar)(float) = hr_sym(hr, "atualizar");
        if (atualizar) atualizar(0.016f);
    }

    hr_destroy(hr);
    block_destroy(jogo_mem);
    return 0;
}
```

### 3. Compilar e Executar

```bash
# Compilar jogo como biblioteca compartilhada
brick build --shared jogo.brc -o jogo.so
gcc -O3 main.c runtime/hot_reload.c runtime/block_memory.c runtime/io.c \
    -ldl -o jogo

# Executar
./jogo

# Em outro terminal, editar jogo.brc e recompilar:
brick build --shared jogo.brc -o jogo.so

# O programa em execução pega a mudança automaticamente!
```

## Limitações

- Funções precisam ser `export fn` para serem visíveis ao host
- Layout de struct deve permanecer compatível entre recargas
- Estado global no código C host persiste entre recargas
- Mudanças no layout de struct = reinicialização necessária
- Apenas lógica de função pode mudar com segurança

## Cross-Platform

| Feature | Linux | Windows |
|---------|-------|---------|
| Biblioteca dinâmica | `.so` | `.dll` |
| Observação de arquivos | `inotify` | `ReadDirectoryChangesW` |
| Carregamento | `dlopen`/`dlsym` | `LoadLibrary`/`GetProcAddress` |
| Threading | `pthreads` | `CreateThread` |
| Swap atômico | `__sync_bool_compare_and_swap` | `InterlockedExchange` |

## Veja Também

- [Guia de Início Rápido](GETTING_STARTED.pt-BR.md) — Configuração do primeiro projeto
- [Arquitetura](ARCHITECTURE.pt-BR.md) — Como o runtime se encaixa
