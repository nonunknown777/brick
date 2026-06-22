<p align="center">
  <img src="docs/logo.png" alt="Meta-C Logo" width="200"/>
</p>

<h1 align="center">Meta-C</h1>
<p align="center">
  <em>Uma linguagem OOP de alta performance que compila para C puro.</em>
</p>

<p align="center">
  <a href="README.md">🇬🇧 English</a>
</p>

---

## 👋 Demonstração Rápida

```meta-c
package DEMO

using IO

interface Damageable {
    fn take_damage(i32 dmg)
}

struct Player {
    i32 hp
    String name
    i32 ammo

    fn Player(i32 h, String n, i32 a) {
        hp = h
        ammo = a
        name = n
    }

    fn take_damage(i32 dmg) {
        hp -= dmg
        print("{0} tomou {1} de dano, hp={2}", name, dmg, hp)
    }
}

fn main() {
    Player p = Player(100, "Felipe", 30)
    p.take_damage(20)
}
```

## Compilar & Executar

```bash
meta-c run exemplo.mc
```

Sem `gcc` manual — `meta-c build` e `meta-c run` cuidam de tudo.

---

## ✨ Funcionalidades

- **OOP com Chaves** — `struct` com construtores, métodos, herança e interfaces.
- **Compila para C Puro** — Código C legível com diretivas `#line` para debug. Sem VM, sem interpretador — código nativo.
- **Memória por Blocos** — Sem `malloc`/`free`, sem GC. Declare blocos (`block nome = 64MB`) e o bump allocator cuida do resto.
- **Sem Pilha para Dados** — Tudo vive em blocos gerenciados. Reset de bloco recupera tudo instantaneamente.
- **Hot Reload Nativo** — Troque código sem parar o programa via `dlopen` + `inotify`.
- **Visualizador TUI** — Dashboard ncurses mostrando estado dos blocos em tempo real.
- **Integração GDB** — Debug no código `.mc` original com pretty-printers para `BlockCtx`.
- **Extensão VS Code** — Syntax highlighting, LSP e webview de memória.
- **Tipos de Largura Fixa** — `i8/i16/i32/i64`, `u8/u16/u32/u64`, `f32/f64`, `usize`/`isize`.

---

## 🚀 Comece Agora

### Pré-requisitos

- Linux (ou Windows com mingw-w64)
- Compilador C++20 (GCC ≥ 11 ou Clang ≥ 14)
- SCons (`pip install scons`)
- ncurses (opcional, para o visualizador)

### Build do Compilador

```bash
git clone https://github.com/nonunknown777/meta-c.git
cd meta-c
scons                        # build release
# ou
./build-release.sh           # release completo + extensão VS Code
```

O binário `meta-c` estará em `build/`.

### Executar um Demo

```bash
# Compilar e executar em um passo
meta-c run examples/hello.mc

# Ou compilar para binário primeiro
meta-c build examples/hello.mc -o hello
./hello
```

### Rodar Testes

```bash
scons test                   # todos os testes unitários
```

### Visualizar Memória

```bash
meta-c --visualize examples/hello.mc   # compila, executa, mostra TUI
meta-c --attach <pid>                  # anexa a processo rodando
```

---

## ⚡ Performance

### Bump Allocator

| Operação              | Tempo                 | vs malloc/free           |
|-----------------------|-----------------------|--------------------------|
| Alocação              | ~3 ciclos de CPU      | ~50–200× mais rápido     |
| Reset de bloco (64MB) | ~5 ns                 | 2000× mais rápido        |

**Benchmark real:**

```
Block alloc: 1.000.000 allocs de 64B em 0.002s   ← 19,5× mais rápido
malloc:      1.000.000 allocs de 64B em 0.039s   ← baseline
```

### Compilador

| Entrada        | Tempo de Compilação |
|----------------|---------------------|
| 100 structs    | 5 ms                |
| 1.000 linhas   | ~10 ms              |

---

## 📝 Exemplos

### Tipos de Largura Fixa e Interface

```meta-c
package EXEMPLO

using IO

interface Desenhavel {
    fn desenhar()
}

struct Circulo extends Desenhavel {
    u32 id
    f32 raio

    fn Circulo(u32 i, f32 r) {
        id = i
        raio = r
    }

    fn desenhar() {
        print("Círculo #{0} raio={1}", id, raio)
    }
}

fn main() {
    Circulo c = Circulo(1u32, 5.0f32)
    c.desenhar()
}
```

### Hot Reload

```bash
# Compilar com suporte a hot reload
meta-c build jogo.mc --release -o jogo

# Executar — Meta-C monitora arquivos via inotify
# Edite seu .mc e salve — o binário recarrega automaticamente
./jogo
```

---

## 📁 Estrutura do Projeto

| Diretório      | Conteúdo                                               |
|----------------|--------------------------------------------------------|
| `src/`         | Compilador em C++20 (Lexer, Parser, Codegen)            |
| `runtime/`     | Runtime C (alocador de blocos, IO, hot reload)          |
| `visualizer/`  | TUI ncurses para visualização de memória                |
| `debugger/`    | Pretty-printers GDB, comandos custom, `.gdbinit`        |
| `examples/`    | Programas `.mc` de exemplo                              |
| `tests/`       | Testes unitários (SCons)                                |
| `benchmarks/`  | Scripts de benchmark                                    |
| `vscode-ext/`  | Extensão VS Code (highlight, LSP, memory view)          |
| `docs/`        | Site GitHub Pages (HTML + assets)                       |
| `wiki/`        | Fonte do GitHub Wiki                                    |
| `tasks/`       | 11 tarefas de desenvolvimento com estado por tarefa     |
| `build/`       | Artefatos de compilação                                 |

---

## 📚 Documentação

- **[GitHub Pages](https://nonunknown777.github.io/meta-c/)** — Site de documentação
- **[Wiki](https://github.com/nonunknown777/meta-c/wiki)** — Referência completa, tutoriais
- **[Spec da Linguagem](shared-context.md)** — Especificação completa

---

## 🤝 Contribuindo

O projeto é dividido em **11 tarefas**, cada uma com seu `AGENTS.md` e `STATE.md`.

Participe — abra uma issue ou PR em [github.com/nonunknown777/meta-c](https://github.com/nonunknown777/meta-c).

---

## 📄 Licença

MIT License.

---

<p align="center">
  <sub>Feito com ❤️ em C++20 e C — porque às vezes você precisa ser rápido.</sub>
</p>
