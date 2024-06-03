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

  if(lux_vm_get_type_t(comp->vm, &rettype) == NULL)
  {
    lux_vm_set_error_t(comp->vm, "Unknown return type: '%s'", &rettype);
    return false;
  }

  token_t name;
  lux_lexer_get_token(comp->lex, &name);

  if(name.type != TT_NAME)
  {
    lux_vm_set_error_t(comp->vm, "Expected function name, got '%s' instead", &name);
    return false;
  }

  if(lux_vm_get_type_t(comp->vm, &name) != NULL || lux_lexer_is_reserved(&name))
  {
    lux_vm_set_error(comp->vm, "Function name cannot be a type or a reserved word");
    return false;
  }

  TRY(lux_lexer_expect_token(comp->lex, '('))
  TRY(lux_lexer_expect_token(comp->lex, ')'))

  TRY(lux_lexer_expect_token(comp->lex, '{'))
  TRY(lux_lexer_expect_token(comp->lex, '}'))

  return true;
}
