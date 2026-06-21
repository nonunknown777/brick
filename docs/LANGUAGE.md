# Referência da Linguagem Meta-C
# Meta-C Language Reference

> Tudo que você precisa saber pra escrever código em Meta-C.
> Everything you need to know to write code in Meta-C.

## Estrutura de um arquivo .mc
## File Structure (.mc)

```
package NOME               ← qual pacote esse arquivo pertence
using OUTRO                ← importa outro pacote

block global = 256MB       ← declaração OBRIGATÓRIA no main
block game = 64MB

struct Player { }          ← suas estruturas com métodos

fn main() { }              ← ponto de entrada do programa
```

## Packages
## Packages

```
package SPRITES                 ← declara pacote
package SPRITES.EFFECTS         ← sub-pacote (hierarquia)
using SPRITES                   ← importa tudo do pacote
private int segredo             ← visível só dentro do pacote
```

Tudo é `public` por padrão. Use `private` pra esconder.
Everything is `public` by default. Use `private` to hide.

## Structs (com métodos!)
## Structs (with methods!)

```
struct Player {
    int hp                     ← campo
    String name                ← campo

    fn Player(int h, String n) {    ← CONSTRUTOR (mesmo nome da struct)
        hp = h
        name = n
    }

    fn take_damage(int dmg) {       ← método normal
        hp -= dmg
    }

    fn get_hp() -> int {            ← método com retorno
        return hp
    }
}
```

### Herança
### Inheritance

```
struct NPC extends Player {
    int ai_type

    fn NPC(int h, int ai) {
        hp = h                    ← campo herdado de Player
        ai_type = ai
    }
}
```

### Interface
### Interface

```
interface Damageable {
    fn take_damage(int d)
}

struct Enemy : Damageable {       ← ou "extends" + "implements"
    fn take_damage(int d) { }
}
```

## Blocos de Memória
## Memory Blocks

```
block global = 256MB        ← declara bloco (KB, MB, GB)
block temp = 8KB

int x = 5                   ← vai pro bloco global (default)

block game {                ← escopo: tudo aqui vai pro bloco game
    Player p = Player(100, "Felipe")
    Enemy e = Enemy(50)
}

float f = 2.0 @temp        ← alocação inline explícita
int hp = p.get_hp()

game.reset()               ← limpa TUDO no bloco game (super rápido)
```

Regras:
Rules:

- `global` é o bloco default (declarado obrigatoriamente no main)
- `block nome: { }` muda o bloco default dentro do escopo
- `@nome` aloca uma variável específica num bloco
- `bloco.reset()` limpa o bloco inteiro (sem free individual)
- Se o bloco encher: `error("block overflow")` — o programa aborta

- `global` is the default block (must be declared in main)
- `block name: { }` changes the default block within that scope
- `@name` allocates a specific variable in a block
- `block.reset()` clears the entire block (no individual free)
- If the block fills up: `error("block overflow")` — the program aborts

## Tipos
## Types

| Meta-C | O que é | Tamanho |
| Meta-C | What it is | Size |
|--------|---------|:-------:|
| `int` | Número inteiro | 64 bits |
| `int` | Integer number | 64 bits |
| `float` | Número decimal | 64 bits |
| `float` | Decimal number | 64 bits |
| `bool` | Verdadeiro/falso | 8 bits |
| `bool` | True/false | 8 bits |
| `char` | Um caractere | 8 bits |
| `char` | A single character | 8 bits |
| `String` | Texto | dinâmico |
| `String` | Text | dynamic |
| `int[N]` | Array fixo de N inteiros | N × 64 bits |
| `int[N]` | Fixed array of N integers | N × 64 bits |
| `null` | Nulo (pra referências) | — |
| `null` | Null (for references) | — |
| `void` | Nada (pra funções sem retorno) | — |
| `void` | Nothing (for functions without return) | — |

## Funções
## Functions

```
fn main() { }                          ← entry point (não volta nada)

fn add(int a, int b) -> int {          ← função que volta int
    return a + b
}

fn log(String msg) {                   ← função sem retorno (void)
    // ...
}
```

Parâmetros e retorno vão pro "bloco anônimo" interno do compilador.
Você não precisa se preocupar com eles.
Parameters and return values go to the compiler's internal "anonymous block".
You don't need to worry about them.

## Controle de Fluxo
## Flow Control

```
if (cond) { }
else { }

while (cond) { }

for (int i = 0; i < 10; i++) { }

return expr
```

## Operadores
## Operators

| Operador | O que faz |
| Operator | What it does |
|:--------:|-----------|
| `+ - * /` | Matemática básica |
| `+ - * /` | Basic math |
| `== != < > <= >=` | Comparação |
| `== != < > <= >=` | Comparison |
| `&& \|\| !` | Lógica (e, ou, não) |
| `&& \|\| !` | Logic (and, or, not) |
| `= ` | Atribuição |
| `= ` | Assignment |
| `.` | Acessar campo/método |
| `.` | Access field/method |
| `()` | Chamar função |
| `()` | Function call |
| `[]` | Indexar array |
| `[]` | Array index |
| `@` | Alocar em bloco específico |
| `@` | Allocate in specific block |
| `->` | Tipo de retorno |
| `->` | Return type |
| `& \| ^ ~ << >>` | Operações de bit |
| `& \| ^ ~ << >>` | Bitwise operations |

## Strings
## Strings

```
String s = "hello"                     ← cria string
String nome = "Felipe" @game           ← string num bloco
String vazia = ""                      ← string vazia
```

String é um tipo embutido. Vive em blocos igual qualquer outra struct.
String is a built-in type. It lives in blocks like any other struct.

## Arrays
## Arrays

```
int[10] arr                            ← array fixo de 10 inteiros
int[5] vals = int[5] @game             ← array num bloco
```

Tamanho fixo definido na declaração.
Fixed size defined at declaration.

## Tratamento de Erros
## Error Handling

```
error("deu ruim")                      ← imprime a mensagem e aborta
```

Não tem try/catch. Se algo der errado, o programa morre com a mensagem.
Isso mantém a performance alta e o código simples.
There is no try/catch. If something goes wrong, the program dies with the message.
This keeps performance high and code simple.

## Visibilidade
## Visibility

```
public int x                           ← visível pra todo mundo (padrão)
private int y                          ← visível só dentro do próprio package
```

## Comentários
## Comments

```
// Isso é um comentário  ← até o fim da linha
```

Só comentário de linha (`//`). Bloco de comentário (`/* */`) não existe.
Only line comments (`//`). Block comments (`/* */`) do not exist.

## Exemplo Completo
## Complete Example

```
package JOGO

using SPRITES

block global = 256MB
block game = 64MB

struct Player {
    int hp
    int ammo
    String name

    fn Player(int h, int a, String n) {
        hp = h
        ammo = a
        name = n
    }

    fn shoot(Enemy e) {
        e.hp -= 10
    }
}

struct Enemy {
    int hp
    int damage

    fn Enemy(int h, int d) {
        hp = h
        damage = d
    }
}

fn main() {
    Player p = Player(100, 30, "Felipe") @game
    Enemy e = Enemy(50, 10) @game

    p.shoot(e)

    while (e.hp > 0) {
        p.shoot(e)
    }

    game.reset()
    global.reset()
}
```
