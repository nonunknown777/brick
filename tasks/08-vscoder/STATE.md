# Estado Atual
# Current State

Sessão: 2
Session: 2

Progresso: 95%
Progress: 95%

Próximo passo: Debug webview memory view com dados reais do GDB; completar suporte a if sem parênteses no parser principal
Next step: Debug webview memory view with real GDB data; complete support for if without parentheses in the main parser

Última ação: Fast in-process scanner (languageService.ts) para respostas instantâneas sem chamar compilador; context-aware completions (após . → fields/methods, após @ → blocks, após keyword → contextuais); signature help; semantic tokens; document symbols; debounce de 800ms na chamada do compilador real; sync incremental (change:2); onEnterRules para indentação; wordPattern; grammar aprimorada com @annotation e block-decl patterns
Last action: Fast in-process scanner (languageService.ts) for instant responses without calling the compiler; context-aware completions (after . → fields/methods, after @ → blocks, after keyword → contextual); signature help; semantic tokens; document symbols; 800ms debounce on real compiler call; incremental sync (change:2); onEnterRules for indentation; wordPattern; enhanced grammar with @annotation and block-decl patterns

Pendências: Debug webview precisa de dados reais do GDB (vs demo data); parser principal precisa atualizar para if sem parênteses e @
Pending: Debug webview needs real GDB data (vs demo data); main parser needs to update for if without parentheses and @
