# Referência da Linguagem Brick

> Tudo que você precisa saber para escrever código em Brick.

## Estrutura de Arquivo (.brc)

```
package NAME              ← which package this file belongs to
using OTHER               ← import another package

block global = 256MB       ← block declaration (REQUIRED in main file)
block game   = 64MB

struct Player { }          ← your structs with methods

fn main() { }              ← program entry point
```

> Todo arquivo `.brc` começa com a declaração do pacote, imports opcionais, blocos de memória, structs e funções.

## Pacotes

```
package SPRITES                    ← declare package
package SPRITES.EFFECTS            ← sub-package (hierarchy)
using SPRITES                      ← import everything from package
private int secret                 ← visible only within the package
```

> Tudo é `public` por padrão. Use `private` para esconder.

## Structs (com métodos!)

```
struct Player {
    int hp                     ← field
    String name                ← field

    fn Player(int h, String n) {    ← CONSTRUCTOR (same name as struct)
        hp = h
        name = n
    }

    fn take_damage(int dmg) {       ← normal method
        hp -= dmg
    }

    fn get_hp() -> int {            ← method with return value
        return hp
    }
}
```

> Structs juntam dados e métodos. O construtor tem o mesmo nome da struct.

### Herança

```
struct NPC extends Player {
    int ai_type

    fn NPC(int h, int ai) {
        hp = h                    ← inherited field from Player
        ai_type = ai
    }
}
```

> Uma struct pode estender outra, herdando seus campos.

### Interface

```
interface Damageable {
    fn take_damage(int d)
}

struct Enemy : Damageable {       ← or "extends" + "implements"
    fn take_damage(int d) { }
}
```

> Interfaces definem contratos de métodos que structs devem implementar.

## Blocos de Memória

```
block global = 256MB        ← declare block (KB, MB, GB)
block temp   = 8KB

int x = 5                   ← goes to global block (default)

block game {                ← scope: everything here goes to block game
    Player p = Player(100, "Felipe")
    Enemy e = Enemy(50)
}

float f = 2.0 @temp        ← explicit inline allocation
int hp = p.get_hp()

game.reset()               ← clears EVERYTHING in block game (super fast)
```

> Blocos de memória são o coração do Brick. São regiões contíguas de memória. Alocar é só avançar um ponteiro — super rápido. Resetar um bloco limpa tudo de uma vez.

Regras:
- `global` é o bloco padrão (precisa ser declarado no main)
- `block nome: { }` muda o bloco padrão dentro daquele escopo
- `@nome` aloca uma variável específica em um bloco
- `bloco.reset()` limpa o bloco inteiro (sem free individual)
- Se o bloco encher: `error("block overflow")` — o programa aborta

## Tipos

### Tipos de Largura Fixa

| Brick | C Type       | Size    | Description          |
|--------|-------------|---------|----------------------|
| `i8`   | `int8_t`    | 8 bits  | Signed integer       |
| `i16`  | `int16_t`   | 16 bits | Signed integer       |
| `i32`  | `int32_t`   | 32 bits | Signed integer       |
| `i64`  | `int64_t`   | 64 bits | Signed integer       |
| `u8`   | `uint8_t`   | 8 bits  | Unsigned integer     |
| `u16`  | `uint16_t`  | 16 bits | Unsigned integer     |
| `u32`  | `uint32_t`  | 32 bits | Unsigned integer     |
| `u64`  | `uint64_t`  | 64 bits | Unsigned integer     |
| `f32`  | `float`     | 32 bits | Floating point       |
| `f64`  | `double`    | 64 bits | Double precision     |
| `usize`| `size_t`    | pointer | Unsigned pointer-size|
| `isize`| `ptrdiff_t` | pointer | Signed pointer-size  |

> Cada tipo numérico tem um tamanho fixo e previsível — sem adivinhação.

### Apelidos

| Alias    | Maps To | Notes                      |
|----------|---------|----------------------------|
| `int`    | `i32`   | Default integer            |
| `float`  | `f32`   | Default float              |
| `char`   | `u8`    | Character                  |
| `byte`   | `u8`    | Same as char               |
| `short`  | `i16`   | Short integer              |
| `long`   | `i64`   | Long integer               |
| `double` | `f64`   | Double precision           |

> Nomes amigáveis para os tipos mais comuns.

### Outros Tipos

| Brick     | C Type         | Description                  |
|------------|----------------|------------------------------|
| `bool`     | `uint8_t`      | true/false                   |
| `String`   | `BrickString`  | Built-in string (dynamic)    |
| `T[N]`     | `T[]`          | Fixed array of N elements    |
| `null`     | `NULL`         | Null pointer                 |
| `void`     | `void`         | No return value (functions)  |

### Sufixos Literais

```
42u8   42u16  42u32  42u64     ← unsigned integer types
42i8   42i16  42i32  42i64     ← signed integer types
3.14f32  3.14f64               ← float types
42usz  42isize                 ← pointer-size types
```

> Literais sem sufixo inferem o tipo pelo contexto (se couber no alvo -> permitido).
> Overflow em literal em tempo de compilação -> erro.

### Regras de Tipo

> - **Ampliação** permitida: i8 -> i16, u8 -> u64, f32 -> f64
> - **Estreitamento** proibido: i64 -> i32 = erro
> - **Sinalizado <-> Não sinalizado** mesmo tamanho proibido: i32 <-> u32 = erro
> - **Int + Float** -> Float (int vira float)
> - **Expressões mistas**: promoção para o tipo que cabe ambos operandos

## Funções

```
fn main() { }                          ← entry point (no return)

fn add(int a, int b) -> int {          ← function returning int
    return a + b
}

fn log(String msg) {                   ← void function (no return)
    // ...
}
```

> Parâmetros e valores de retorno vão para o "bloco anônimo" interno do compilador.

### `export fn` — Funções Visíveis para C

```
export fn calculate(int x) -> int {
    return x * 2
}
```

Funções declaradas com `export fn` NÃO recebem `static inline` no C gerado. Isso permite que código C (ou outras linguagens) linkem contra elas. Útil para bibliotecas compartilhadas.

| Declaração | C gerado |
|-----------|----------|
| `fn calc()` | `static inline int32_t calc()` |
| `export fn calc()` | `int32_t calc()` (visível ao linker) |

## Controle de Fluxo

```
if cond { }
else { }

while cond { }

for int i = 0; i < 10; i++ { }

return expr
```

> Controle de fluxo padrão: if/else, while, for e return.

## Operadores

| Operador       | O que faz                 |
|----------------|---------------------------|
| `+ - * /`      | Matemática básica         |
| `== != < > <= >=` | Comparação            |
| `&& \|\| !`    | Lógica (e, ou, não)       |
| `= `           | Atribuição                |
| `.`            | Acessar campo/método      |
| `()`           | Chamar função             |
| `[]`           | Índice de array           |
| `@`            | Alocar em bloco específico|
| `->`           | Tipo de retorno           |
| `& \| ^ ~ << >>` | Operações binárias    |

## Strings

```
String s = "hello"                     ← creates string
String nome = "Felipe" @game           ← string in a block
String empty = ""                      ← empty string
```

> String é um tipo nativo. Vive em blocos como qualquer outra struct.

## Arrays

```
int[10] arr                            ← fixed array of 10 integers
int[5] vals = int[5] @game             ← array in a block
```

> Tamanho fixo definido na declaração.

## Tratamento de Erros

```
error("something went wrong")          ← prints message and aborts
```

> Sem try/catch. Se algo der errado, o programa imprime o erro e sai.

## Visibilidade

```
public int x                           ← visible everywhere (default)
private int y                          ← visible only within own package
```

> `public` é o padrão. `private` restringe a visibilidade ao pacote.

## Comentários

```
// This is a comment  ← line comment only
```

> Apenas comentários de linha (`//`). Comentários de bloco (`/* */`) não são suportados.

## Macros

### `$macro()` — Chamada Explícita de Macro

```
$twice(10)   // expande o macro
```

Macros podem ser chamados com `$nome(args)` para clareza visual. Ambas as sintaxes funcionam:

| Sintaxe | Exemplo |
|---------|---------|
| Implícita | `twice(10)` |
| Explícita | `$twice(10)` |

O `$` deixa explícito que é uma chamada de macro (não de função), útil em código complexo.

Macros geram código em tempo de compilação. Você define um padrão uma vez e o compilador replica onde for chamado.

```
macro troca(a, b) {
    __tmp = $a
    $a = $b
    $b = __tmp
}

fn main() {
    x = 10; y = 20
    troca(x, y)            // expande pro corpo do macro
    print("{0} {1}", x, y) // → "20 10"
}
```

### Declaração

```
macro nome(param1, param2, ...) { corpo }
```

- Parâmetros **não tem tipo** — contêm expressões, não valores
- `$nome` insere o argumento passado para `nome`
- `$(expr)` avalia `expr` em tempo de compilação e insere o resultado
- Nomes começando com `__` ganham gensyms únicos (sem colisão)

### `build {}` — Computação em Tempo de Compilação

```
build {
    x = 42
    emit { z = x }        // gera: z = 42
}
```

`build` roda em tempo de compilação. Variáveis dentro de `build` não existem no binário final. Use `emit` para gerar código de verdade.

### `emit {}` — Geração de Código

Tudo dentro de `emit` é código literal com interpolação `$`. O conteúdo é cravado na saída onde o macro for chamado.

```
macro criar_getter(nome, campo) {
    emit {
        fn get_$nome() { return $campo }
    }
}
```

### Higiene

Variáveis que começam com `__` dentro de um macro ganham nomes únicos (ex: `__tmp` → `__tmp__1`), evitando colisão com código do usuário.

### Reflexão de Tipo (dentro de `build`)

| Expressão | Retorna | Exemplo |
|-----------|---------|---------|
| `T.name` | Nome do tipo como string | `"i32"` |
| `T.size` | Tamanho em bytes | `4` |
| `T.fields` | Nomes dos campos | `["x", "y"]` |

### Tratamento de Erros

| Situação | O que acontece |
|----------|---------------|
| Argumentos errados | Erro de compilação |
| `$` fora de macro/build | Erro: "unexpected $" |
| Macro recursivo | Pego após 64 níveis |
| I/O dentro de `build` | `build` não pode chamar print() |

Veja [Macros](MACROS.pt-BR.md) para documentação completa com exemplos.

## Exemplo Completo

```
package GAME

using IO

block global = 256MB
block game   = 64MB

interface Drawable {
    fn draw()
}

struct Player {
    i32 hp
    String name
    u8 active

    fn Player(i32 h, String n) {
        hp = h
        name = n
        active = 1u8
    }

    fn damage(i32 dmg) {
        hp -= dmg
        if hp < 0 { hp = 0 }
        print("{0} took {1} damage, hp={2}", name, dmg, hp)
    }
}

fn main() {
    Player p = Player(100, "Felipe") @game
    p.damage(20)
    game.reset()
}
```

> Este exemplo mostra um programa Brick completo com pacote, blocos, interface, struct e função main.
