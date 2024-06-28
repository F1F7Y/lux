#include "private.h"

#include <stdio.h>
#include <string.h>

static bool lux_compiler_expression(compiler_t* comp, closure_t* closure, vmtype_t* wishtype, unsigned char* _retreg, vmtype_t** _rettype, bool allowprimary);
static bool lux_compiler_scope(compiler_t* comp, closure_t* closure);

//-----------------------------------------------
// Initilazes the compiler_t struct
//-----------------------------------------------
void lux_compiler_init(compiler_t* comp, vm_t* vm, lexer_t* lex)
{
  comp->vm = vm;
  comp->lex = lex;
  lux_compiler_clear_registers(comp);
  comp->z = 0;
  comp->vc = 0;
}

//-----------------------------------------------
// Returns true if operator is supported
//-----------------------------------------------
static bool lux_operator_supported(token_t* token)
{
  switch(token->type)
  {
    case TT_PLUS:
    case TT_MINUS:
    case TT_MULT:
    case TT_DIV:
    case TT_MOD:
    case TT_LESS:
    case TT_LESSEQ:
    case TT_MORE:
    case TT_MOREEQ:
    case TT_EQUALS:
    case TT_NOTEQUALS:
    case TT_LOGICAND:
    case TT_LOGICOR:
    case TT_BWAND:
    case TT_BWXOR:
    case TT_BWOR:
      return true;
  }
  return false;
}

//-----------------------------------------------
// Returns the priority of an operator
// The higher the priority the sooner it'll be
// evaluated
//-----------------------------------------------
static int lux_operator_priority(token_t* token)
{
  switch(token->type)
  {
    case TT_LOGICOR:
      return 0;
    case TT_LOGICAND:
      return 1;
    case TT_BWOR:
      return 2;
    case TT_BWXOR:
      return 3;
    case TT_BWAND:
      return 4;
    case TT_EQUALS:
    case TT_NOTEQUALS:
      return 5;
    case TT_LESS:
    case TT_LESSEQ:
    case TT_MORE:
    case TT_MOREEQ:
      return 6;
    case TT_PLUS:
    case TT_MINUS:
      return 7;
    case TT_MULT:
    case TT_DIV:
    case TT_MOD:
      return 8;
  }

  return 0;
}

//-----------------------------------------------
// Returns a opcode for an operator
// Returns false on fatal error
//-----------------------------------------------
static bool lux_instruction_for_operator(vm_t* vm, vmtype_t* ltype, vmtype_t* rtype, token_t* operator, unsigned char* _op, vmtype_t** _type)
{
  if(ltype == vm->tint && rtype == vm->tint)
  {
    switch(operator->type)
    {
      case TT_PLUS: *_op = OP_ADDI; *_type = vm->tint; return true;
      case TT_MINUS: *_op = OP_SUBI; *_type = vm->tint; return true;
      case TT_MULT: *_op = OP_MULI; *_type = vm->tint; return true;
      case TT_DIV: *_op = OP_DIVI; *_type = vm->tint; return true;
      case TT_MOD: *_op = OP_MOD; *_type = vm->tint; return true;
      case TT_LESS: *_op = OP_LTI; *_type = vm->tbool; return true;
      case TT_LESSEQ: *_op = OP_LTEI; *_type = vm->tbool; return true;
      case TT_MORE: *_op = OP_MTI; *_type = vm->tbool; return true;
      case TT_MOREEQ: *_op = OP_MTEI; *_type = vm->tbool; return true;
      case TT_EQUALS: *_op = OP_EQI; *_type = vm->tbool; return true;
      case TT_NOTEQUALS: *_op = OP_NEQI; *_type = vm->tbool; return true;
      case TT_BWAND: *_op = OP_BAND; *_type = vm->tint; return true;
      case TT_BWXOR: *_op = OP_BXOR; *_type = vm->tint; return true;
      case TT_BWOR: *_op = OP_BOR; *_type = vm->tint; return true;
    }
  }
  else if(ltype == vm->tfloat && rtype == vm->tfloat)
  {
    switch(operator->type)
    {
      case TT_PLUS: *_op = OP_ADDF; *_type = vm->tfloat; return true;
      case TT_MINUS: *_op = OP_SUBF; *_type = vm->tfloat; return true;
      case TT_MULT: *_op = OP_MULF; *_type = vm->tfloat; return true;
      case TT_DIV: *_op = OP_DIVF; *_type = vm->tfloat; return true;
      case TT_LESS: *_op = OP_LTF; *_type = vm->tbool; return true;
      case TT_LESSEQ: *_op = OP_LTEF; *_type = vm->tbool; return true;
      case TT_MORE: *_op = OP_MTF; *_type = vm->tbool; return true;
      case TT_MOREEQ: *_op = OP_MTEF; *_type = vm->tbool; return true;
      case TT_EQUALS: *_op = OP_EQF; *_type = vm->tbool; return true;
      case TT_NOTEQUALS: *_op = OP_NEQF; *_type = vm->tbool; return true;
    }
  }
  else if(ltype == vm->tbool && rtype == vm->tbool)
  {
    switch(operator->type)
    {
      case TT_LOGICAND: *_op = OP_LAND; *_type = vm->tbool; return true;
      case TT_LOGICOR: *_op = OP_LOR; *_type = vm->tbool; return true;
      case TT_EQUALS: *_op = OP_EQI; *_type = vm->tbool; return true;
      case TT_NOTEQUALS: *_op = OP_NEQI; *_type = vm->tbool; return true;
    }
  }

  lux_vm_set_error_ss(vm, "Unknown operation for types %s %s", ltype->name, rtype->name);
  return false;
}

//-----------------------------------------------
// Parses arguments for a function call
// Returns false on fatal error
//-----------------------------------------------
static bool lux_compiler_function_call(compiler_t* comp, closure_t* closure, closure_t* called)
{
  TRY(lux_lexer_expect_token(comp->lex, '('))
  for(int i = 0; i < called->numargs; i++)
  {
    vmtype_t* argtype;
    unsigned char reg;
    TRY(lux_compiler_expression(comp, closure, called->args[i], &reg, &argtype, false))

    if(argtype != called->args[i])
    {
      lux_vm_set_error_ss(comp->vm, "Function expected argument of type %s, got %s instead", called->args[i]->name, argtype->name);
      return false;
    }

    TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 3));
    lux_vm_closure_append_byte(comp->vm, closure, OP_MOV);
    lux_vm_closure_append_byte(comp->vm, closure, reg);
    lux_vm_closure_append_byte(comp->vm, closure, i + 1);

    lux_compiler_free_register_generic(comp, reg);

    if(i < called->numargs - 1)
    {
      TRY(lux_lexer_expect_token(comp->lex, ','))
    }
  }
  TRY(lux_lexer_expect_token(comp->lex, ')'))

  return true;
}

//-----------------------------------------------
// Parses a value and tries to cast it
// A value can be a single number, variable,
// function call or an entire expression
// Returns false on fatal error
//-----------------------------------------------
static bool lux_compiler_parse_value(compiler_t* comp, closure_t* closure, token_t* value, unsigned char* ret, vmtype_t** rettype)
{
  // Parse the value
  cpvar_t* var;
  closure_t* c;
  if(lux_token_is_c(value, '('))
  {
    TRY(lux_compiler_expression(comp, closure, NULL, ret, rettype, false))
    TRY(lux_lexer_expect_token(comp->lex, ')'))
  }
  else if(value->type == TT_NAME && (var = lux_compiler_get_var(comp, value)) != NULL)
  {
    *ret = var->r;
    *rettype = var->type;
  }
  else if (value->type == TT_NAME && (c = lux_vm_get_function_t(comp->vm, value)) != NULL)
  {
    TRY(lux_compiler_function_call(comp, closure, c))
      
    TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 11));
    lux_vm_closure_append_byte(comp->vm, closure, OP_LDI);
    lux_vm_closure_append_byte(comp->vm, closure, 0);
    lux_vm_closure_append_int(comp->vm, closure, c->index);

    lux_vm_closure_append_byte(comp->vm, closure, OP_CALL);
    lux_vm_closure_append_byte(comp->vm, closure, 0);

    TRY(lux_compiler_alloc_register_generic(comp, ret))

    lux_vm_closure_append_byte(comp->vm, closure, OP_MOV);
    lux_vm_closure_append_byte(comp->vm, closure, 0);
    lux_vm_closure_append_byte(comp->vm, closure, *ret);

    *rettype = c->rettype;
  }
  else if(value->type == TT_INT)
  {
    TRY(lux_compiler_alloc_register_generic(comp, ret))
    TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 6));
    lux_vm_closure_append_byte(comp->vm, closure, OP_LDI);
    lux_vm_closure_append_byte(comp->vm, closure, *ret);
    lux_vm_closure_append_int(comp->vm, closure, value->ivalue);
    *rettype = comp->vm->tint;
  }
  else if(value->type == TT_FLOAT)
  {
    TRY(lux_compiler_alloc_register_generic(comp, ret))
    TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 6));
    lux_vm_closure_append_byte(comp->vm, closure, OP_LDI);
    lux_vm_closure_append_byte(comp->vm, closure, *ret);
    lux_vm_closure_append_float(comp->vm, closure, value->fvalue);
    *rettype = comp->vm->tfloat;
  }
  else if(value->type == TT_BOOL)
  {
    TRY(lux_compiler_alloc_register_generic(comp, ret))
    TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 6));
    lux_vm_closure_append_byte(comp->vm, closure, OP_LDI);
    lux_vm_closure_append_byte(comp->vm, closure, *ret);
    lux_vm_closure_append_int(comp->vm, closure, value->ivalue);
    *rettype = comp->vm->tbool;
  }
  else
  {
    lux_vm_set_error_t(comp->vm, "Failed to get value from: '%s'", value);
    return false;
  }

  return true;
}

//-----------------------------------------------
// Tries to cast a value, return false if it
// couldn't
// NOTE: Doesn't allocate memory, you need to
//       alloc 3 bytes before calling this!!!
//-----------------------------------------------
static bool lux_compiler_try_cast(compiler_t* comp, closure_t* closure, vmtype_t* ft, unsigned char fr, vmtype_t* tt, unsigned char* rr)
{
  if(ft == comp->vm->tint && tt == comp->vm->tfloat) // int -> float
  {
    lux_compiler_alloc_register_generic(comp, rr);
    lux_vm_closure_append_byte(comp->vm, closure, OP_ITOF);
    lux_vm_closure_append_byte(comp->vm, closure, fr);
    lux_vm_closure_append_byte(comp->vm, closure, *rr);
    lux_compiler_free_register_generic(comp, fr);
    return true;
  }
  else if(ft == comp->vm->tfloat && tt == comp->vm->tint) // float -> int
  {
    lux_compiler_alloc_register_generic(comp, rr);
    lux_vm_closure_append_byte(comp->vm, closure, OP_FTOI);
    lux_vm_closure_append_byte(comp->vm, closure, fr);
    lux_vm_closure_append_byte(comp->vm, closure, *rr);
    lux_compiler_free_register_generic(comp, fr);
    return true;
  }

  return false;
}

//-----------------------------------------------
// Parses the operator and value after it
// Uses recursion for operator precedence
// Returns false on fatal error
//-----------------------------------------------
static bool lux_compiler_expression_e(compiler_t* comp, closure_t* closure, int priority, unsigned char lreg, vmtype_t* ltype, unsigned char* _retreg, vmtype_t** _rettype)
{
  // Get op
  token_t op;
  lux_lexer_get_token(comp->lex, &op);
  if(!lux_operator_supported(&op))
  {
    // Invalid op, end of expression
    lux_lexer_unget_last_token(comp->lex);
    *_retreg = lreg;
    *_rettype = ltype;
    return true;
  }

  if(lux_operator_priority(&op) < priority)
  {
    // End of higher priority chain
    lux_lexer_unget_last_token(comp->lex);
    *_retreg = lreg;
    *_rettype = ltype;
    return true;
  }

  // Get op rvalue
  token_t rvalue;
  lux_lexer_get_token(comp->lex, &rvalue);

  unsigned char rval;
  vmtype_t* rvtype;
  TRY(lux_compiler_parse_value(comp, closure, &rvalue, &rval, &rvtype))

  // Peek next op
  token_t nextop;
  lux_lexer_get_token(comp->lex, &nextop);
  lux_lexer_unget_last_token(comp->lex);
  while(lux_operator_priority(&nextop) > lux_operator_priority(&op) && lux_operator_supported(&nextop))
  {
    // Next operation has higher priority, do it first
    unsigned char res;
    vmtype_t* restype;
    TRY(lux_compiler_expression_e(comp, closure, lux_operator_priority(&nextop), rval, rvtype, &res, &restype))
    rval = res;
    rvtype = restype;
    // The next operation after is also higher priority (loop till we can break out)
    lux_lexer_get_token(comp->lex, &nextop);
    lux_lexer_unget_last_token(comp->lex);
  }

  // If at least one value is a float promote the other to float too
  if(ltype == comp->vm->tfloat || rvtype == comp->vm->tfloat)
  {
    unsigned char tempreg;
    TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 6));
    if(lux_compiler_try_cast(comp, closure, ltype, lreg, comp->vm->tfloat, &tempreg))
    {
      ltype = comp->vm->tfloat;
      lreg = tempreg;
    }
    if(lux_compiler_try_cast(comp, closure, rvtype, rval, comp->vm->tfloat, &tempreg))
    {
      rvtype = comp->vm->tfloat;
      rval = tempreg;
    }
  }

  unsigned char resreg;
  unsigned char resop;
  vmtype_t* restype;
  TRY(lux_instruction_for_operator(comp->vm, ltype, rvtype, &op, &resop, &restype))
  TRY(lux_compiler_alloc_register_generic(comp, &resreg))

  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 4));
  lux_vm_closure_append_byte(comp->vm, closure, resop);
  lux_vm_closure_append_byte(comp->vm, closure, lreg);
  lux_vm_closure_append_byte(comp->vm, closure, rval);
  lux_vm_closure_append_byte(comp->vm, closure, resreg);

  lux_compiler_free_register_generic(comp, lreg);
  lux_compiler_free_register_generic(comp, rval);

  *_retreg = rval = resreg;
  *_rettype = rvtype = restype;

  // If we're the first iteration check again for more
  if(priority == -1)
  {
    TRY(lux_compiler_expression_e(comp, closure, -1, rval, rvtype, _retreg, _rettype))
  }

  return true;
}

//-----------------------------------------------
// Parses an expression
// Parses the initial value then relies on
// lux_compiler_expression_e to recursively
// parse
// Returns false on fatal error
//-----------------------------------------------
static bool lux_compiler_expression(compiler_t* comp, closure_t* closure, vmtype_t* wishtype, unsigned char* _retreg, vmtype_t** _rettype, bool allowprimary)
{
  // Get value
  token_t value;
  lux_lexer_get_token(comp->lex, &value);

  unsigned char valr;
  vmtype_t* valtype;
  // Check if we're trying to asign an expression to a variable
  cpvar_t* var = lux_compiler_get_var(comp, &value);
  if(var != NULL)
  {
    token_t nextop;
    lux_lexer_get_token(comp->lex, &nextop);

    if(nextop.type == TT_ASIGN)
    {
      TRY(lux_compiler_expression(comp, closure, var->type, &valr, &valtype, false));

      if(valtype != var->type)
      {
        lux_vm_set_error_ss(comp->vm, "Can't assign %s to %s", valtype->name, var->type->name);
        return false;
      }

      TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 3));
      lux_vm_closure_append_byte(comp->vm, closure, OP_MOV);
      lux_vm_closure_append_byte(comp->vm, closure, valr);
      lux_vm_closure_append_byte(comp->vm, closure, var->r);

      lux_compiler_free_register_generic(comp, valr);

      *_retreg = var->r;
      *_rettype = var->type;

      return true;
    }
    lux_lexer_unget_last_token(comp->lex);
  }
  // Check if we're declaring a new variable
  vmtype_t* type = lux_vm_get_type_t(comp->vm, &value);
  if(allowprimary && type != NULL)
  {
    token_t name;
    lux_lexer_get_token(comp->lex, &name);
    if(name.type != TT_NAME)
    {
      lux_vm_set_error_t(comp->vm, "Expected variable name, got %s", &name);
      return false;
    }

    cpvar_t* var;
    TRY(lux_compiler_register_var(comp, type, &name, &var))

    token_t nextop;
    lux_lexer_get_token(comp->lex, &nextop);

    if(nextop.type != TT_ASIGN)
    {
      if(!lux_operator_supported(&nextop))
      {
        lux_lexer_unget_last_token(comp->lex);
        printf("WARN: Variable '%s' not initilazed\n", var->name);
        return true;
      }

      lux_vm_set_error(comp->vm, "Expected '='");
      return false;
    }

    TRY(lux_compiler_expression(comp, closure, var->type, &valr, &valtype, false));

    if(valtype != var->type)
    {
      lux_vm_set_error_ss(comp->vm, "Can't assign %s to %s", valtype->name, type->name);
      return false;
    }

    TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 3));
    lux_vm_closure_append_byte(comp->vm, closure, OP_MOV);
    lux_vm_closure_append_byte(comp->vm, closure, valr);
    lux_vm_closure_append_byte(comp->vm, closure, var->r);

    lux_compiler_free_register_generic(comp, valr);

    *_retreg = var->r;
    *_rettype = var->type;

    return true;
  }

  // Try to get the value and parse the rest of the expression
  TRY(lux_compiler_parse_value(comp, closure, &value, &valr, &valtype))

  // Peek next op
  token_t nextop;
  lux_lexer_get_token(comp->lex, &nextop);
  lux_lexer_unget_last_token(comp->lex);
  if(lux_operator_supported(&nextop))
  {
    // We got a valid operator, recurse
    TRY(lux_compiler_expression_e(comp, closure, -1, valr, valtype, _retreg, _rettype))
  }
  else
  {
    // Only single value, set out vars
    *_retreg = valr;
    *_rettype = valtype;
  }

  // Try to cast to desired type
  unsigned char cr;
  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 3));
  if(lux_compiler_try_cast(comp, closure, *_rettype, *_retreg, wishtype, &cr))
  {
    *_retreg = cr;
    *_rettype = wishtype;
  }

  return true;
}

//-----------------------------------------------
// Parses an if statement
// Uses recursion for 'else if' and 'else' chains
// Returns false on fatal error
//-----------------------------------------------
bool lux_compiler_if_statement(compiler_t* comp, closure_t* closure)
{
  // Parse expression
  TRY(lux_lexer_expect_token(comp->lex, '('))
  unsigned char resval;
  vmtype_t* restype;
  TRY(lux_compiler_expression(comp, closure, comp->vm->tbool, &resval, &restype, false))
  TRY(lux_lexer_expect_token(comp->lex, ')'))

  if(restype != comp->vm->tbool)
  {
    lux_vm_set_error_s(comp->vm, "Expected type bool in if expression, got %s", restype->name);
    return false;
  }

  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 6));
  lux_vm_closure_append_byte(comp->vm, closure, OP_BEQZ);
  lux_vm_closure_append_byte(comp->vm, closure, resval);
  int beqzoffset = closure->used;
  lux_vm_closure_append_int(comp->vm, closure, 0);

  lux_compiler_free_register_generic(comp, resval);

  TRY(lux_compiler_scope(comp, closure))

  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 5));
  lux_vm_closure_append_byte(comp->vm, closure, OP_JMP);
  int jmpoffset = closure->used;
  lux_vm_closure_append_int(comp->vm, closure, 0);

  *(int*)(closure->code + beqzoffset) = closure->used;

  // Check for chain
  token_t token;
  lux_lexer_get_token(comp->lex, &token);
  if(lux_token_is_str(&token, "else"))
  {
    lux_lexer_get_token(comp->lex, &token);
    if(lux_token_is_str(&token, "if"))
    {
      TRY(lux_compiler_if_statement(comp, closure));
      *(int*)(closure->code + jmpoffset) = closure->used;
      return true;
    }
    else
    {
      lux_lexer_unget_last_token(comp->lex);
    }
    TRY(lux_compiler_scope(comp, closure))
    *(int*)(closure->code + jmpoffset) = closure->used;
  }
  else
  {
    *(int*)(closure->code + jmpoffset) = closure->used;
    lux_lexer_unget_last_token(comp->lex);
  }

  return true;
}

//-----------------------------------------------
// Parses a while statement
// Returns false on fatal error
//-----------------------------------------------
bool lux_compiler_while_statement(compiler_t* comp, closure_t* closure)
{
  int start = closure->used;
  
  // Parse expression
  TRY(lux_lexer_expect_token(comp->lex, '('))
  unsigned char resval;
  vmtype_t* restype;
  TRY(lux_compiler_expression(comp, closure, comp->vm->tbool, &resval, &restype, false))
  TRY(lux_lexer_expect_token(comp->lex, ')'))

  if(restype != comp->vm->tbool)
  {
    lux_vm_set_error_s(comp->vm, "Expected type bool in if expression, got %s", restype->name);
    return false;
  }

  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 6));
  lux_vm_closure_append_byte(comp->vm, closure, OP_BEQZ);
  lux_vm_closure_append_byte(comp->vm, closure, resval);
  int beqzoffset = closure->used;
  lux_vm_closure_append_int(comp->vm, closure, 0);

  lux_compiler_free_register_generic(comp, resval);

  TRY(lux_compiler_scope(comp, closure))

  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 5));
  lux_vm_closure_append_byte(comp->vm, closure, OP_JMP);
  lux_vm_closure_append_int(comp->vm, closure, start);

  *(int*)(closure->code + beqzoffset) = closure->used;

  return true;
}

//-----------------------------------------------
// Parses a for statement
// Returns false on fatal error
//-----------------------------------------------
bool lux_compiler_for_statement(compiler_t* comp, closure_t* closure)
{
  lux_compiler_enter_scope(comp);
  TRY(lux_lexer_expect_token(comp->lex, '('));
  // First set of primary expressions
  bool seconditer = false;
  while(true)
  {
    token_t peek;
    lux_lexer_get_token(comp->lex, &peek);
    lux_lexer_unget_last_token(comp->lex);

    if(lux_token_is_c(&peek, ';'))
    {
      break;
    }
    else if(seconditer)
    {
      TRY(lux_lexer_expect_token(comp->lex, ','))
    }

    unsigned char resval;
    vmtype_t* restype;
    TRY(lux_compiler_expression(comp, closure, NULL, &resval, &restype, true))
    lux_compiler_free_register_generic(comp, resval);
    seconditer = true;
  }

  TRY(lux_lexer_expect_token(comp->lex, ';'))

  int start = closure->used;
  
  // Conditional expression
  unsigned char resval;
  vmtype_t* restype;
  TRY(lux_compiler_expression(comp, closure, comp->vm->tbool, &resval, &restype, true))

  if(restype != comp->vm->tbool)
  {
    lux_vm_set_error_s(comp->vm, "for loop condition needs to evaluate to 'bool' not '%s'", restype->name);
    return false;
  }

  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 6));
  lux_vm_closure_append_byte(comp->vm, closure, OP_BEQZ);
  lux_vm_closure_append_byte(comp->vm, closure, resval);
  int beqzoffset = closure->used;
  lux_vm_closure_append_int(comp->vm, closure, 0);

  TRY(lux_lexer_expect_token(comp->lex, ';'))
  
  // Third set of expressions appended to the end
  closure_t tempclosure;
  memset(&tempclosure, 0, sizeof(closure_t));
  seconditer = false;
  while(true)
  {
    token_t peek;
    lux_lexer_get_token(comp->lex, &peek);
    lux_lexer_unget_last_token(comp->lex);

    if(lux_token_is_c(&peek, ')'))
    {
      break;
    }
    else if(seconditer)
    {
      TRY(lux_lexer_expect_token(comp->lex, ','))
    }

    unsigned char resval;
    vmtype_t* restype;
    TRY(lux_compiler_expression(comp, &tempclosure, NULL, &resval, &restype, true))
    lux_compiler_free_register_generic(comp, resval);
    seconditer = true;
  }

  TRY(lux_lexer_expect_token(comp->lex, ')'))

  TRY(lux_compiler_scope(comp, closure))

  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, tempclosure.used));
  lux_vm_closure_append_bytes(comp->vm, closure, tempclosure.code, tempclosure.used);
  xfree(comp->vm, tempclosure.code);
  
  // Jump to condition at the start
  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 5));
  lux_vm_closure_append_byte(comp->vm, closure, OP_JMP);
  lux_vm_closure_append_int(comp->vm, closure, start);

  *(int*)(closure->code + beqzoffset) = closure->used;

  lux_compiler_leave_scope(comp);
  return true;
}

//-----------------------------------------------
// Parses a return statement
// Returns false on fatal error
//-----------------------------------------------
bool lux_compiler_return_statement(compiler_t* comp, closure_t* closure)
{
  if(closure->rettype->can_be_variable)
  {
    unsigned char retvalue;
    vmtype_t* rettype;
    TRY(lux_compiler_expression(comp, closure, closure->rettype, &retvalue, &rettype, false));

    unsigned char possiblecast;
    TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 3));
    if(lux_compiler_try_cast(comp, closure, rettype, retvalue, closure->rettype, &possiblecast))
    {
      rettype = closure->rettype;
      retvalue = possiblecast;
    }

    if(rettype != closure->rettype)
    {
      lux_vm_set_error_ss(comp->vm, "Incompatible return type '%s' for function '%s'", rettype->name, closure->name);
      return false;
    }

    TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 3));
    lux_vm_closure_append_byte(comp->vm, closure, OP_MOV);
    lux_vm_closure_append_byte(comp->vm, closure, retvalue);
    lux_vm_closure_append_byte(comp->vm, closure, 0);

    lux_compiler_free_register_generic(comp, retvalue);
  }

  TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 1));
  lux_vm_closure_append_byte(comp->vm, closure, OP_RET);
  return true;
}

//-----------------------------------------------
// Parses an entire scope from { to }
// Returns false on fatal error
//-----------------------------------------------
static bool lux_compiler_scope(compiler_t* comp, closure_t* closure)
{
  lux_compiler_enter_scope(comp);
  TRY(lux_lexer_expect_token(comp->lex, '{'))
  while(true)
  {
    token_t token;
    lux_lexer_get_token(comp->lex, &token);

    if(lux_token_is_c(&token, '{'))
    {
      lux_lexer_unget_last_token(comp->lex);
      TRY(lux_compiler_scope(comp, closure))
      continue;
    }
    else if(lux_token_is_c(&token, '}'))
    {
      lux_compiler_leave_scope(comp);
      return true;
    }
    else if(lux_token_is_str(&token, "if"))
    {
      TRY(lux_compiler_if_statement(comp, closure))
      continue;
    }
    else if(lux_token_is_str(&token, "while"))
    {
      TRY(lux_compiler_while_statement(comp, closure))
      continue;
    }
    else if(lux_token_is_str(&token, "for"))
    {
      TRY(lux_compiler_for_statement(comp, closure))
      continue;
    }
    else if(lux_token_is_str(&token, "return"))
    {
      TRY(lux_compiler_return_statement(comp, closure))
      continue;
    }
    else
    {
      lux_lexer_unget_last_token(comp->lex);
      unsigned char resval;
      vmtype_t* restype;
      TRY(lux_compiler_expression(comp, closure, NULL, &resval, &restype, true));
      lux_compiler_free_register_generic(comp, resval);
      continue;
    }

    lux_vm_set_error_t(comp->vm, "Unexpected token: %s", &token);
    return false;
  }

  assert(false);
  return false;
}

//-----------------------------------------------
// Runs the compiler
// Returns false on fatal error
//-----------------------------------------------
bool lux_compiler_compile_file(compiler_t* comp)
{
  token_t dummy;
  while(lux_lexer_get_token(comp->lex, &dummy) != TT_EOF)
  {
    lux_compiler_clear_registers(comp);

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

    lux_compiler_enter_scope(comp);

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

      cpvar_t* var;
      TRY(lux_compiler_register_var(comp, vt, &name, &var))
      if(closure->numargs > 12)
      {
        lux_vm_set_error(comp->vm, "A function can only have up to 12 arguments");
        return false;
      }

      closure->args[closure->numargs] = vt;
      closure->numargs++;

      TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 3));
      lux_vm_closure_append_byte(comp->vm, closure, OP_MOV);
      lux_vm_closure_append_byte(comp->vm, closure, closure->numargs);
      lux_vm_closure_append_byte(comp->vm, closure, var->r);

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

    if(!lux_vm_closure_last_byte_is(comp->vm, closure, OP_RET))
    {
      if(closure->rettype->can_be_variable)
      {
        lux_vm_set_error_t(comp->vm, "Function %s needs to return a value", &name);
        return false;
      }
      else
      {
        TRYMEM(lux_vm_closure_ensure_free(comp->vm, closure, 1));
        lux_vm_closure_append_byte(comp->vm, closure, OP_RET);
      }
    }
    lux_vm_closure_finish(comp->vm, closure);
    lux_compiler_leave_scope(comp);
  }
  return true;
}

//-----------------------------------------------
// Clears all registers to RS_NOT_USED
//-----------------------------------------------
void lux_compiler_clear_registers(compiler_t* comp)
{
  memset(comp->r, 0, sizeof(int) * 256);
  comp->r[0] = true;
}

//-----------------------------------------------
// Allocates a register of type RS_GENERIC
// Returns false on fatal error
//-----------------------------------------------
bool lux_compiler_alloc_register_generic(compiler_t* comp, unsigned char* reg)
{
  // r0 is return values
  // r1 - 12 is func args
  for(int i = 1 + 12; i < 256; i++)
  {
    if(comp->r[i] == RS_NOT_USED)
    {
      comp->r[i] = RS_GENERIC;
      *reg = i;
      return true;
    }
  }
  lux_vm_set_error(comp->vm, "Compiler ran out of registers");
  return false;
}

//-----------------------------------------------
// Allocates a register of type RS_VARIABLE
// Returns false on fatal error
//-----------------------------------------------
bool lux_compiler_alloc_register_variable(compiler_t* comp, unsigned char* reg)
{
  // r0 is return values
  // r1 - 12 is func args
  for(int i = 1 + 12; i < 256; i++)
  {
    if(comp->r[i] == RS_NOT_USED)
    {
      comp->r[i] = RS_VARIABLE;
      *reg = i;
      return true;
    }
  }
  lux_vm_set_error(comp->vm, "Compiler ran out of registers");
  return false;
}

//-----------------------------------------------
// Sets a register to RS_NOT_USED only if it is
// of type RS_GENERIC
//-----------------------------------------------
void lux_compiler_free_register_generic(compiler_t* comp, unsigned char reg)
{
  if(comp->r[reg] == RS_GENERIC)
  {
    comp->r[reg] = RS_NOT_USED;
  }
}

//-----------------------------------------------
// Sets a register to RS_NOT_USED only if it is
// of type RS_VARIABLE
//-----------------------------------------------
void lux_compiler_free_register_variable(compiler_t* comp, unsigned char reg)
{
  if(comp->r[reg] == RS_VARIABLE)
  {
    comp->r[reg] = RS_NOT_USED;
  }
}

//-----------------------------------------------
// Registers a variable
// Returns false on fatal error
//-----------------------------------------------
bool lux_compiler_register_var(compiler_t* comp, vmtype_t* type, token_t* name, cpvar_t** var)
{
  for(int i = 0; i < comp->vc; i++)
  {
    if(!strncmp(comp->vars[i].name, name->buf, name->length) && strlen(comp->vars[i].name) == name->length)
    {
      lux_vm_set_error_t(comp->vm, "Variable %s already exists", name);
      return false;
    }
  }
  for(closure_t *c = comp->vm->functions; c != NULL; c = c->next)
  {
    if(!strncmp(c->name, name->buf, name->length) && strlen(c->name) == name->length)
    {
      lux_vm_set_error_t(comp->vm, "Variable %s cant share a name with a function of the same name", name);
      return false;
    }
  }

  if(comp->vc == 128)
  {
    lux_vm_set_error(comp->vm, "Maximum allowed number of local variables is 128");
    return false;
  }

  if(name->length > 127)
  {
    lux_vm_set_error(comp->vm, "A variable name can be up to 128 bytes");
    return false;
  }

  cpvar_t* v = *var = &comp->vars[comp->vc];
  strncpy(v->name, name->buf, name->length);
  v->name[name->length] = '\0';
  v->type = type;
  v->z = comp->z;
  TRY(lux_compiler_alloc_register_variable(comp, &v->r))
  comp->vc++;

  return true;
}

//-----------------------------------------------
// Gets a variable
// Returns NULL if it doesn't exist
//-----------------------------------------------
cpvar_t* lux_compiler_get_var(compiler_t* comp, token_t* name)
{
  for(int i = 0; i < comp->vc; i++)
  {
    if(!strncmp(comp->vars[i].name, name->buf, name->length) && strlen(comp->vars[i].name) == name->length)
    {
      return &comp->vars[i];
    }
  }

  return NULL;
}

//-----------------------------------------------
// Increases the compiler scope tracker
//-----------------------------------------------
void lux_compiler_enter_scope(compiler_t* comp)
{
  comp->z++;
}

//-----------------------------------------------
// Leaves a scope and cleans up all variables
// declared in it
//-----------------------------------------------
void lux_compiler_leave_scope(compiler_t* comp)
{
  comp->z--;
  int n = 0;
  for(int i = 0; i < comp->vc; i++)
  {
    if(comp->vars[i].z > comp->z)
    {
      lux_compiler_free_register_variable(comp, comp->vars[i].r);
    }
    else
    {
      n++;
    }
  }
  comp->vc = n;
}
