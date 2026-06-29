# Task 10 - Tester / Optimizer / Doc - NEXT

## Pendências

### 0. Token string_view migration (✅ feito)
- ✅ `Token::lexeme` mudou de `std::string` para `std::string_view`

### 1. Aritmética de ponteiros + ++/-- + and/or keywords (✅ feito)
- ✅ `++`/`--` tokens, prefix e postfix (desugeram para `+=1`/`-=1`)
- ✅ `*` dereferência, `&` address-of como unários
- ✅ Aritmética de ponteiros (`ptr+N`, `ptr-N`, `ptr+=N`, `ptr-=N`)
- ✅ Comparação de ponteiros (`p==q`, `p<q`, `p!=null`)
- ✅ Indexação de ponteiro (`p[N]`)
- ✅ Keywords `and`/`or` como alias para `&&`/`||`
- ✅ 40 novos testes, 198/198 unitários passando
- ✅ 11/11 integração passando

### 2. Próximas features
- **Union types** (com anonymous union dentro de struct)
- **Bitfields** (`:` + largura em bits em campos de struct)
- **Function pointers** (`fn(params)->ret` como tipo)

### 3. Documentação pendente
- Adicionar comentários inline no código onde faltam
- Documentar runtime (block_memory.c, io.c)
- Documentar pointer arithmetic no shared-context.md

### 4. Otimizações
- String interning no parser/codegen (reutilizar strings de nome de tipo/variável)
- Compiler profiling (medir tempo de compilação)
- Generated code profiling (runtime performance)
- Inline asm em hot paths (bump alloc, pool alloc)

### 5. Testes adicionais
- Testes de C interop com mais bibliotecas
- Testes de type checker (mais casos edge)
- Benchmark suite (compile time + runtime)
- Testes de integração com aritmética de ponteiros (compilar .brc → executar)

### 6. Coordenação
- Verificar estado real tasks 08 e 09
- CI/CD setup (GitHub Actions)
- Atualizar shared-context.md com aritmética de ponteiros, ++/--, and/or
