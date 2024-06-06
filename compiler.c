#include "private.h"

#include <stdio.h>
#include <string.h>

void lux_compiler_init(compiler_t* comp, vm_t* vm, lexer_t* lex)
{
  comp->vm = vm;
  comp->lex = lex;
}

static unsigned char lux_instruction_for_operator_int(char operator)
{
  switch(operator)
  {
    case '+': return OP_ADDI;
    case '-': return OP_SUBI;
    case '*': return OP_MULI;
    case '/': return OP_DIVI;
  }
  assert(false);
  return OP_ADDI;
}

// Parses '+ 9' then calls itself
static bool lux_compiler_expression_e(compiler_t* comp, closure_t* closure, bool priority, unsigned char lv, unsigned char* ret)
{
  token_t op;
  lux_lexer_get_token(comp->lex, &op);
  if(priority && (*op.buf == '+' || *op.buf == '-'))
  {
    lux_lexer_unget_last_token(comp->lex);
    return true;
  }

  switch(*op.buf)
  {
    case '+':
    case '-':
    case '*':
    case '/':
      break;
    default:
      lux_lexer_unget_last_token(comp->lex);
      *ret = lv;
      return true;
  }

  token_t rvalue;
  lux_lexer_get_token(comp->lex, &rvalue);

  if(rvalue.type != TT_INT)
  {
    lux_vm_set_error_t(comp->vm, "Expected TT_INT for value '%s'", &rvalue);
    return false;
  }

  // Check if next operation has higer priority if so do it first
  token_t nextop;
  lux_lexer_get_token(comp->lex, &nextop);
  lux_lexer_unget_last_token(comp->lex);
  if((*op.buf == '+' || *op.buf == '-') && (*nextop.buf == '*' || *nextop.buf == '/'))
  {
    unsigned char rv;
    TRY(lux_compiler_get_register(comp, &rv));
    lux_vm_closure_append_byte(closure, OP_LDI);
    lux_vm_closure_append_byte(closure, rv);
    lux_vm_closure_append_int(closure, rvalue.ivalue);
    unsigned char rr;
    TRY(lux_compiler_expression_e(comp, closure, true, rv, &rr));

    unsigned char r;
    TRY(lux_compiler_get_register(comp, &r));

    lux_vm_closure_append_byte(closure, lux_instruction_for_operator_int(*op.buf));
    lux_vm_closure_append_byte(closure, lv);
    lux_vm_closure_append_byte(closure, rr);
    lux_vm_closure_append_byte(closure, r);

    lux_compiler_free_register(comp, lv);
    lux_compiler_free_register(comp, rr);

    TRY(lux_compiler_expression_e(comp, closure, false, r, ret));
    return true;
  }

  unsigned char rv;
  TRY(lux_compiler_get_register(comp, &rv));
  lux_vm_closure_append_byte(closure, OP_LDI);
  lux_vm_closure_append_byte(closure, rv);
  lux_vm_closure_append_int(closure, rvalue.ivalue);

  unsigned char rr;
  TRY(lux_compiler_get_register(comp, &rr));

  lux_vm_closure_append_byte(closure, lux_instruction_for_operator_int(*op.buf));
  lux_vm_closure_append_byte(closure, lv);
  lux_vm_closure_append_byte(closure, rv);
  lux_vm_closure_append_byte(closure, rr);

  lux_compiler_free_register(comp, lv);
  lux_compiler_free_register(comp, rv);

  *ret = rr;

  lux_lexer_get_token(comp->lex, &op);
  lux_lexer_unget_last_token(comp->lex);
  switch(*op.buf)
  {
    case '+':
    case '-':
    case '*':
    case '/':
    {
      TRY(lux_compiler_expression_e(comp, closure, priority, rr, ret))
    }
    break;
    default:
    {
      return true;
    }
  }

  return true;
}


// Parses initial number then calls lux_compiler_expression_e
static bool lux_compiler_expression(compiler_t* comp, closure_t* closure, unsigned char* ret)
{
  token_t value;
  lux_lexer_get_token(comp->lex, &value);

  if(value.type != TT_INT)
  {
    lux_vm_set_error_t(comp->vm, "Expected TT_INT for value '%s'", &value);
    return false;
  }

  unsigned char vr;
  TRY(lux_compiler_get_register(comp, &vr))
  lux_vm_closure_append_byte(closure, OP_LDI);
  lux_vm_closure_append_byte(closure, vr);
  lux_vm_closure_append_int(closure, value.ivalue);

  token_t op;
  lux_lexer_get_token(comp->lex, &op);
  lux_lexer_unget_last_token(comp->lex);
  switch(*op.buf)
  {
    case '+':
    case '-':
    case '*':
    case '/':
    {
      TRY(lux_compiler_expression_e(comp, closure, false, vr, ret))
    }
    break;
    default:
    {
      *ret = vr;
      return true;
    }
  }

  return true;
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
          unsigned char retvalue;
          TRY(lux_compiler_expression(comp, closure, &retvalue));

          lux_vm_closure_append_byte(closure, OP_MOV);
          lux_vm_closure_append_byte(closure, retvalue);
          lux_vm_closure_append_byte(closure, 0);

          lux_compiler_free_register(comp, retvalue);
        }
        lux_vm_closure_append_byte(closure, OP_RET);
        continue;
      }
      
      closure_t* f = lux_vm_get_function_t(comp->vm, &token);
      if(f != NULL)
      {
        TRY(lux_lexer_expect_token(comp->lex, '('));
        TRY(lux_lexer_expect_token(comp->lex, ')'));

        if(f->rettype->can_be_variable)
        {
          printf("WARN: Function return value ignored\n");
        }

        lux_vm_closure_append_byte(closure, OP_LDI);
        lux_vm_closure_append_byte(closure, 0);
        lux_vm_closure_append_int(closure, f->index);

        lux_vm_closure_append_byte(closure, OP_CALL);
        lux_vm_closure_append_byte(closure, 0);
        continue;
      }
      
      vmtype_t* t = lux_vm_get_type_t(comp->vm, &token);
      if(t != NULL)
      {
        token_t name;
        lux_lexer_get_token(comp->lex, &name);
        // verify this

        token_t peek;
        lux_lexer_get_token(comp->lex, &peek);
        if(peek.length == 1 && *peek.buf == '=')
        {
          unsigned char retvalue;
          TRY(lux_compiler_expression(comp, closure, &retvalue));
        }
        else
        {
          lux_lexer_unget_last_token(comp->lex);
          printf("WARM: Variable not initilazed\n");
        }
        continue;
      }
    }

    lux_vm_set_error_t(comp->vm, "Unexpected token: %s", &token);
    return false;
  }

  assert(false);
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

    lux_compiler_clear_registers(comp);
    TRY(lux_compiler_scope(comp, closure));

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
  }
  return true;
}

void lux_compiler_clear_registers(compiler_t* comp)
{
  memset(comp->r, 0, sizeof(bool) * 256);
  comp->r[0] = true;
}

bool lux_compiler_get_register(compiler_t* comp, unsigned char* reg)
{
  for(int i = 1; i < 256; i++)
  {
    if(!comp->r[i])
    {
      comp->r[i] = true;
      *reg = i;
      return true;
    }
  }
  lux_vm_set_error(comp->vm, "Compiler ran out of registers");
  return false;
}

void lux_compiler_free_register(compiler_t* comp, unsigned char reg)
{
  comp->r[reg] = false;
}
