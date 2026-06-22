# Próximo Passo - Lexer
# Next Step - Lexer

## Feature: Integração com Parser
## Feature: Integration with Parser

O lexer está 100% funcional. O próximo passo é integrá-lo com o parser
(task 02).

### Verificar interface de tokens

O parser em `src/parser/` consome `std::vector<Token>` do lexer.
Garantir que:

1. `TokenType` em `types.h` cobre todos os tokens que o parser precisa
2. `Token::literal_type` é populado corretamente para literais com sufixo
3. `SourceLocation` (line, col, file) está correto

### Possíveis ajustes futuros

- Se o parser precisar de tokens adicionais, adicionar ao enum e ao
  switch no lexer
- Se o parser precisar de lookahead diferente, ajustar o comportamento
  de tokenização

## Contexto
## Context

- Lexer: 29 testes unitários passando
- Tipos de largura fixa implementados com sufixos de literal
- `literal_type` vazio para literais sem sufixo (inferência pelo type checker)
- Próximo time: task 02 Parser
