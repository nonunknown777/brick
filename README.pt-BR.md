# Brick

**v1.1.0** — Uma linguagem de programação que compila para C. Performance máxima,
gerenciamento explícito de memória por blocos, e hot reload nativo.

```brick
package GAME
using IO
block global = 64MB

interface Damageable { fn take_damage(i32 dmg) }

struct Player : Damageable {
    i32 hp
    String name
    int[] items

    fn Player(i32 h, String n) {
        hp = h; name = n
    }

    fn take_damage(i32 dmg) {
        hp -= dmg
        if hp < 0 { hp = 0 }
        print("{0} tomou {1} de dano, hp={2}", name, dmg, hp)
    }
}

fn main() {
    Player p = Player(100, "Felipe") @game
    p.take_damage(20)
    game.reset()
}
```

## Início Rápido

```bash
git clone <repo> && cd brick
scons                          # compilar compilador
brick run exemplos/hello.brc   # compilar e executar
brick build hello.brc -o hello # compilar para binário
```

## Features

| Categoria | Features |
|-----------|----------|
| **Tipos** | Largura fixa: `u8`..`u64`, `i8`..`i64`, `f32`/`f64`, `usize`/`isize`, `byte`, `bool`, `String`. Aliases: `short`/`long`/`double`, `type NAME = TYPE`. Bitfields: `u4`, `i3` |
| **Literais** | Hex (`0xFF`), binário (`0b1010`), octal (`0o777`), separador (`1_000_000`), sufixos de tipo (`42u8`, `3.14f64`) |
| **Arrays** | Fixo `int[10]`, dinâmico `int[]` com `.append`/`.len`/`.cap`, literais `{1, 2, 3}` |
| **Structs** | Métodos, construtores, herança (`extends`), interfaces (`interface`), dispatch vtbl, blocos `impl`, aninhamento anônimo struct/union, `@packed`/`@align(N)`, init nomeado |
| **Unions** | Unions nomeadas, unions anônimas dentro de structs, structs anônimas dentro de unions |
| **Enums** | Constantes nomeadas com valores hex (`enum Flags { A = 0xFF }`) |
| **Ponteiros** | `*T`, aritmética completa (`+`, `-`, `+=`, `-=`, `++`, `--`, `[]`, `==`, `!=`, `<`, `>`), null |
| **Controle** | `if`/`else`, `while`, `for` (C-style + `for x in N`), `break`/`continue`, `match` com guards, `defer` |
| **Constantes** | `const NAME = value` com avaliação em tempo de compilação, usado em tamanhos de array |
| **Funções** | `fn`, `->` tipo retorno, `export fn` visibilidade C, ponteiros de função `fn(int)->void`, parâmetros default |
| **Macros** | `macro` com interpolação `$`, `build {}` eval, `emit {}` code gen, varargs `args...`, higiene, `$macro()` explícito |
| **Pacotes** | `package NAME`, `using PACKAGE`, `export`/`private`, multi-arquivo, aninhados (`MATH.VEC2`), auto-resolução, `-I`, `BRICK_PATH` |
| **Interop C** | `include`, `link`, `extern fn`, `@system`, `export fn`, `*u8` → `char*`, `String` → `*u8` |
| **Operadores** | `and`/`or`/`not`, `&&`/`||`/`!`, bitwise (`&` `|` `^` `~` `<<` `>>`), `sizeof`/`alignof`, `++`/`--` |
| **Memória** | Bump allocator por blocos (~3 ciclos), pool allocator para tipos ≤64B, TLS blocks, double-buffer hot reload |
| **Visibilidade** | `private`/`public` em campos, funções, structs, consts, enums, unions, interfaces, types, macros |
| **Erros** | `error("msg")` panic com mensagem e abort |

## CLI

```bash
brick <input.brc> [-o output.c]           # compilar para C
brick build <files...> [-o output]         # compilar para binário
brick run <input.brc>                      # compilar e executar
brick new <projeto>                        # criar novo projeto scaffold
brick bind <header.h>                      # gerar bindings C
brick --visualize <file>                   # compilar + visualizador TUI
brick --attach <pid>                       # anexar visualizador a processo
```

## Documentação

- [Guia de Início Rápido](docs/GETTING_STARTED.pt-BR.md) — Instalação, build, primeiro programa
- [Referência da Linguagem](docs/LANGUAGE.pt-BR.md) — Sintaxe completa, tipos, pacotes, memória
- [Arquitetura](docs/ARCHITECTURE.pt-BR.md) — Pipeline do compilador (Lexer → Parser → Macros → Codegen)
- [Macros](docs/MACROS.pt-BR.md) — Geração de código em tempo de compilação
- [Hot Reload](docs/hot-reload.pt-BR.md) — Troca de código ao vivo
- [Otimizações](docs/OPTIMIZATIONS.pt-BR.md) — Detalhes de performance

## Licença

MIT
