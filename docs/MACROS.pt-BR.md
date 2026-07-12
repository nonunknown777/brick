# Sistema de Macros do Brick

Macros fornecem geração de código em tempo de compilação. São higiênicas, tipadas apenas na saída, e integram com `build {}` e `emit {}` para metaprogramação poderosa.

## Macro Básica

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

A variável `__tmp` dentro da macro recebe um **nome único** (`__tmp__1`) para prevenir colisões com variáveis do usuário (higiene).

## Definição

```brick
macro nome(param1, param2, ...) {
    // corpo — qualquer código Brick com interpolação $
}
```

- Parâmetros são **sem tipo** — contêm expressões (não valores avaliados)
- O corpo é analisado como código Brick normal
- `$nome` insere o argumento como está
- `__` no início de nome recebe gensym único

## build {} — Computação em Tempo de Compilação

Blocos `build {}` executam em tempo de compilação. Variáveis internas são temporárias e não existem no binário final.

```brick
build {
    x = 42
    emit { z = x }        // gera: z = 42
}
```

## emit {} — Geração de Código

`emit {}` gera código no escopo circundante:

```brick
macro vec2_op(nome, op) {
    emit {
        fn $nome(x1, y1, x2, y2, saida_x, saida_y) {
            $op(x1, x2, saida_x)
            $op(y1, y2, saida_y)
        }
    }
}
```

## Higiene (Gensym)

Variáveis começando com `__` dentro de macros recebem identificadores únicos:

```brick
macro duas_vezes(x) {
    __resultado = $x * 2     // __resultado → __resultado__1
}
```

## Varargs (args...)

```brick
macro print_all(valores...) {
    $valores[0]    // primeiro argumento
    $valores[1]    // segundo argumento
}
```

## Type Reflection (dentro de build)

| Expressão | Retorna | Exemplo |
|-----------|---------|---------|
| `T.name` | Nome do tipo como string | `"i32"` |
| `T.size` | Tamanho em bytes | `4` |
| `T.fields` | Nomes dos campos como array de strings | `["x", "y"]` |

## Regras

| Situação | Comportamento |
|----------|---------------|
| Contagem de argumentos errada | Erro de compilação |
| `$` fora de macro/build | Erro de compilação |
| Macro recursiva > 64 níveis | Erro de compilação |
| I/O dentro de `build` | Erro de compilação |

## Veja Também

- [Referência da Linguagem](LANGUAGE.pt-BR.md#24-macros) — Sintaxe de macros em detalhe
- [Arquitetura](ARCHITECTURE.pt-BR.md) — Como a expansão de macros se encaixa no pipeline
