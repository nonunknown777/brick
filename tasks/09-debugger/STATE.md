# Task 09 - Debugger - STATE

## Status: 🔶 PARCIAL

Debugger GDB pretty-printers e .gdbinit existem.

## Implementado
- #line directives no código C gerado (codegen)
- GDB mostra código-fonte Brick original
- GDB pretty-printers para BlockCtx (info blocks, block name)
- Comandos GDB custom: info blocks, block <name>, block-watch (provavel)

## Arquivos
- `debugger/` - GDB .gdbinit e pretty-printers
- `#line` generation em codegen.cpp

## Observações
- Estado exato precisa ser verificado (task 09)
- Codegen já gera #line directives — debug mapping funcional
