# Referência da Linguagem Brick (v1.1.0)

Referência completa da linguagem de programação Brick. Todas as features documentadas.

---

## 1. Estrutura do Arquivo

```brick
package NOME              ← pacote deste arquivo
using OUTRO               ← importar outro pacote

block global = 256MB       ← declaração de bloco (OBRIGATÓRIO no main)
block jogo   = 64MB

type MeuInt = int          ← alias de tipo

struct Jogador { }         ← structs com métodos
union Dados { }            ← unions
enum Cor { VERMELHO; VERDE } ← enums

const MAX_HP = 100          ← constante em tempo de compilação
const int TILE = 16         ← constante com tipo explícito

fn main() { }              ← ponto de entrada
```

---

## 2. Comentários

```brick
// Isto é um comentário de linha
int x = 5  // comentário inline
```

Apenas comentários de linha (`//`). Comentários de bloco (`/* */`) não são suportados.

---

## 3. Tipos

### 3.1 Tipos de Largura Fixa

| Brick | Tipo C | Tam. | Descrição |
|-------|--------|:----:|-----------|
| `i8` | `int8_t` | 8 | Inteiro com sinal |
| `i16` | `int16_t` | 16 | Inteiro com sinal |
| `i32` | `int32_t` | 32 | Inteiro com sinal (padrão `int`) |
| `i64` | `int64_t` | 64 | Inteiro com sinal |
| `u8` | `uint8_t` | 8 | Inteiro sem sinal (também `char`/`byte`) |
| `u16` | `uint16_t` | 16 | Inteiro sem sinal |
| `u32` | `uint32_t` | 32 | Inteiro sem sinal |
| `u64` | `uint64_t` | 64 | Inteiro sem sinal |
| `f32` | `float` | 32 | Ponto flutuante (padrão `float`) |
| `f64` | `double` | 64 | Dupla precisão |
| `usize` | `size_t` | ptr | Inteiro sem sinal do tamanho do ponteiro |
| `isize` | `ptrdiff_t` | ptr | Inteiro com sinal do tamanho do ponteiro |
| `bool` | `uint8_t` | 8 | Booleano (true/false) |
| `String` | `BrickString` | dyn | Texto dinâmico (alocado em bloco) |
| `void` | `void` | — | Nada (para funções) |

### 3.2 Aliases

| Alias | Mapa Para |
|-------|-----------|
| `int` | `i32` |
| `float` | `f32` |
| `char` | `u8` |
| `byte` | `u8` |
| `short` | `i16` |
| `long` | `i64` |
| `double` | `f64` |

### 3.3 Type Aliases Definidos pelo Usuário

```brick
type MeuInt = int
type Coord = f64
type Cor = u32
type Callback = fn(i32)->void
```

### 3.4 Bitfields

Os tipos `uN`/`iN` definem campos com largura específica de bits (1–64):

```brick
struct Flags {
    u4  nibble_baixo     // 4 bits sem sinal
    i3  signed_3bit      // 3 bits com sinal
    u1  bit_unico        // flag de 1 bit
    u24 parte_endereco   // endereço de 24 bits
    u8  byte_val         // 8 bits (equivalente a u8, mas bitfield explícito)
}
```

### 3.5 Sufixos Literais

```
42u8   42u16  42u32  42u64        ← tipos inteiros sem sinal
42i8   42i16  42i32  42i64        ← tipos inteiros com sinal
3.14f32  3.14f64                  ← tipos float
42usz  42isize                    ← tipos tamanho de ponteiro
```

Literais sem sufixo inferem tipo do contexto. Overflow → erro de compilação.

---

## 4. Literais

### Inteiro

```brick
42           ← sem tipo (inferido)
-128i8       ← sufixo de tipo
0x00FF       ← literal hex
0b1010       ← literal binário
0o777        ← literal octal
1_000_000    ← separador de underscore
```

### Float

```brick
3.14         ← inferido como f32 ou f64
3.14f32      ← float 32 bits
3.14159f64   ← float 64 bits
1.0e40       ← valor grande promovido a f64
```

### Hex

```brick
0xFF         ← 255
0xABCD       ← 43981
0x1A2Bu16    ← hex com sufixo de tipo
0x443355FF   ← cor RGBA
```

### Outros

```brick
true         ← literal bool
false        ← literal bool
'a'          ← literal char
"olá"        ← literal string
null         ← literal ponteiro nulo
```

---

## 5. Regras de Tipos

- **Widening** permitido: `i8` → `i16`, `u8` → `u64`, `f32` → `f64`
- **Narrowing** proibido: `i64` → `i32` = erro (use cast explícito: `i32(expr)`)
- **Com sinal ↔ Sem sinal** mesmo rank: proibido (`i32` ↔ `u32` = erro)
- **Int + Float** → Float (int promove a float)
- **Cast explícito**: `T(expr)` ou `expr as T` — permite narrowing

### Overflow Checking

```brick
u8 a = 300     // erro: 300 não cabe em u8
u8 b = 0x1FF   // erro: valor hex muito grande para o tipo
```

---

## 6. Variáveis

```brick
x = 5                ← inferido como int
int y = 10           ← tipo explícito
String nome = "Brick"
float pi = 3.14
i64 grande = 9223372036854775807
u8 pequeno = 255u8
f32 preciso = 3.14f32
```

Variáveis sem inicializador usam o nome do tipo:

```brick
int x               ← declarada mas não inicializada
String s            ← declarada
```

---

## 7. Constantes

```brick
const MAX_JOGADORES = 4            ← constante compile-time (tipo inferido)
const int TAM_TILE = 16            ← com tipo explícito
const LARGURA_TELA = 800           ← pode usar em tamanhos de array

const TAM_GRADE = 32
int[TAM_GRADE] buffer              ← constante usada como tamanho de array
```

Constantes são avaliadas em tempo de compilação. Valores são substituídos diretamente no código C gerado como `static const`.

---

## 8. Blocos de Memória

Blocos são regiões contíguas de memória com um bump allocator.

### Declaração

```brick
block global = 256MB       ← bloco padrão
block jogo   = 64MB
block temp   = 8KB
block dados  = 1GB
```

**Unidades**: `B`, `KB`, `MB`, `GB` (case-sensitive).

### Modos de Alocação

**1. Bloco padrão** — variáveis vão para `global`:

```brick
int x = 5                  ← em global
String s = "olá"           ← em global
```

**2. Escopo de bloco** — tudo dentro do escopo usa aquele bloco:

```brick
block jogo {
    Jogador p = Jogador(100, "Felipe")   ← ambos em 'jogo'
    Inimigo e = Inimigo(50)
}
```

**3. Anotação inline** — alocar em um bloco específico:

```brick
float f = 2.0 @temp        ← f vive em 'temp'
Jogador p = Jogador(100, "Felipe") @jogo
```

### Operações de Bloco

```brick
jogo.reset()               ← O(1) — libera TODA a memória em 'jogo'
global.reset()
```

- **Sem free individual** — apenas reset de blocos inteiros
- **Referências cruzadas** entre blocos são permitidas
- **Overflow** → `error("block overflow")` — programa aborta

### Pool Allocator

Tipos ≤ 64 bytes usam automaticamente um pool allocator (O(1) free):

```brick
// Particula = 16 bytes → pool_alloc() usado automaticamente
struct Particula { f32 x, y, z; i32 vida }
Particula p = Particula() @global
```

### TLS Blocks

Cada thread pode ter seu próprio bloco via `__thread`:

```c
block_set_tls(meu_bloco);  // do C
// Alocações vão para meu_bloco sem especificar
```

### Double-Buffer

Zero-pausa para hot reload:

```c
block_enable_double_buffer(cena);
block_swap_buffers(cena);   // swap atômico em ~1 ciclo
```

---

## 9. Structs (POO)

### Struct Básica

```brick
struct Jogador {
    int hp
    String nome
    int municao

    // Construtor — mesmo nome da struct
    fn Jogador(int h, String n, int m) {
        hp = h
        nome = n
        municao = m
    }

    // Método
    fn tomar_dano(int dmg) {
        hp -= dmg
    }

    // Método com valor de retorno
    fn get_hp() -> int {
        return hp
    }
}
```

Uso:

```brick
Jogador p = Jogador(100, "Felipe", 30) @jogo
p.tomar_dano(20)
int hp = p.get_hp()
```

### Herança

```brick
struct NPC extends Jogador {
    int tipo_ia

    fn NPC(int h, String n, int m, int ia) {
        hp = h          // herdado de Jogador
        nome = n
        municao = m
        tipo_ia = ia    // novo campo
    }

    fn patrulhar() {
        // comportamento específico de NPC
    }
}
```

C gerado: struct pai é o primeiro campo (`base`), campos herdados diretamente.

### Interfaces

```brick
interface Danificavel {
    fn tomar_dano(int d)
}

interface Serializavel {
    fn salvar() -> String
}

// Uma struct implementa múltiplas interfaces
struct Inimigo : Danificavel, Serializavel {
    fn tomar_dano(int d) { }
    fn salvar() -> String { return "Inimigo" }
}
```

### Bloco impl Separado

```brick
struct Flecha { int dano }

impl Flecha : Danificavel {
    fn tomar_dano(int d) {
        dano = d
    }
}
```

### Dispatch Virtual (vtbl)

Quando `impl Struct : Interface` existe, o compilador gera:
- Uma vtbl struct com ponteiros de função
- Uma struct wrapper com `void* data` e `const Vtbl* vtbl`
- Instâncias vtbl estáticas
- Funções wrapper que castam `void*` para o tipo concreto

```brick
interface Desenhavel { fn desenhar() }

struct Circulo : Desenhavel {
    fn desenhar() { print("Círculo") }
}

fn main() {
    Desenhavel d = Circulo() @global
    d.desenhar()  // despacha através de vtbl
}
```

### is / as — Type Checks de Interface

Em tempo de execução, verifique se um valor implementa uma interface com `is`, e faça cast com `as`:

```brick
interface Desenhavel { fn desenhar() }
struct Circulo : Desenhavel { fn desenhar() { print("Círculo") } }
struct Quadrado : Desenhavel { fn desenhar() { print("Quadrado") } }

fn main() {
    Desenhavel d = Circulo() @global

    // is — verifica se o tipo concreto é Circulo
    if d is Circulo {
        print("é um círculo!")
    }

    // as — cast para tipo concreto
    Circulo c = d as Circulo
    c.desenhar()
}
```

- `expr is Type` → `bool`
- `expr as Type` → `Type`

### Inicialização Nomeada de Struct

```brick
Jogador p = {hp = 100, nome = "Felipe", municao = 30}  ← nomeado
Jogador p2 = {100, "Felipe", 30}                        ← posicional
```

### @packed/@align

```brick
struct Compacto @packed { u8 a; i32 b }       // __attribute__((packed))
struct Alinhado @align(64) { u8 a; i32 b }    // __attribute__((aligned(64)))
struct Ambos @packed @align(16) { u8 x; i64 y }
```

### Sem `this`, Sem Shadowing

Nomes de campos são resolvidos diretamente dentro de métodos. Não é necessário `this->hp`. Shadowing (parâmetro ou local com mesmo nome de campo) não é permitido.

---

## 10. Unions

### Union Nomeada

```brick
union Dados {
    int i
    float f
    bool b
}

fn main() {
    Dados d
    d.i = 42     // define inteiro
    d.f = 3.14   // sobrescreve mesma memória como float
}
```

### Union Anônima dentro de Struct

```brick
struct Pacote {
    int id
    union {
        int x
        float y
    }             // x e y se sobrepõem na memória
}

fn main() {
    Pacote p
    p.id = 1
    p.x = 99     // acessa campo da union diretamente
}
```

### Struct Anônima dentro de Union

```brick
union Dados {
    u32 raw
    struct { u8 baixo; u8 alto }
}

fn main() {
    Dados d
    d.raw = 0x0A0B
    u8 b = d.baixo    // 0x0B
    u8 a = d.alto     // 0x0A
}
```

---

## 11. Enums

```brick
enum Cor {
    VERMELHO     // = 0 (auto-incremento)
    VERDE        // = 1
    AZUL         // = 2
}

enum FlagsTextura {
    CLAMP_U = 0x01    ← valor hex
    CLAMP_V = 0x02
    FILTRO  = 0x04
}

fn main() {
    Cor c = VERDE
    if c == VERDE { print("verde") }

    // Uso bitwise de enum
    u32 flags = CLAMP_U | CLAMP_V
    if flags & CLAMP_U { print("clampeado") }
}
```

Enums geram constantes `#define` em C. Variantes são constantes globais.

---

## 12. Funções

### Funções Básicas

```brick
fn main() { }                            ← ponto de entrada (retorna void)

fn somar(int a, int b) -> int {          ← retorna int
    return a + b
}

fn log(String msg) {                     ← função void
    // ...
}
```

### export fn — Visibilidade C

```brick
export fn calcular(int x) -> int {
    return x * 2
}
```

| Declaração | C Gerado |
|-----------|----------|
| `fn calc()` | `static inline int32_t calc()` |
| `export fn calc()` | `int32_t calc()` (visível ao linker) |

### Ponteiros de Função

```brick
fn somar(int a, int b) -> int { return a + b }

fn main() {
    fn(int, int)->int op    ← declara ponteiro de função
    op = somar              ← atribui função
    int r = op(3, 4)        ← chama através do ponteiro
    print(r)                // 7
}
```

Sintaxe: `fn(tipos_param)->tipo_retorno nome_var`

### Parâmetros Default

```brick
fn mover(int x, int y, int velocidade = 1) {
    // velocidade tem valor padrão 1
}

fn main() {
    mover(10, 20)        // usa velocidade = 1
    mover(10, 20, 5)     // usa velocidade = 5
}
```

---

## 13. Controle de Fluxo

### If/Else

```brick
if hp <= 0 { print("morto") }
else { print("vivo") }

if x > 0 { print("positivo") }
else if x < 0 { print("negativo") }
else { print("zero") }
```

### While

```brick
while hp > 0 { aplicar_dano(10) }
```

### For (C-Style)

```brick
for int i = 0; i < 10; i++ { print(i) }
```

### For x in N (Range)

```brick
fn somar_ate(int N) -> int {
    int total = 0
    for x in N { total = total + x }
    return total      // somar_ate(5) = 0+1+2+3+4 = 10
}
```

### Break / Continue

```brick
while true {
    if pronto { break }
    if pular { continue }
}
```

### Return

```brick
fn somar(int a, int b) -> int { return a + b }

fn log(String msg) {
    // return não necessário para void
}
```

---

## 14. Ponteiros e Aritmética

### Operações Básicas

```brick
*int p = &x             // &x = endereço de x
int v = *p              // *p = dereferência

// Aritmética (semântica C — escala por sizeof(T))
p = p + 1               // avança 1 elemento
p += 2
p -= 1

// Diferença de ponteiros
isize diff = q - p      // número de elementos entre endereços

// Indexação
int v = p[0]            // *(p + 0)

// Comparação
bool eq = p == q
bool lt = p < q
bool nulo = p != null

// Incremento / Decremento
++p                     // p += 1
p--                     // p -= 1
```

### Regras

- `ptr + int` / `ptr - int` → contagem de elementos (não bytes)
- `ptr - ptr` → `isize` (elementos entre)
- `*T + *T` é erro (apenas subtração)
- `*T + float` é erro (offset deve ser inteiro)
- `&literal` é erro (apenas variáveis têm endereço)
- `p[N]` funciona em qualquer `*T` como C

### Ponteiro Nulo

```brick
*int p = null
if p != null { }
```

---

## 15. Operadores

### Aritmética

| Operador | Descrição |
|:--------:|-----------|
| `+` | Adição |
| `-` | Subtração |
| `*` | Multiplicação |
| `/` | Divisão |
| `++` | Incremento |
| `--` | Decremento |

### Comparação

| Operador | Descrição |
|:--------:|-----------|
| `==` | Igual |
| `!=` | Diferente |
| `<` | Menor que |
| `>` | Maior que |
| `<=` | Menor ou igual |
| `>=` | Maior ou igual |

### Lógicos

| Operador | Descrição |
|:--------:|-----------|
| `&&` | AND lógico |
| `\|\|` | OR lógico |
| `!` | NOT lógico |
| `and` | AND lógico (keyword) |
| `or` | OR lógico (keyword) |
| `not` | NOT lógico (keyword) |

### Bitwise

| Operador | Descrição |
|:--------:|-----------|
| `&` | AND bitwise |
| `\|` | OR bitwise |
| `^` | XOR bitwise |
| `~` | NOT bitwise |
| `<<` | Deslocamento à esquerda |
| `>>` | Deslocamento à direita |

### Atribuição

| Operador | Descrição |
|:--------:|-----------|
| `=` | Atribuir |
| `+=` | Adicionar e atribuir |
| `-=` | Subtrair e atribuir |
| `*=` | Multiplicar e atribuir |
| `/=` | Dividir e atribuir |

### Outros

| Operador | Descrição |
|:--------:|-----------|
| `.` | Acessar campo ou método |
| `()` | Chamada de função/método |
| `[]` | Índice de array/ponteiro |
| `@` | Alocar em bloco específico |
| `->` | Tipo de retorno |

---

## 16. Strings

```brick
String s = "olá"                          ← cria uma String
String nome = "Felipe" @jogo              ← String em bloco específico
String vazia = ""                         ← string vazia
String saudacao = "Olá, " + "mundo!"     ← concatenação em tempo de compilação
```

- `String` é um tipo built-in com `.data` (ponteiro char) e `.len` (tamanho)
- Strings são alocadas em blocos
- Escape sequences: `\n` (nova linha), `\t` (tab), `\\` (barra invertida), `\"` (aspas)
- Quando passada para parâmetros C `*u8`, `.data` é passado automaticamente
- Concatenação usa `+` (apenas tempo de compilação para literais de string; concatenação em runtime não suportada ainda)

---

## 17. Arrays

### Arrays Fixos

```brick
int[10] arr                          ← array fixo de 10 inteiros
int[5] vals = {1, 2, 3, 4, 5}       ← com inicializador chaves
u8[4] bytes = {0xFF, 0x00, 0xAA, 0x55}
f32[4] m = {1.0, 0.0, 0.0, 1.0}
f32[4][4] matrix                    ← array 2D
```

Tamanho fixo na declaração (constante de compilação). Inicializador chaves define todos elementos.

### Arrays Dinâmicos (T[])

```brick
struct Container {
    int[] items     ← array dinâmico (ponteiro + contagem + capacidade)
}
```

`T[]` como campo de struct gera 3 campos C: `T* items; int64_t items_cnt; int64_t items_cap;`

Propriedades built-in:
- `.len` → contagem atual de elementos (`items_cnt`)
- `.cap` → capacidade alocada (`items_cap`)
- `.append(val)` → adiciona um elemento (auto-cresce)

```brick
struct Inventario {
    int[] items
}

fn main() {
    Inventario inv @global
    inv.items.append(10)
    inv.items.append(20)
    print(inv.items.len)     // 2
    print(inv.items.cap)     // 4 (auto-crescido)
}
```

### Literais de Array em Expressões

```brick
fn soma_3(*i32 a) -> i32 { return a[0] + a[1] + a[2] }

fn main() {
    i32 r = soma_3({10, 20, 30})   // literal compound C99
}
```

---

## 18. Match

```brick
match valor {
    1 { print("um") }
    2, 3 { print("dois ou três") }     ← multi-padrão
    _ { print("outro") }               ← wildcard (padrão)
}
```

### Match com Guards

```brick
fn test_guard() -> int {
    int val = 5
    int saida = 0
    match val {
        5 if val > 3 { saida = 1 }       ← condição guard
        5 { saida = 2 }                  ← fallback
        _ { saida = 3 }
    }
    return saida
}
```

Compila para `switch` C com `if` guards dentro dos cases.

---

## 19. Defer

```brick
fn main() {
    defer { print("limpeza") }
    print("fazendo trabalho")
    // "limpeza" é chamado ao sair do escopo
}
```

Corpos deferidos executam em ordem LIFO (último deferido, primeiro executado). Executam quando o escopo termina, inclusive antes de `return`.

```brick
fn test_multi() -> int {
    defer { print("primeiro defer") }
    defer { print("segundo defer") }
    return 42
    // Output: "segundo defer" depois "primeiro defer" depois retorna 42
}
```

---

## 20. Error Handling

```brick
error("algo deu errado")    ← imprime mensagem e aborta (panic)
```

Sem try/catch — falhe rápido com `error()`. Runtime usa `fprintf(stderr, ...); exit(1)`.

---

## 21. sizeof e alignof

```brick
i64 s = int.sizeof         // 4
s = f64.sizeof             // 8
s = MinhaStruct.sizeof     // soma dos campos + padding
s = minha_var.sizeof       // tamanho do tipo da variável

i64 a = f32.alignof        // 4
a = f64.alignof            // 8
a = MinhaStruct.alignof    // alinhamento da struct
```

Ambos são avaliados em tempo de compilação. Geram `sizeof(T)` / `_Alignof(T)` em C.

---

## 22. Visibilidade

```brick
public int x               ← visível em todo lugar (padrão)
private int y              ← visível apenas dentro do pacote
```

`public` é o padrão. `private` restringe visibilidade ao pacote atual.

Aplicável a: structs, funções, consts, enums, unions, interfaces, type aliases, macros e campos de struct.

---

## 23. I/O (Pacote IO)

```brick
using IO

fn main() {
    print(42)                    // "42\n"
    print(3.14)                  // "3.140000\n"
    print(true)                  // "true\n"
    print('a')                   // "a\n"
    print("olá")                 // "olá\n"
    print()                      // "\n"
    print("x = {0}", 10)         // "x = 10\n"
    print("{0} + {1} = {2}", 1, 2, 3)  // "1 + 2 = 3\n"
}
```

- `using IO` é obrigatório
- `print()` sempre adiciona `\n` (semântica println)
- Suportado: todos tipos numéricos, bool, char, String
- Formatação usa `{0}`, `{1}`, etc.

---

## 24. Macros

### Macro Básica

```brick
macro swap(a, b) {
    __tmp = $a
    $a = $b
    $b = __tmp
}

fn main() {
    x = 10; y = 20
    swap(x, y)
    print("{0} {1}", x, y) // "20 10"
}
```

### Sintaxe $macro() Explícita

```brick
$swap(x, y)   // $ explícito — equivalente a swap(x, y)
```

Ambas sintaxes funcionam.

### build {} — Computação em Tempo de Compilação

```brick
build {
    x = 42
    emit { z = x }        // gera: z = 42
}
```

`build` executa em tempo de compilação. Variáveis não existem no binário final.

### emit {} — Geração de Código

```brick
macro vec2_add(nome) {
    emit {
        fn $nome(x1, y1, x2, y2, saida_x, saida_y) {
            saida_x = x1 + x2
            saida_y = y1 + y2
        }
    }
}

vec2_add(add_posicoes)
```

### Higiene

Variáveis começando com `__` dentro de uma macro recebem nomes únicos (ex: `__tmp` → `__tmp__1`), prevenindo colisões com código do usuário.

### Varargs

```brick
macro print_all(valores...) {
    $valores[0]    // primeiro valor
    $valores[1]    // segundo valor
}
```

### Type Reflection (dentro de build)

| Expressão | Retorna | Exemplo |
|-----------|---------|---------|
| `T.name` | Nome do tipo como string | `"i32"` |
| `T.size` | Tamanho em bytes | `4` |
| `T.fields` | Nomes dos campos como strings | `["x", "y"]` |

---

## 25. Interoperabilidade com C

### Include de Headers C

```brick
include "meu_header.h"         // #include "meu_header.h"
include "stdio.h" @system      // #include <stdio.h>
include "math.h" and link m    // include + link juntos

// Ou separadamente:
include "SDL.h"
link SDL2
```

### Funções C Externas

```brick
extern fn sqrt(f64 x) -> f64
extern fn atoi(*u8 str) -> i32
extern fn puts(*u8 s) -> i32
extern fn sin(f64 x) -> f64
extern fn cos(f64 x) -> f64
extern fn pow(f64 b, f64 exp) -> f64
```

### Gerar Bindings C

```bash
brick bind <header.h>    # gera bindings .brc
```

---

## 26. Pacotes

### Declaração

```brick
package SPRITES                    ← declara pacote
package SPRITES.EFFECTS            ← sub-pacote hierárquico
```

### Import

```brick
using SPRITES                      ← importa tudo do pacote
using SPRITES.EFFECTS              ← importa sub-pacote aninhado
```

### export / private

```brick
export fn somar(int a, int b) -> int { return a + b }
export const PI = 31415
export struct Vec2 { int x; int y }
export enum Cor { VERMELHO; VERDE; AZUL }
export interface Desenhavel { fn desenhar() }

private fn ajuda_interna() -> int { return 999 }
private const SEGREDO = 42
```

---

## 27. Compilação

```bash
brick input.brc -o output.c           # compilar para C
brick build hello.brc -o hello        # compilar para binário (uma etapa)
brick run hello.brc                   # compilar e executar
brick new projeto                     # criar scaffold de projeto

gcc -O3 output.c runtime/block_memory.c runtime/io.c -o programa -ldl
```

## 28. Palavras Reservadas

```
package   using     public    private   struct
extends   interface fn        return    if
else      while     for       block     reset
true      false     null      error     int
float     bool      char      String    void
macro     build     emit      export    include
link      extern    const     enum      match
defer     union     type      impl      and
or        not       is        as
```
