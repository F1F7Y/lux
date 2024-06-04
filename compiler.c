#include "private.h"

#include <stdio.h>

void lux_compiler_init(compiler_t* comp, vm_t* vm, lexer_t* lex)
{
  comp->vm = vm;
  comp->lex = lex;
}

static bool lux_compiler_scope(compiler_t* comp, closure_t* closure)
{
  TRY(lux_lexer_expect_token(comp->lex, '{'))
  while(true)
  {
    token_t token;
    lux_lexer_get_token(comp->lex, &token);

    if(token.type == TT_TOKEN)
    {
      switch(*token.buf)
      {
        case '}':
        {
          return true;
        }
        break;
      }
    }
    else if(token.type == TT_NAME)
    {
      if(lux_token_is_str(&token, "return"))
      {
        if(closure->rettype->can_be_variable)
        {
          token_t retvalue;
          lux_lexer_get_token(comp->lex, &retvalue);
          if(retvalue.type != TT_INT)
          {
            lux_vm_set_error(comp->vm, "Expected return value");
            return false;
          }
          lux_vm_closure_append_byte(closure, OP_LDI);
          lux_vm_closure_append_byte(closure, 0);
          lux_vm_closure_append_int(closure, retvalue.ivalue);
        }
        lux_vm_closure_append_byte(closure, OP_RET);
      }
    }
  }

  assert(false);
  lux_vm_set_error(comp->vm, "FATAL: Unreachable compiler code");
  return false;
}

bool lux_compiler_compile_file(compiler_t* comp)
{
  token_t dummy;
  while(lux_lexer_get_token(comp->lex, &dummy) != TT_EOF)
  {
    lux_lexer_unget_last_token(comp->lex);
    token_t rettype;
    lux_lexer_get_token(comp->lex, &rettype);

    if(rettype.type != TT_NAME)
    {
      lux_vm_set_error(comp->vm, "Only function definitions can be at root level");
      return false;
    }
    
    vmtype_t* t = lux_vm_get_type_t(comp->vm, &rettype); 
    if(t == NULL)
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

    closure_t* closure = lux_vm_register_function_t(comp->vm, &name, t);
    TRY(closure);

    TRY(lux_lexer_expect_token(comp->lex, '('))
    bool read_function_args = !lux_lexer_expect_token(comp->lex, ')');
    if(read_function_args)
    {
      lux_lexer_unget_last_token(comp->lex);
    }
    while(read_function_args)
    {
      token_t type;
      lux_lexer_get_token(comp->lex, &type);

      if(lux_lexer_expect_token(comp->lex, ')'))
      {
        lux_vm_set_error(comp->vm, "Function argument missing name");
        return false;
      }
      
      vmtype_t* vt = lux_vm_get_type_t(comp->vm, &type);
      if(!vt)
      {
        lux_vm_set_error(comp->vm, "Function argument has unknown type");
        return false;
      }

      if(!vt->can_be_variable)
      {
        lux_vm_set_error_s(comp->vm, "Type '%s' cannot be used as a function argument", vt->name);
        return false;
      }

      lux_lexer_unget_last_token(comp->lex);
      token_t name;
      lux_lexer_get_token(comp->lex, &name);

      if(lux_lexer_expect_token(comp->lex, ')'))
      {
        break;
      }
      lux_lexer_unget_last_token(comp->lex);
      TRY(lux_lexer_expect_token(comp->lex, ','));
      if(lux_lexer_expect_token(comp->lex, ')'))
      {
        lux_vm_set_error(comp->vm, "Function argument missing after ,");
        return false;
      }
      lux_lexer_unget_last_token(comp->lex);
    }

    TRY(lux_compiler_scope(comp, closure));
#if 0
    if(!lux_vm_closure_last_byte_is(closure, OP_RET))
    {
      if(closure->rettype->can_be_variable)
      {
        lux_vm_set_error_t(comp->vm, "Function %s needs to return a value", &name);
        return false;
      }
      else
      {
        lux_vm_closure_append_byte(closure, OP_RET);
      }
    }
#endif
  }
  return true;
}
