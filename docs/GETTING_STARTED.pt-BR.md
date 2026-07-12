# Guia de Início Rápido do Brick

## Pré-requisitos

- **Linux** ou **Windows** com MinGW-w64
- **Compilador C++**: gcc ≥ 9 ou clang ≥ 12
- **Python 3** e **pip**
- **SCons** (`pip install scons`)
- **ncurses** (Linux): `sudo apt install libncurses-dev` (opcional, para o visualizador)

## Compilar o Compilador

```bash
git clone <repo>
cd brick
scons                    # compila o compilador 'brick' em build/
```

### Opções de Build

```bash
scons profile=debug      # build debug (sem otimização, amigável ao GDB)
scons visualizer=no      # pular visualizador TUI
scons vsix=yes           # compilar extensão VS Code (.vsix)
```

## Hello World

Crie `hello.brc`:

```brick
using IO

block global = 64MB

fn main() {
    print("Olá, Brick!")
}
```

Compile e execute:

```bash
brick run hello.brc
# Output: Olá, Brick!
```

Ou passo a passo:

```bash
brick hello.brc -o hello.c    # compilar Brick → C
gcc -O3 hello.c runtime/block_memory.c runtime/io.c -o hello
./hello                        # Olá, Brick!
```

## Primeiro Programa Real

```brick
package JOGO
using IO

block global = 256MB
block jogo = 64MB

interface Danificavel {
    fn tomar_dano(int dmg)
}

struct Inimigo : Danificavel {
    int hp
    String nome

    fn Inimigo(int h, String n) {
        hp = h; nome = n
    }

    fn tomar_dano(int dmg) {
        hp -= dmg
        if hp <= 0 { print("{0} destruído!", nome) }
    }
}

fn main() {
    Inimigo e = Inimigo(100, "Goblin") @jogo
    e.tomar_dano(30)
    e.tomar_dano(80)
    jogo.reset()
}
```

```bash
brick build jogo.brc -o jogo
./jogo
```

## Projeto Multi-Arquivo

```
projeto/
├── main.brc
├── lib/
│   └── MATH.brc
```

`lib/MATH.brc`:

```brick
package MATH

export fn somar(int a, int b) -> int {
    return a + b
}

export const PI = 31415
```

`main.brc`:

```brick
package GAME
using IO
using MATH

block global = 64MB

fn main() {
    int r = somar(3, 4)
    print("{0}", r)
}
```

Build:

```bash
brick build main.brc -I lib -o programa
./programa
```

## Extensão VS Code

```bash
cd vscode-ext
npm install
npm run compile
# Pressione F5 para abrir Extension Development Host
```

Features: syntax highlighting, LSP (completions, hover, go-to-def, signature help, semantic tokens), memory webview.

## Próximos Passos

- [Referência da Linguagem](LANGUAGE.pt-BR.md) — Sintaxe completa
- [Arquitetura](ARCHITECTURE.pt-BR.md) — Como o compilador funciona
- [Macros](MACROS.pt-BR.md) — Geração de código em tempo de compilação
- [Hot Reload](hot-reload.pt-BR.md) — Troca de código ao vivo
