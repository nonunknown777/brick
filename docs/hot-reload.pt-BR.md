# Hot Reload do Brick

> Troque código sem parar o programa.

## Visão Geral

Hot reload permite editar seu código `.brc`, salvar, e ver o resultado **sem reiniciar** o programa. Útil para game jams, editores ao vivo, e qualquer situação onde downtime é inaceitável.

```
Edita .brc  →  salva  →  compila pra .so  →  dlopen carrega  →  função trocada atomicamente
```

## Como Funciona

### Arquitetura

O hot reload usa 3 primitivas do Linux:

| Primitiva | Função |
|---|---|
| `dlopen` + `dlsym` | Carrega nova versão do .so e extrai símbolos |
| `inotify` | Monitora o arquivo .so por modificações |
| `__atomic_store` | Troca ponteiros de função atomicamente |

O motor de hot reload roda em uma **thread separada**, monitorando o diretório do .so. Quando detecta uma mudança (`IN_CLOSE_WRITE`), ele:

1. **Copia** o .so pra um arquivo temporário (dlopen cacheia por path, então copia contorna o cache)
2. **dlopen** o novo .so
3. **Swap atômico**: atualiza todos os ponteiros de função registrados com `__atomic_store`
4. **Fecha** o .so antigo com `dlclose`
5. Dispara o **callback** de notificação

### Estados

| Estado | Significado |
|---|---|
| `HR_WAITING` | Aguardando load inicial |
| `HR_LOADING` | Carregando novo .so |
| `HR_OK` | Pronto |
| `HR_ERROR` | Falha no dlopen |

## Como Usar

### Compilar com suporte a hot reload

```bash
brick build game.brc --release -o game
```

A flag `--release` compila cada package como um .so separado.

### Executar

```bash
./game
```

O motor de hot reload monitora os .so em segundo plano.

### Editar e ver resultado

Edite seu `.brc`, salve, e o binário recarrega automaticamente. A troca de função é **atômica** — não há momento onde o ponteiro aponta pra lugar nenhum.

### Ciclo típico de desenvolvimento

```bash
# Terminal 1: compila e roda
brick build mygame.brc --release -o mygame
./mygame

# Terminal 2: edita o código
vim mygame.brc    # faz alterações
# compila a nova versão
brick build mygame.brc --release -o mygame
# → hot reload detecta o novo .so e troca as funções automaticamente
```

## API de Hot Reload

### Para usuários finais

Basta usar `brick build --release`. O compilador gera os .so e o runtime cuida do resto.

### Para integração em C

```c
#include "hot_reload.h"

// 1. Cria o motor
HotReloadEngine* hr = hr_create("./build/mylib.so");

// 2. Registra funções que serão trocadas
void (*my_func)(void) = NULL;
hr_register_func(hr, "my_function", (void**)&my_func);

// 3. Carrega versão inicial
hr_load_initial(hr);

// 4. Inicia monitoramento (inotify em thread separada)
hr_start_watching(hr);

// 5. (opcional) Callback após cada reload
hr_set_callback(hr, my_callback);

// ... seu programa roda, edita o .so, e as funções são trocadas ...

// 6. Limpeza
hr_destroy(hr);
```

### Funções da API

| Função | Descrição |
|---|---|
| `hr_create(path)` | Cria motor de hot reload |
| `hr_register_func(hr, name, ptr)` | Registra função para swap |
| `hr_load_initial(hr)` | Carrega símbolos iniciais |
| `hr_start_watching(hr)` | Inicia monitoramento com inotify |
| `hr_reload(hr)` | Força recarga manual |
| `hr_state(hr)` | Retorna estado atual |
| `hr_set_callback(hr, cb)` | Define callback pós-reload |
| `hr_destroy(hr)` | Limpa recursos |

## Detalhes da Implementação

### Swap atômico

Usamos `__atomic_store` com `__ATOMIC_SEQ_CST` para garantir que todos os ponteiros de função sejam atualizados atomicamente. Leitores sempre veem um ponteiro válido.

### Congelamento de blocos

Durante o reload, o runtime congela todas as **alocações em blocos** (`block_freeze()`). Nenhuma nova alocação acontece durante a troca. Após o swap, os blocos são descongelados (`block_thaw()`).

### Cache busting

dlopen cacheia por path de arquivo. Para garantir que uma nova versão seja carregada, copiamos o .so para um path temporário antes de chamar dlopen.

### Rollback

Se o dlopen da nova versão falhar, o sistema mantém o handle antigo e os ponteiros de função intocados. O programa continua rodando com a versão anterior.

### Atraso de segurança

Um `nanosleep` de 50ms entre a detecção de mudança e o reload garante que a escrita do arquivo .so tenha terminado completamente.

## Boas Práticas

- **Estruturas de dados estáveis**: não mude o layout das structs entre versões (ABI precisa ser compatível)
- **Estado global mínimo**: variáveis globais não são recarregadas — passe estado via structs
- **Callbacks**: use `hr_set_callback` para reconfigurar estado após cada reload

## Limitações

| Limitação | Motivo | Alternativa |
|---|---|---|
| Só Linux | dlopen + inotify são POSIX | Windows support planejado |
| .so por package | Cada package vira um .so separado | Organize packages por módulo |
| Dados não recarregam | Só código (funções) é trocado | Use structs estáveis |
| ABI deve ser compatível | Ponteiros de função esperam mesma assinatura | Congele a interface pública |

## Comparação

| Brick Hot Reload | Live++ / C++ Modules |
|---|---|
| Swap atômico de ponteiros de função | Recompilação de TU inteiras |
| Sem custo de runtime quando não usado | Overhead de instrumentação |
| Em C puro, sem runtime C++ | Requer runtime C++ |
| Simples e previsível | Complexo e estado-da-arte |

## Exemplo Completo

Consulte os testes em `tests/test_hot_reload.c` e `tests/test_libs_window_hr.c` para exemplos completos de uso da API.
