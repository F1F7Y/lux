#include "private.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//-----------------------------------------------
// Debug native closure callbacks
static bool callback_printint(vm_t* vm, vmframe_t* frame)
{
  printf("DBG: %i\n", frame->r[1].ivalue);
  return true;
}

static bool callback_printfloat(vm_t* vm, vmframe_t* frame)
{
  printf("DBG: %f\n", frame->r[1].fvalue);
  return true;
}

static bool callback_printbool(vm_t* vm, vmframe_t* frame)
{
  printf("DBG: %s\n", frame->r[1].ivalue ? "true" : "false");
  return true;
}

//-----------------------------------------------
// Initilazes the vm_t struct and registers
// basic types and debug functions
// Returns false on fatal error
//-----------------------------------------------
bool lux_vm_init(vm_t* vm, char* mem, unsigned int memsize)
{
  memset(vm->lasterror, 0, 256);
  vm->types = NULL;
  vm->functions = NULL;
  vm->frames = NULL;

  if(memsize < sizeof(xmemchunk_t))
  {
    return false;
  }

  xmemchunk_t* chunk = vm->freemem = (xmemchunk_t*)mem;
  chunk->size = memsize - sizeof(xmemchunk_t);
  chunk->next = NULL;

  (void)lux_vm_register_type(vm, "void",  false);
  (void)lux_vm_register_type(vm, "int",   true);
  (void)lux_vm_register_type(vm, "float", true);
  (void)lux_vm_register_type(vm, "bool",  true);

  vm->tint = lux_vm_get_type_s(vm, "int");
  vm->tfloat = lux_vm_get_type_s(vm, "float");
  vm->tbool = lux_vm_get_type_s(vm, "bool");

  TRY(lux_vm_register_native_function(vm, "void printint(int)", callback_printint))
  TRY(lux_vm_register_native_function(vm, "void printfloat(float)", callback_printfloat))
  TRY(lux_vm_register_native_function(vm, "void printbool(bool)", callback_printbool))

  return true;
}

//-----------------------------------------------
// Loads and compiles a text buffer into a vm
// Returns false on fatal error
//-----------------------------------------------
bool lux_vm_load(vm_t* vm, char* buf)
{
  lexer_t lexer;
  lux_lexer_init(&lexer, vm, buf);

  compiler_t comp;
  lux_compiler_init(&comp, vm, &lexer);
  TRY(lux_compiler_compile_file(&comp));

  return true;
}

//-----------------------------------------------
// Gets a function by name
// Returns NULL if it doesn't exist
//-----------------------------------------------
closure_t* lux_vm_get_function(vm_t* vm, const char* name)
{
  for(closure_t* fp = vm->functions; fp != NULL; fp = fp->next)
  {
    if(!strcmp(fp->name, name))
    {
      return fp;
    }
  }

  return NULL;
}

//-----------------------------------------------
// Internal implementation for OP_CALL
// Returns false on fatal error
//-----------------------------------------------
bool lux_vm_call_function_internal(vm_t* vm, closure_t* func, vmframe_t* frame)
{
  //printf("Calling %s internal\n", func->name);
  vmframe_t newframe;
  newframe.vm = vm;
  newframe.closure = func;
  newframe.next = vm->frames;
  vm->frames = &newframe;

  for(int i = 0; i < func->numargs; i++)
  {
    newframe.r[i + 1] = frame->r[i + 1];
  }

  if(!func->native)
  {
    TRY(lux_vm_interpret_frame(vm, &newframe))
  }
  else
  {
    TRY(func->callback(vm, &newframe))
  }

  vm->frames = newframe.next;
  frame->r[0] = newframe.r[0];
  return true;
}

//-----------------------------------------------
// Public implementation of a function call
// Doesn't support calling native functions
// Returns false on fatal error
//-----------------------------------------------
bool lux_vm_call_function(vm_t* vm, closure_t* func, vmregister_t* ret)
{
  //printf("Calling %s public\n", func->name);
  vmframe_t frame;
  frame.vm = vm;
  frame.closure = func;
  frame.next = vm->frames;
  vm->frames = &frame;
  TRY(lux_vm_interpret_frame(vm, &frame))
  vm->frames = frame.next;
  *ret = frame.r[0];
  return true;
}

//-----------------------------------------------
// Tries to register a type
// Returns false on fatal error
//-----------------------------------------------
bool lux_vm_register_type(vm_t* vm, const char* type, bool can_be_variable)
{
  if(lux_vm_get_type_s(vm, type) != NULL)
  {
    lux_vm_set_error_s(vm, "Tried to re-register type: '%s'", type);
    return false;
  }
  vmtype_t* t = xalloc(vm, sizeof(*t));
  strncpy(t->name, type, 128);
  t->name[127] = '\0';
  t->next = vm->types;
  t->can_be_variable = can_be_variable;
  vm->types = t;
  return true;
}

//-----------------------------------------------
// Gets a type by its string name
// Returns NULL if it doesn't exist
//-----------------------------------------------
vmtype_t* lux_vm_get_type_s(vm_t* vm, const char* type)
{
  for(vmtype_t* t = vm->types; t != NULL; t = t->next)
  {
    if(!strcmp(t->name, type))
    {
      return t;
    }
  }

  return NULL;
}

//-----------------------------------------------
// Gets a type using a token
// Returns NULL if it doesn't exist
//-----------------------------------------------
vmtype_t* lux_vm_get_type_t(vm_t* vm, token_t* type)
{
  for(vmtype_t* t = vm->types; t != NULL; t = t->next)
  {
    if(!strncmp(t->name, type->buf, type->length) && strlen(t->name) == type->length)
    {
      return t;
    }
  }

  return NULL;
}

//-----------------------------------------------
// Tries to register a function using a string
// Returns NULL on fatal error
//-----------------------------------------------
closure_t* lux_vm_register_function_s(vm_t* vm, const char* name, vmtype_t* rettype)
{
  if(lux_vm_get_function_s(vm, name) != NULL)
  {
    lux_vm_set_error_s(vm, "Function '%s' already exists", name);
    return NULL;
  }

  closure_t* fp = xalloc(vm, sizeof(closure_t));
  strncpy(fp->name, name, 128);
  fp->name[127] = '\0';
  fp->native = false;
  fp->rettype = rettype;
  //fp->numargs = 0;
  fp->code = NULL;
  fp->used = 0;
  fp->allocated = 0;
  fp->next = vm->functions;
  if(vm->functions == NULL)
  {
    fp->index = 0;
  }
  else
  {
    fp->index = vm->functions->index + 1;
  }

  vm->functions = fp;
  return fp;
}

//-----------------------------------------------
// Tries to register a function using a token
// Return NULL on fatal error
//-----------------------------------------------
closure_t* lux_vm_register_function_t(vm_t* vm, token_t* name, vmtype_t* rettype)
{
  if(name->length > 127)
  {
    lux_vm_set_error(vm, "A function name cannot be longer than 127 bytes");
    return NULL;
  }

  char tokenbuf[1024];
  strncpy(tokenbuf, name->buf, 1024);
  tokenbuf[name->length] = '\0';

  return lux_vm_register_function_s(vm, tokenbuf, rettype);
}

//-----------------------------------------------
// Tries to register a native function
// Returns false on fatal error
//-----------------------------------------------
bool lux_vm_register_native_function(vm_t* vm, const char* signature, bool (*callback)(vm_t* vm, vmframe_t* frame))
{
  lexer_t lexer;
  lux_lexer_init(&lexer, vm, (char*)signature);

  token_t ret;
  lux_lexer_get_token(&lexer, &ret);

  vmtype_t* rettype = lux_vm_get_type_t(vm, &ret);
  if(rettype == NULL)
  {
    lux_vm_set_error_t(vm, "Expected return type, got %s instead", &ret);
    return false;
  }

  token_t name;
  lux_lexer_get_token(&lexer, &name);
  if(name.type != TT_NAME)
  {
    lux_vm_set_error_t(vm, "Expected function name, got %s instead", &name);
    return false;
  }

  closure_t* closure = lux_vm_register_function_t(vm, &name, rettype);
  TRY(closure)

  closure->native = true;

  TRY(lux_lexer_expect_token(&lexer, '('))

  while(true)
  {
    token_t token;
    lux_lexer_get_token(&lexer, &token);

    if(*token.buf == ')')
    {
      break;
    }

    if(token.type != TT_NAME)
    {
      lux_vm_set_error_t(vm, "Expected type, got %s", &token);
      return false;
    }

    vmtype_t* type = lux_vm_get_type_t(vm, &token);
    if(type == NULL)
    {
      lux_vm_set_error_t(vm, "Expected type, got %s", &token);
      return false;
    }

    if(closure->numargs == 12)
    {
      lux_vm_set_error(vm, "A function can have up to 12 arguments max");
      return false;
    }

    closure->args[closure->numargs] = type;
    closure->numargs++;

    lux_lexer_get_token(&lexer, &token);
    if(*token.buf == ')')
    {
      break;
    }
    else if(*token.buf != ',')
    {
      lux_vm_set_error_t(vm, "Expected ',', got %s", &token);
      return false;
    }
  }

  closure->callback = callback;

  return true;
}

//-----------------------------------------------
// Gets a function by its string name
// Returns NULL if it doesn't exist
//-----------------------------------------------
closure_t* lux_vm_get_function_s(vm_t* vm, const char* name)
{
  for(closure_t* fp = vm->functions; fp != NULL; fp = fp->next)
  {
    if(!strcmp(fp->name, name))
    {
      return fp;
    }
  }

  return NULL;
}

//-----------------------------------------------
// Gets a function by its token name
// Returns NULL if it doesn't exist
//-----------------------------------------------
closure_t* lux_vm_get_function_t(vm_t* vm, token_t* name)
{
  for(closure_t* fp = vm->functions; fp != NULL; fp = fp->next)
  {
    if(!strncmp(fp->name, name->buf, name->length) && strlen(fp->name) == name->length)
    {
      return fp;
    }
  }

  return NULL;
}

//-----------------------------------------------
// Ensures there's enough space for 'size' bytes
//-----------------------------------------------
static void lux_vm_closure_ensure_free(vm_t* vm, closure_t* closure, int size)
{
  if(closure->allocated - closure->used > size)
  {
    return;
  }

  closure->allocated += 64;

  closure->code = xrealloc(vm, closure->code, closure->allocated);
}

//-----------------------------------------------
// Appends a byte into a closure stream
//-----------------------------------------------
void lux_vm_closure_append_byte(vm_t* vm, closure_t* closure, unsigned char byte)
{
  lux_vm_closure_ensure_free(vm, closure, 1);
  *(unsigned char*)(closure->code + closure->used) = byte;
  closure->used += 1;
}

//-----------------------------------------------
// Appends an int into a closure stream
//-----------------------------------------------
void lux_vm_closure_append_int(vm_t* vm, closure_t* closure, int i)
{
  lux_vm_closure_ensure_free(vm, closure, 4);
  *(int*)(closure->code + closure->used) = i;
  closure->used += 4;
}

//-----------------------------------------------
// Appends a float into a closure stream
//-----------------------------------------------
void lux_vm_closure_append_float(vm_t* vm, closure_t* closure, float f)
{
  lux_vm_closure_ensure_free(vm, closure, 4);
  *(float*)(closure->code + closure->used) = f;
  closure->used += 4;
}

//-----------------------------------------------
// Returns true if the last byte in a closure
// stream is 'b'
//-----------------------------------------------
bool lux_vm_closure_last_byte_is(vm_t* vm, closure_t* closure, char b)
{
  if(closure->used == 0)
  {
    return false;
  }

  return *(closure->code + closure->used - 1) == b;
}

//-----------------------------------------------
// Packs the closure stream into its smallest
// possible allocation size
//-----------------------------------------------
void lux_vm_closure_finish(vm_t* vm, closure_t* closure)
{
  closure->allocated = closure->used;
  closure->code = xrealloc(vm, closure->code, closure->allocated);
}

//-----------------------------------------------
// Sets the error using a string
//-----------------------------------------------
void lux_vm_set_error(vm_t* vm, char* error)
{
  strncpy(vm->lasterror, error, 256);
  vm->lasterror[255] = '\0';
}

//-----------------------------------------------
// Sets the error with one string vararg
//-----------------------------------------------
void lux_vm_set_error_s(vm_t* vm, char* error, const char* str1)
{
  snprintf(vm->lasterror, 256, error, str1);
}

//-----------------------------------------------
// Sets the error with one token vararg
//-----------------------------------------------
void lux_vm_set_error_t(vm_t* vm, char* error, token_t* token)
{
  char tokenbuf[1024];
  strncpy(tokenbuf, token->buf, 1024);
  tokenbuf[token->length] = '\0';
  lux_vm_set_error_s(vm, error, tokenbuf);
}

//-----------------------------------------------
// Sets the error with two string varargs
//-----------------------------------------------
void lux_vm_set_error_ss(vm_t* vm, char* error, const char* str1, const char* str2)
{
  snprintf(vm->lasterror, 256, error, str1, str2);
}

//-----------------------------------------------
// Sets the error with one token and one string
// vararg
//-----------------------------------------------
void lux_vm_set_error_ts(vm_t* vm, char* error, token_t* token, const char* str)
{
  char tokenbuf[1024];
  strncpy(tokenbuf, token->buf, 1024);
  tokenbuf[token->length] = '\0';
  lux_vm_set_error_ss(vm, error, tokenbuf, str);
}

//-----------------------------------------------
// Sets the error with one string and one token
// vararg
//-----------------------------------------------
void lux_vm_set_error_st(vm_t* vm, char* error, const char* str, token_t* token)
{
  char tokenbuf[1024];
  strncpy(tokenbuf, token->buf, 1024);
  tokenbuf[token->length] = '\0';
  lux_vm_set_error_ss(vm, error, str, tokenbuf);
}
