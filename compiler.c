#include "private.h"

#include <stdio.h>

void lux_compiler_init(compiler_t* comp, vm_t* vm, lexer_t* lex)
{
  comp->vm = vm;
  comp->lex = lex;
}

bool lux_compiler_compile_file(compiler_t* comp)
{
  token_t rettype;
  lux_lexer_get_token(comp->lex, &rettype);

  if(rettype.type != TT_NAME)
  {
    lux_vm_set_error(comp->vm, "Only function definitions can be at root level");
    return false;
  }

  token_t name;
  lux_lexer_get_token(comp->lex, &name);

  if(name.type != TT_NAME)
  {
    lux_vm_set_error_st(comp->vm, "Expected %s name, got '%s' instead", "function", &name);
    return false;
  }

  TRY(lux_lexer_expect_token(comp->lex, '('))
  TRY(lux_lexer_expect_token(comp->lex, ')'))

  TRY(lux_lexer_expect_token(comp->lex, '{'))
  TRY(lux_lexer_expect_token(comp->lex, '}'))

  return true;
}
