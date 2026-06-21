# Estado Atual
# Current State

Sessão: 2 (revisão pela task 10)
Session: 2 (reviewed by task 10)

Progresso: 95%
Progress: 95%

Próximo passo: Integração com codegen — verificar exemplos .mc compilam
Next step: Integration with codegen — verify .mc examples compile

Última ação: if/while opcionalmente sem parênteses; reset keyword aceito como member name em DOT
Last action: if/while optionally without parentheses; reset keyword accepted as member name in DOT

Pendências: Nenhuma
Pending: None

## O que foi feito (task 10)
## What was done (task 10)

- `if_stmt()`: se próximo token não for `(`, usa expression direto como condição
- `while_stmt()`: mesma lógica
- `postfix()`: DOT handler aceita RESET como member name (não só IDENTIFIER)

- `if_stmt()`: if next token is not `(`, uses expression directly as condition
- `while_stmt()`: same logic
- `postfix()`: DOT handler accepts RESET as member name (not only IDENTIFIER)
