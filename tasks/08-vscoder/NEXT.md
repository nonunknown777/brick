# Próximo Passo - VSCoder
# Next Step - VSCoder

## ✅ Concluído — C Interop (include, link, extern fn, *T pointer)
## ✅ Completed — C Interop (include, link, extern fn, *T pointer)

### Syntax Highlighting ✅

Adicionados ao grammar `syntaxes/brick.tmLanguage.json`:
- `include|link|extern|and` como `keyword.control.package.brc`
- `*T` pointer type como `storage.modifier.pointer.brc`

### Language Service ✅

Atualizado `languageService.ts`:
- KEYWORDS + KEYWORD_DOCS com include, link, extern, and
- Scanner: tokenização EXTERN, INCLUDE, LINK, AND
- Símbolos: `extern fn NAME(...) -> Type` detectado como function
- Campos struct: `*u8`, `*void`, `*Player`, `*Player[10]`
- Variáveis: detecção de `*u8 ptr`, `*void handle`, `*Player ref`, `*Player[10] team`

### Server Completions ✅

Atualizado `server.ts`:
- `keywordSet` com include, link, extern, and
- `keywordCompletions` com snippets para include/link/extern
- Context-aware: extern → fn, include → "header.h", link → libname

### Snippets ✅

Adicionados `snippets/brick.code-snippets`:
- `extern` → extern fn declaration
- `include` → include "header.h"
- `link` → link libname
- `incandlink` → include + and + link combinado

### Tests ✅

Adicionados `test/scanner.test.ts`:
- C interop keywords (include, link, extern tokens)
- Extern fn detection (sqrt, puts, sin symbols)
- Include and link combined syntax
- Pointer type in struct fields (*u8, *void)
- Pointer type variable detection (*u8, *void, *Player)
- Pointer array types (*Player[10], *u8[256])

### Fixes ✅
- `.mc` → `.brc` em `.vscode/settings.json`
- `license: MIT` + `repository` URL em `package.json`

### Total: 169 tests, 0 failures ✅

## Pendências anteriores
## Previous pending

- Debug webview memory view com dados reais do GDB (já implementado, precisa validação em campo)
- Parser principal (task 02) precisa atualizar para if sem parênteses e @
- Publicar extensão no Marketplace VS Code
