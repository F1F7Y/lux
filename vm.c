#include "private.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
  return true;
}

bool lux_vm_load(vm_t* vm, char* buf)
{
  lexer_t lexer;
  lux_lexer_init(&lexer, vm, buf);

  compiler_t comp;
  lux_compiler_init(&comp, vm, &lexer);
  TRY(lux_compiler_compile_file(&comp));

  return true;
}

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

bool lux_vm_call_function(vm_t* vm, closure_t* func, vmregister_t* ret)
{
  printf("Calling %s\n", func->name);
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
}

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

static void lux_vm_closure_ensure_free(vm_t* vm, closure_t* closure, int size)
{
  if(closure->allocated - closure->used > size)
  {
    return;
  }

  closure->allocated += 32;

  closure->code = xrealloc(vm, closure->code, closure->allocated);
}

void lux_vm_closure_append_byte(vm_t* vm, closure_t* closure, unsigned char byte)
{
  lux_vm_closure_ensure_free(vm, closure, 1);
  *(unsigned char*)(closure->code + closure->used) = byte;
  closure->used += 1;
}

void lux_vm_closure_append_int(vm_t* vm, closure_t* closure, int i)
{
  lux_vm_closure_ensure_free(vm, closure, 4);
  *(int*)(closure->code + closure->used) = i;
  closure->used += 4;
}

void lux_vm_closure_append_float(vm_t* vm, closure_t* closure, float f)
{
  lux_vm_closure_ensure_free(vm, closure, 4);
  *(float*)(closure->code + closure->used) = f;
  closure->used += 4;
}

bool lux_vm_closure_last_byte_is(vm_t* vm, closure_t* closure, char b)
{
  if(closure->used == 0)
  {
    return false;
  }

  return *(closure->code + closure->used - 1) == b;
}

void lux_vm_set_error(vm_t* vm, char* error)
{
  strncpy(vm->lasterror, error, 256);
  vm->lasterror[255] = '\0';
}

void lux_vm_set_error_s(vm_t* vm, char* error, const char* str1)
{
  snprintf(vm->lasterror, 256, error, str1);
}

void lux_vm_set_error_t(vm_t* vm, char* error, token_t* token)
{
  char tokenbuf[1024];
  strncpy(tokenbuf, token->buf, 1024);
  tokenbuf[token->length] = '\0';
  lux_vm_set_error_s(vm, error, tokenbuf);
}

void lux_vm_set_error_ss(vm_t* vm, char* error, const char* str1, const char* str2)
{
  snprintf(vm->lasterror, 256, error, str1, str2);
}

void lux_vm_set_error_ts(vm_t* vm, char* error, token_t* token, const char* str)
{
  char tokenbuf[1024];
  strncpy(tokenbuf, token->buf, 1024);
  tokenbuf[token->length] = '\0';
  lux_vm_set_error_ss(vm, error, tokenbuf, str);
}

void lux_vm_set_error_st(vm_t* vm, char* error, const char* str, token_t* token)
{
  char tokenbuf[1024];
  strncpy(tokenbuf, token->buf, 1024);
  tokenbuf[token->length] = '\0';
  lux_vm_set_error_ss(vm, error, str, tokenbuf);
}
