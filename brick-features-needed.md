# Brick — Features Necessárias para Reduzir Dependência de C

> **Propósito:** Guia de implementação para o compilador Brick.
> Baseado na implementação prática de uma engine gráfica (bgfx + GLFW) onde ~60% da lógica
> precisou ficar em C porque o Brick v0.6.0 não tinha suporte para arrays locais,
> narrowing, hex literals, polimorfismo dinâmico, etc.
>
> **Alvo:** Compilador Brick (C++) em `src/`

---

## Sumário

1. [Prioridade Máxima — Bloqueantes](#1-prioridade-máxima--bloqueantes)
   - [1.1 Arrays Locais na Stack](#11-arrays-locais-na-stack)
   - [1.2 Hex Literals](#12-hex-literals)
   - [1.3 Narrowing Explícito (Cast de Tipos)](#13-narrowing-explícito-cast-de-tipos)
   - [1.4 Array Literals como Argumento](#14-array-literals-como-argumento)
2. [Prioridade Alta — Polimorfismo e Cenas](#2-prioridade-alta--polimorfismo-e-cenas)
   - [2.1 Arrays Dinâmicos em Structs](#21-arrays-dinâmicos-em-structs)
   - [2.2 `impl Struct : Interface` Separado](#22-impl-struct--interface-separado)
   - [2.3 Dispatch Virtual / Ponteiro de Método](#23-dispatch-virtual--ponteiro-de-método)
3. [Prioridade Média — Ergonomia](#3-prioridade-média--ergonomia)
   - [3.1 `error("msg")` Corrigido](#31-errormsg-corrigido)
   - [3.2 Inicializadores de Struct como Literal](#32-inicializadores-de-struct-como-literal)
   - [3.3 Constantes com Valores de Expressão](#33-constantes-com-valores-de-expressão)
   - [3.4 `include` com Path Relativo](#34-include-com-path-relativo)
4. [Prioridade Baixa — Nice-to-have](#4-prioridade-baixa--nice-to-have)
   - [4.1 Enum com Valores Hex](#41-enum-com-valores-hex)
   - [4.2 Alinhamento de Struct e `@packed`](#42-alinhamento-de-struct-e-packed)
   - [4.3 sizeof / alignof em Tempo de Compilação](#43-sizeof--alignof-em-tempo-de-compilação)
5. [Arquivos do Compilador a Modificar](#5-arquivos-do-compilador-a-modificar)
6. [Testes de Regressão](#6-testes-de-regressão)

---

## 1. Prioridade Máxima — Bloqueantes

Estas features impedem o uso de Brick para ~60% das tarefas de engine.
Sem elas, a maior parte da lógica de rendering, geometria e matemática
precisa ficar em C.

---

### 1.1 Arrays Locais na Stack

#### Problema

Brick v0.6.0 não aceita declaração de arrays com tamanho fixo na stack.

```brick
// ❌ Brick v0.6.0 — syntax error
fn create_cube() {
    f32 verts[24] = { ... }   // "unexpected token '['"
    u16 indices[36]           // "unexpected token '['"
}

// ❌ Também não aceita
f32[] verts = f32[24]         // "unexpected token '['"

// ❌ Alocação dinâmica com tamanho literal
f32 m[16]                     // syntax error
```

#### Onde é necessário

**Arquivo real:** `src/engine.c` — `engine_mesh_create_cube()` (24 vertices, 36 indices)
**Arquivo real:** `src/engine.c` — `mat4_identity/perspective/look_at` (arrays `float[16]`, `float[3]`)
**Arquivo real:** `src/engine.c` — `engine_mesh_set_rotation_y()` (matriz de rotação 4x4)

Exemplo completo do que não pode ser escrito em Brick:

```brick
// ═══ O que queremos escrever em Brick ═══

fn mat4_identity(*f32 m) {
    for i32 i = 0; i < 16; i++ {
        m[i] = 0.0
    }
    m[0] = 1.0
    m[5] = 1.0
    m[10] = 1.0
    m[15] = 1.0
}

fn engine_mesh_create_cube() -> u16 {
    // ─── 24 vértices: 3 floats (pos) + 1 u32 (color) = 7 fields ───
    f32 v[168]   // ← PRECISA DE ARRAY LOCAL
    u16 idx[36]  // ← PRECISA DE ARRAY LOCAL

    // Preencher vértices...
    // Chamar bgfx_create_vertex_buffer...
    // Retornar ID da mesh
}
```

#### C gerado esperado

```c
// O que o codegen deve gerar
static inline uint16_t engine_mesh_create_cube(void) {
    float v[168];
    uint16_t idx[36];
    // ... preenchimento ...
}
```

#### Implementação no Compilador

**Arquivos:** `parser/parser.cpp`, `codegen/codegen.cpp`, `codegen/type_checker.cpp`

1. **Parser:** Aceitar sintaxe `T nome[N]` e `T nome[N] = { ... }` em:
   - Escopo local de função
   - Parâmetros de função (já funciona como `*T`)
   - Inicialização com lista `{ val1, val2, ... }`

2. **Type Checker:**
   - `T[N]` é um tipo de array com tamanho conhecido em tempo de compilação
   - `sizeof(T[N])` = `N * sizeof(T)`
   - Não permitir `T[N]` como campo de struct heap (block-allocated) — apenas stack
   - Permitir `T[N]` como campo de struct se a struct inteira for stack-local
   - Decaimento implícito para `*T` ao passar para `extern fn`

3. **Codegen:**
   - Gerar declaração C `T nome[N];` no escopo local
   - Inicializadores `{ ... }` são copiados diretamente
   - Se houver inicializador, gerar `T nome[N] = { ... };`

4. **Casos de borda:**
   - `N` deve ser constante em tempo de compilação (literal ou `const`)
   - Array multidimensional: `f32 m[4][4]` → gerar `float m[4][4];`
   - Atribuição entre arrays: proibido (C já não permite), usar `memcpy` via extern
   - `f32 m[16] = {0}` → zerar tudo

#### Testes

```brick
// test_array_local.brc
fn main() {
    f32 arr[4]                         // declaração simples
    i32 nums[3] = {10, 20, 30}          // com inicializador
    f32 ident[16] = {0}                 // zero-inicializado
    ident[0] = 1.0
    ident[5] = 1.0
    ident[10] = 1.0
    ident[15] = 1.0
    print(\"{0}\", ident[0])             // deve imprimir 1.0
}
```

---

### 1.2 Hex Literals

#### Problema

Brick v0.6.0 interpreta `0x443355FF` como `0` (descarta tudo após `x`).

```brick
// ❌ Brick v0.6.0 — ambos viram 0
u32 color = 0x443355FF
u32 flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH

// ✅ Workaround atual — ilegível e propenso a erro
u32 color = 1144215038   // ninguém sabe que cor é essa
```

#### Onde é necessário

**Toda** interação com bgfx, gráficos, cores, e flags de sistema usa hex:
- Cores RGBA: `0x443355FF` (roxo), `0xFF0000FF` (vermelho), etc.
- Flags de estado: `BGFX_STATE_DEFAULT` = `0ULL`, `BGFX_CLEAR_COLOR` = `0x00010000`
- Bitmasks: `BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT`
- Qualquer constante de biblioteca C

Exemplo de `engine.c` que não pode ser em Brick sem hex:

```brick
// ═══ O que queremos escrever ═══

fn engine_frame_begin(u32 clear_color) {
    bgfx_set_view_clear(0,
        BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,   // ← hex flags
        clear_color,
        1.0, 0)
}

fn main() {
    // Cores em hex são MUITO mais legíveis
    engine_frame_begin(0x443355FF)  // roxo
    // vs
    engine_frame_begin(1144215038)  // ????
}
```

#### C gerado esperado

```c
// input: u32 color = 0x443355FF
// output:
uint32_t color = 0x443355FF;

// input: u64 flags = 0x00010000 | 0x00000002
// output:
uint64_t flags = 0x00010000 | 0x00000002;
```

#### Implementação no Compilador

**Arquivos:** `lexer/lexer.cpp`, possivelmente `codegen/codegen.cpp`

1. **Lexer:** Detectar padrão `0x[0-9a-fA-F]+` e:
   - Converter para valor inteiro (usar `strtoull` com base 16)
   - Armazenar como literal inteiro com a base preservada para formatação

2. **Codegen:** Ao gerar C, usar `0x%X` para literais que foram escritos como hex:
   - Se o usuário escreveu `0x443355FF`, gerar `0x443355FF` no C
   - Se o usuário escreveu `1144215038`, gerar `1144215038`
   - Isso garante que o código C gerado seja legível e debuggável

3. **Type inference:**
   - `0x443355FF` cabe em `u32` → tipo `u32`
   - `0xFFFFFFFFFFFFFFFF` → tipo `u64`
   - Sufixo explícito: `0xFFu64`, `0xFFu16`, `0xFFu8`

4. **Casos de borda:**
   - Hex negativo? Não existe em C — `-0xFF` é `-(0xFF)` → tratar como expressão
   - Overflow: `0x1FFFFFFFFFFFFFFFF` em u32 → erro compile-time
   - Hexadecimal + ponto flutuante: `0x1.0p-1` (C99 hex float) — opcional, baixa prioridade

#### Testes

```brick
fn main() {
    u32 color = 0x443355FF
    u16 low   = 0x00FF
    u64 big   = 0xFFFFFFFF00000000
    i32 neg   = -0x7FFFFFFF

    print(\"{0}\", color == 1144215038)   // true
    print(\"{0}\", low)                    // 255
    print(\"{0}\", big != 0)              // true
}
```

---

### 1.3 Narrowing Explícito (Cast de Tipos)

#### Problema

Brick proíbe narrowing (f64→f32, i64→i32) mesmo com cast explícito.

```brick
// ❌ Brick v0.6.0 — narrowing error
fn update(f64 dt) {
    f32 angle = dt * 0.5         // "narrowing: f64 → f32"
}

f64 time = glfwGetTime()
f32 sec = time                   // narrowing error
```

#### Onde é necessário

bgfx usa `float` (f32) para tudo, mas GLFW/clock retornam `double` (f64).
Qualquer conversão de tempo, ângulo ou posição exige narrowing.

**Arquivo real:** `src/engine.c` — toda função de matemática (mat4, rotacao) usa `float`
**Arquivo real:** `src/engine.c` — `engine_mesh_set_rotation_y()` recebe `float angle`
**Arquivo real:** `src/main.brc` — `rotation = rotation + 0.016` (workaround: ignora dt real)

```brick
// ═══ O que queremos escrever ═══

fn update(f64 dt) {
    f32 angle = f32(dt)         // cast explícito, narrowing OK
    engine_mesh_set_rotation_y(mesh, angle)
}

// Ou com sintaxe de operador
fn update(f64 dt) {
    f32 angle = dt as f32       // alternativa
}
```

#### C gerado esperado

```c
// input: f32 angle = f32(dt)
// output:
float angle = (float)(dt);

// input: i32 small = i32(big_value)
// output:
int32_t small = (int32_t)(big_value);
```

#### Implementação no Compilador

**Arquivos:** `parser/parser.cpp`, `codegen/type_checker.cpp`, `codegen/codegen.cpp`

1. **Parser:** Aceitar sintaxe de função-like cast:
   - `T(expr)` onde T é um tipo, ex: `f32(dt)`, `i32(x)`, `u16(n)`
   - Opcional: `expr as T` com operador `as`

2. **Type Checker:**
   - Se `T(expr)` e `typeof(expr)` é diferente de `T`:
     - Se é widening: OK, sem cast necessário no C
     - Se é narrowing: **permitir** com aviso (ou sem aviso se explícito)
   - Se `T` e `typeof(expr)` são incompatíveis (ex: `*u8` para `f32`): erro

3. **Codegen:**
   - Gerar `(T)(expr)` no C
   - Ex: `f32(dt)` → `(float)(dt)`, `u16(n)` → `(uint16_t)(n)`

4. **Casos de borda:**
   - `i32(u32_val)` — signed↔unsigned com mesmo rank: permitir (C faz isso)
   - `u32(f32_val)` — float→int: gerar `(uint32_t)(val)`, possível perda de dados
   - `f32(i32_val)` — int→float: OK, sem perda para valores até ~16 milhões

#### Testes

```brick
fn main() {
    f64 dt = 0.016
    f32 dt32 = f32(dt)           // narrowing explícito
    i32 x = 42
    f32 fx = f32(x)               // widening, OK sem cast
    u16 small = u16(65535)        // narrowing seguro
    // i32 bad = i32(3.14)        // narrowing com perda, mas compila
}
```

---

### 1.4 Array Literals como Argumento

#### Problema

Brick não permite passar arrays literais como argumento para `extern fn`.

```brick
// ❌ Brick v0.6.0 — "unexpected token '['"
engine_camera_look_at(0, [0.0, 0.0, -5.0], [0.0, 0.0, 0.0], [0.0, 1.0, 0.0])

// ❌ Também não funciona em contextos menores
engine_set_view_transform(0, [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1], [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1])
```

#### Workaround atual

```c
// engine.c — função auxiliar com parâmetros escalares
void engine_camera_look_at_f(float eye_x, float eye_y, float eye_z,
                               float at_x, float at_y, float at_z) {
    float eye[3] = {eye_x, eye_y, eye_z};
    float at[3] = {at_x, at_y, at_z};
    float up[3] = {0.0f, 1.0f, 0.0f};
    engine_camera_look_at(0, eye, at, up);
}
```

```brick
// main.brc — chamada escalar (funciona mas verboso)
engine_camera_look_at_f(0.0, 0.0, -5.0, 0.0, 0.0, 0.0)
```

#### O que queremos

```brick
// ═══ Direto, sem wrapper C ═══

fn render() {
    engine_set_view_transform(0,
        [1.0, 0.0, 0.0, 0.0,
         0.0, 1.0, 0.0, 0.0,
         0.0, 0.0, 1.0, 0.0,
         0.0, 0.0, 0.0, 1.0],
        [1.0, 0.0, 0.0, 0.0,
         0.0, 1.0, 0.0, 0.0,
         0.0, 0.0, 1.0, 0.0,
         0.0, 0.0, 0.0, 1.0])

    engine_camera_look_at(0,
        [0.0, 0.0, -5.0],
        [0.0, 0.0, 0.0],
        [0.0, 1.0, 0.0])
}
```

#### C gerado esperado

```c
// input: engine_camera_look_at(0, [0.0, 0.0, -5.0], [0.0, 0.0, 0.0], [0.0, 1.0, 0.0])

// output — o compilador cria um temporário anônimo
float _tmp0[3] = {0.0f, 0.0f, -5.0f};
float _tmp1[3] = {0.0f, 0.0f, 0.0f};
float _tmp2[3] = {0.0f, 1.0f, 0.0f};
engine_camera_look_at(0, _tmp0, _tmp1, _tmp2);
```

#### Implementação no Compilador

**Arquivos:** `parser/parser.cpp`, `codegen/codegen.cpp`

1. **Parser:** Detectar `[expr, expr, ...]` como expressão array literal
2. **Type Checker:** Inferir tipo como `T[N]`, decair para `*T` se necessário
3. **Codegen:** Para cada array literal em argumento de função:
   - Criar variável temporária com nome único (ex: `_arr_<id>`)
   - Gerar declaração + inicializador antes da chamada
   - Passar a variável como argumento

4. **Casos de borda:**
   - Array literal aninhado: `[[1,2],[3,4]]` para `f32[2][2]`
   - Array vazio: `[]` — não permitir (tamanho zero)
   - Array literal como valor de retorno: `return [1, 2, 3]` — não permitir (lifetime)

#### Testes

```brick
include "engine.h"

extern fn engine_camera_look_at(u16 id, *f32 eye, *f32 at, *f32 up)

fn main() {
    // Array literal como argumento
    engine_camera_look_at(0, [0.0, 0.0, -5.0], [0.0, 0.0, 0.0], [0.0, 1.0, 0.0])
}
```

---

## 2. Prioridade Alta — Polimorfismo e Cenas

Estas features permitiriam o sistema de SceneManager com dispatch dinâmico
100% em Brick.

---

### 2.1 Arrays Dinâmicos em Structs ✅ IMPLEMENTADO

**Status:** ✅ COMPLETO (Fase 2.3 do cronograma)
- `T[]` como campo de struct → expandido para `T* name; int64_t name_cnt; int64_t name_cap;`
- `.len` → acessa `name_cnt`
- `.cap` → acessa `name_cap`
- `arr[i]` → indexação via ponteiro C nativo
- Pendente: `append()` built-in para adicionar elementos

#### Problema (resolvido)

Brick v0.6.0 não aceitava `T[]` como campo de struct:

```brick
// ❌ Brick v0.6.0 — "unexpected token '['"
struct SceneManager {
    Scene[] scenes      // syntax error
    i32 count
}
```

#### Onde é necessário

SceneManager precisa armazenar uma lista de cenas para iterar no game loop.

```brick
// ═══ O que queremos ═══

struct SceneManager {
    Scene[] scenes      // array dinâmico de interfaces
    i32 count
    i32 current
}

impl SceneManager {
    fn add(Scene s) {
        scenes = scenes + [s]   // append
        count = count + 1
    }

    fn switch(i32 idx) {
        if idx >= 0 and idx < count {
            current = idx
        }
    }

    fn render(f64 dt) {
        if count > 0 {
            scenes[current].render(dt)
        }
    }
}
```

#### C gerado esperado

```c
typedef struct {
    Scene* scenes;       // ponteiro para heap do block
    int32_t count;
    int32_t current;
    BrickBlockCtx* _block;  // block onde o array foi alocado
} SceneManager;
```

#### Implementação no Compilador

**Arquivos:** `codegen/type_checker.cpp`, `codegen/codegen.cpp`, `parser/parser.cpp`

1. **Parser:** Aceitar `T[]` como tipo de campo em struct
2. **Type Checker:** `T[]` como campo de struct deve:
   - Armazenar um ponteiro para o data (allocado em block)
   - Armazenar o tamanho atual (count)
   - Armazenar a capacidade atual (capacity)
   - Referência ao block de alocação
3. **Codegen:** Gerar struct com campos ocultos de runtime
4. **Operações:** `arr + [item]` (append), `arr[i]` (index), `arr.len` (length)

---

### 2.2 `impl Struct : Interface` Separado

#### Problema

Brick não suporta `impl CubeScene : Scene { }` — a implementação de interface
deve ser inline na struct.

```brick
// ❌ Brick v0.6.0 — "unexpected token '{'"
impl CubeScene : Scene {
    fn init() { }
    fn update(f64 dt) { }
    fn render(f64 dt) { }
    fn shutdown() { }
}
```

#### Workaround (funciona mas verboso)

```brick
struct CubeScene {
    u16 mesh
    u16 prog
    f32 rotation

    fn init() {
        mesh = engine_mesh_create_cube()
        prog = engine_program_create("vs.bin", "fs.bin")
    }

    fn update(f64 dt) {
        rotation = rotation + f32(dt)
    }

    fn render(f64 dt) {
        engine_mesh_set_rotation_y(mesh, rotation)
        engine_draw_mesh(mesh, prog)
    }

    fn shutdown() {
        engine_mesh_destroy(mesh)
        engine_program_destroy(prog)
    }
}
```

#### O que queremos

```brick
// ═══ Separação clara entre declaração e implementação ═══

interface Scene {
    fn init()
    fn update(f64 dt)
    fn render(f64 dt)
    fn shutdown()
}

struct CubeScene {
    u16 mesh
    u16 prog
    f32 rotation
}

impl CubeScene : Scene {
    fn init() {
        mesh = engine_mesh_create_cube()
        prog = engine_program_create("vs.bin", "fs.bin")
    }

    fn update(f64 dt) {
        rotation = rotation + f32(dt)
    }

    fn render(f64 dt) {
        engine_mesh_set_rotation_y(mesh, rotation)
        engine_draw_mesh(mesh, prog)
    }

    fn shutdown() {
        engine_mesh_destroy(mesh)
        engine_program_destroy(prog)
    }
}
```

#### C gerado esperado

```c
typedef struct {
    uint16_t mesh;
    uint16_t prog;
    float rotation;
} CubeScene;

static inline void CubeScene_init(CubeScene* this) { ... }
static inline void CubeScene_update(CubeScene* this, double dt) { ... }
static inline void CubeScene_render(CubeScene* this, double dt) { ... }
static inline void CubeScene_shutdown(CubeScene* this) { ... }
```

---

### 2.3 Dispatch Virtual / Ponteiro de Método

#### Problema

Sem dispatch virtual ou ponteiros de método, não é possível chamar
`scene.render(dt)` polimorficamente.

```brick
// ❌ O que não funciona
Scene[] scenes
scenes[0].render(dt)  // não sabe qual método chamar
```

#### O que seria necessário

**Opção A — Tabela Virtual (vtbl):**

```brick
interface Scene {
    fn init()
    fn update(f64 dt)
    fn render(f64 dt)
    fn shutdown()
}

// Compilador gera vtbl automaticamente:
// struct Scene_Vtbl {
//     void (*init)(void*);
//     void (*update)(void*, double);
//     void (*render)(void*, double);
//     void (*shutdown)(void*);
// };
// struct Scene {
//     Scene_Vtbl* vtbl;
//     void* data;
// };
```

**Opção B — Ponteiro de Função com Captura:**

```brick
// Declarar ponteiro de método
type SceneRender = fn(*void, f64)  // função que recebe *void e f64

struct SceneSlot {
    *void instance
    SceneRender render_fn
}
```

#### Prioridade

Esta feature tem **menor prioridade** que arrays dinâmicos em structs (2.1),
pois sem arrays polimórficos não há onde armazenar as cenas. Recomenda-se:

1. Primeiro implementar arrays dinâmicos em structs (2.1)
2. Depois vtbl para interfaces (2.2 + 2.3 combinados)

---

## 3. Prioridade Média — Ergonomia

Features que melhoram a qualidade de vida mas não bloqueiam completamente
o uso de Brick.

---

### 3.1 `error("msg")` Corrigido

#### Problema

`error("msg")` passa um `BrickString` struct para `fprintf` em vez de `char*`.

```brick
// ❌ Gera C quebrado
error("deu ruim")
// gera: fprintf(stderr, brick_string);  ← ERRO: fprintf espera char*, não struct
```

#### C gerado atual (quebrado)

```c
// input: error("deu ruim")
// output (ATUAL — QUEBRADO):
// BrickString _msg = ...;
// fprintf(stderr, _msg);   // <-- passa struct em vez de char*
```

#### C gerado esperado (corrigido)

```c
// output (CORRIGIDO):
// BrickString _msg = ...;
// fprintf(stderr, "%.*s", (int)_msg.len, _msg.data);
// ou
// fprintf(stderr, "%s", _msg.data);
```

#### Implementação

**Arquivos:** `codegen/codegen.cpp`

1. Encontrar onde `error("msg")` gera a chamada `fprintf`
2. Substituir para acessar `_msg.data` em vez da struct inteira
3. Usar `"%s"` ou `"%.*s"` com o campo `data`

---

### 3.2 Inicializadores de Struct como Literal

#### Problema

Não há sintaxe concisa para criar struct temporária.

```brick
// ❌ Não funciona
engine_camera_look_at(0, Vec3{0,0,-5}, Vec3{0,0,0}, Vec3{0,1,0})

// ❌ Também não
Vec3 eye = {0, 0, -5}   // inicializador estilo C não funciona
```

#### O que queremos

```brick
// ═══ Inicializador nomeado ═══

struct Vec3 { f32 x, y, z }

fn main() {
    Vec3 eye = Vec3{ .x = 0.0, .y = 0.0, .z = -5.0 }
    Vec3 at  = Vec3{ 0.0, 0.0, 0.0 }            // posicional

    engine_camera_look_at(0, &eye, &at, &Vec3{0, 1, 0})  // temporário
}
```

---

### 3.3 Constantes com Valores de Expressão

#### Problema

Constantes só aceitam literais, não expressões.

```brick
// ❌ Não compila
const FOV_RADIANS = 60.0 * 3.14159 / 180.0
const HALF_WIDTH = 1280 / 2
```

#### O que queremos

```brick
// ═══ Expressões constantes ═══

const FOV_DEG = 60.0
const FOV_RAD = FOV_DEG * 3.14159265 / 180.0
const WINDOW_W = 1280
const HALF_W = WINDOW_W / 2
```

---

### 3.4 `include` com Path Relativo

#### Problema

O `include` atual gera `#include <header.h>` (angle brackets), que o compilador
C resolve via `-I`. Isso funciona, mas:

1. Não diferencia headers do sistema de headers do projeto
2. O compilador C não sabe a ordem de resolução

#### Melhoria

```brick
// Atual: sempre angle brackets
include "engine.h"       // #include <engine.h>

// Sugestão: suportar ambos
include <engine.h>       // #include <engine.h> (system header)
include "engine.h"       // #include "engine.h" (local header, busca primeiro no diretório)

// Ou:
include <bgfx/c99/bgfx.h>
include_local "engine.h"
```

---

## 4. Prioridade Baixa — Nice-to-have

Features que seriam úteis mas não são críticas.

### 4.1 Enum com Valores Hex

```brick
// ═══ Útil para flags de bgfx ═══

enum ClearFlags {
    COLOR = 0x00010000
    DEPTH = 0x00000002
    STENCIL = 0x00000004
}

enum TextureFlags {
    RT         = 0x00000010
    MIN_POINT  = 0x00000020
    MAG_POINT  = 0x00000040
    U_CLAMP    = 0x00000100
    V_CLAMP    = 0x00000200
}
```

### 4.2 Alinhamento de Struct e `@packed`

```brick
// ═══ Para GPU data (vértices, uniformes) ═══

@packed
struct Vertex_PosColor {
    f32 x, y, z
    u32 color
}

@align(16)
struct Mat4 {
    f32 m[16]
}
```

### 4.3 sizeof / alignof em Tempo de Compilação

```brick
// ═══ Útil para alocação de buffers ═══

fn main() {
    i32 vert_size = sizeof(Vertex_PosColor)  // 16
    i32 align = alignof(Mat4)                // 16

    // Alocar buffer com tamanho calculado
    // u8[] vert_data = u8[vert_size * 24] @temp
}
```

---

## 5. Arquivos do Compilador a Modificar

Com base na arquitetura do Brick (`brick-language.md` seção 27):

| Feature | Lexer | Parser | Type Checker | Codegen |
|---------|-------|--------|--------------|---------|
| **1.1 Arrays locais** | — | `parser.cpp` | `type_checker.cpp` | `codegen.cpp` |
| **1.2 Hex literals** | `lexer.cpp` | — | — | `codegen.cpp` |
| **1.3 Narrowing** | — | `parser.cpp` | `type_checker.cpp` | `codegen.cpp` |
| **1.4 Array literals** | — | `parser.cpp` | `type_checker.cpp` | `codegen.cpp` |
| **2.1 Arrays em struct** | — | `parser.cpp` | `type_checker.cpp` | `codegen.cpp` |
| **2.2 impl separado** | — | `parser.cpp` | `type_checker.cpp` | `codegen.cpp` |
| **2.3 Vtbl** | — | `parser.cpp` | `type_checker.cpp` | `codegen.cpp` |
| **3.1 error() fix** | — | — | — | `codegen.cpp` |
| **3.2 Init literals** | — | `parser.cpp` | `type_checker.cpp` | `codegen.cpp` |
| **3.3 Const expr** | — | `parser.cpp` | `type_checker.cpp` | `codegen.cpp` |
| **3.4 include local** | — | `parser.cpp` | — | — |
| **4.1 Enum hex** | — | `parser.cpp` | `type_checker.cpp` | `codegen.cpp` |
| **4.2 @packed/@align** | — | `parser.cpp` | — | `codegen.cpp` |
| **4.3 sizeof** | — | — | `type_checker.cpp` | `codegen.cpp` |

### Ordem recomendada de implementação

```
Fase 1 (máximo impacto, mínimo esforço):
  1.2 Hex literals     → ~1-2 dias (só lexer + codegen)
  1.3 Narrowing cast   → ~2-3 dias (parser + type_checker + codegen)
  3.1 error() fix      → ~1 dia (só codegen)

Fase 2 (liberta a engine):
  1.1 Arrays locais    → ~3-5 dias (parser + type_checker + codegen)
  1.4 Array literals   → ~2-3 dias (parser + codegen)

Fase 3 (sistema de cenas em Brick):
  2.1 Arrays em structs → ~5-7 dias (parser + type_checker + codegen + runtime)
  2.2 impl separado     → ~2-3 dias (parser + codegen)
  2.3 Vtbl              → ~5-7 dias (parser + type_checker + codegen)

Fase 4 (polimento):
  3.2 Init literals     → ~2-3 dias
  3.3 Const expr        → ~2-3 dias
  3.4 include local     → ~1 dia
  4.1-4.3               → ~3-5 dias cada
```

---

## 6. Testes de Regressão

Cada feature deve ter testes no formato Brick (`.brc`) que compilam e rodam.

### Estrutura de Testes

```
brick/
  tests/
    features/
      test_hex_literal.brc
      test_array_local.brc
      test_narrowing_cast.brc
      test_array_literal.brc
      test_impl_separate.brc
      test_error_fix.brc
    compile_only/       # Testes que só precisam compilar
      ...
    runtime/            # Testes que precisam executar e verificar output
      ...
```

### Exemplo de Teste — Hex Literal

```brick
// tests/features/test_hex_literal.brc
using IO

fn main() {
    u32 color = 0x443355FF
    u32 expected = 1144215038

    if color != expected {
        print("FAIL: hex literal {0} != {1}", color, expected)
        exit(1)
    }

    // Testar zero
    u32 zero = 0x0
    if zero != 0 {
        print("FAIL: hex zero")
        exit(1)
    }

    // Testar flags combinadas
    u32 flags = 0x00010000 | 0x00000002
    if flags == 0 {
        print("FAIL: hex flags")
        exit(1)
    }

    print("PASS: all hex tests")
}
```

### Exemplo de Teste — Array Local

```brick
// tests/features/test_array_local.brc
using IO

fn main() {
    f32 m[16]
    for i32 i = 0; i < 16; i++ {
        m[i] = 0.0
    }
    m[0] = 1.0
    m[5] = 1.0
    m[10] = 1.0
    m[15] = 1.0

    // Verificar identidade
    if m[0] != 1.0 or m[15] != 1.0 {
        print("FAIL: identity matrix")
        exit(1)
    }
    if m[1] != 0.0 {
        print("FAIL: zero element")
        exit(1)
    }

    print("PASS: all array tests")
}
```

---

## Apêndice A: Código Atual em C que Poderia Migrar para Brick

| Função em C | Arquivo | Depende de |
|-------------|---------|------------|
| `engine_mesh_create_cube()` | `engine.c` | Arrays locais (1.1), Hex (1.2) |
| `engine_mesh_set_rotation_y()` | `engine.c` | Arrays locais (1.1) |
| `engine_camera_look_at()` | `engine.c` | Array literals (1.4) |
| `engine_frame_limit()` | `engine.c` | Nenhuma — já pode migrar! |
| Debug text / input wrappers | `engine.c` | Nenhuma — já pode migrar! |
| `mat4_identity/perspective/look_at` | `engine.c` | Arrays locais (1.1), Narrowing (1.3) |
| SceneManager completo | — | Arrays em struct (2.1), Vtbl (2.3) |
| `main.brc` flat scene | `main.brc` | Nenhuma — já está em Brick! |

### O que JÁ PODE SER FEITO EM BRICK HOJE (sem mudanças no compilador)

```brick
// ✅ Já funciona! Pode migrar do C para Brick AGORA

// ─── Frame limiter ───
extern fn engine_frame_limit(f64 fps) -> f64

// ─── Input ───
extern fn engine_key_held(i32 key) -> bool
extern fn engine_key_pressed(i32 key) -> bool
extern fn engine_mouse_pos(*f64 x, *f64 y)

// ─── Debug text ───
extern fn engine_dbg_print(u16 x, u16 y, u8 attr, *u8 msg)
extern fn engine_dbg_clear(u8 attr, bool small)

// ─── Window ───
extern fn engine_get_time() -> f64
extern fn engine_is_open() -> bool
extern fn engine_set_debug_stats(bool on)

// ─── Tudo isso já pode ser chamado de Brick!
// O que falta: arrays locais + narrowing + hex para a parte de geometria/mat4
```

---

## Apêndice B: Impacto no Projeto Atual

Com as features **Fase 1 + Fase 2** implementadas, o código movido de C para Brick:

| Arquivo | Status hoje | Com Features F1+F2 |
|---------|-------------|-------------------|
| `src/engine.c` | ~800 linhas C | ~400 linhas C (só bgfx/GLFW calls) |
| `src/engine.h` | ~50 funções | ~30 funções (só as que usam bgfx internamente) |
| `src/main.brc` | ~90 linhas Brick | ~200 linhas Brick (com mat4, camera, scene setup) |
| `src/scene_*.c` (legado) | 7 arquivos C, deletados | 7 arquivos `.brc` com `interface Scene` |

Relação final C:Brick passaria de **70% C / 30% Brick** para
**30% C / 70% Brick** — quase toda a lógica do jogo em Brick, C só para
inicialização bgfx/GLFW e chamadas de API de baixo nível.
