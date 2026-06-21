# Próximo Passo - Parser
# Next Step - Parser

## Imediato
## Immediate

Adicionar suporte ao `using IO;` como package built-in.
Add support for `using IO;` as built-in package.

## Especificação
## Specification

O parser já suporta `using <ident>` (UsingDecl em parser.cpp:151-160).
Quando o usuário escreve `using IO;`, o parser cria um UsingDecl com
package_parts = {"IO"}. O TypeChecker/Codegen vão tratar o `IO`
como um package built-in especial que fornece a função `print()`.
The parser already supports `using <ident>` (UsingDecl in parser.cpp:151-160).
When the user writes `using IO;`, the parser creates a UsingDecl with
package_parts = {"IO"}. The TypeChecker/Codegen will treat `IO`
as a special built-in package that provides the `print()` function.

### Mudanças necessárias
### Required changes

**parser.cpp** — NENHUMA. O parser já parseia corretamente:
**parser.cpp** — NONE. The parser already parses correctly:

```meta-c
using IO;
```

Isso produz um `UsingDecl{package_parts: {"IO"}}`.
This produces a `UsingDecl{package_parts: {"IO"}}`.

A função `print()` é uma chamada de função normal:
The `print()` function is a normal function call:

```meta-c
print(10)
```

O parser já produz `CallExpr{callee: IdentExpr{"print"}, args: [IntLiteral{10}]}`.
The parser already produces `CallExpr{callee: IdentExpr{"print"}, args: [IntLiteral{10}]}`.

### Apenas validação
### Validation only

- `using IO;` é um identificador simples (sem DOTS), já parseado
- `print(expr)` é uma chamada de função normal (callee = "print")
- Formatação: `print("fmt {0}", x)` é só mais argumentos no CallExpr
- O parser NÃO precisa parsear chaves dentro de strings — isso é
  tratado no codegen

- `using IO;` is a simple identifier (no DOTS), already parsed
- `print(expr)` is a normal function call (callee = "print")
- Formatting: `print("fmt {0}", x)` is just more arguments in the CallExpr
- The parser does NOT need to parse braces inside strings — that is
  handled in codegen

## Pendências
## Pending

- Verificar se há conflito de nomes (ex: struct chamada "IO" ou "print")
- Se `using IO;` estiver presente, `print` deve ser reservado como
  função built-in (verificação no TypeChecker)

- Check for name conflicts (e.g. struct named "IO" or "print")
- If `using IO;` is present, `print` should be reserved as
  built-in function (check in TypeChecker)
