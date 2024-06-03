#include "private.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

bool lux_vm_init(vm_t* vm)
{
  memset(vm->lasterror, 0, 256);
  vm->types = NULL;
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
  printf("Compilation succeeded\n");

  return true;
}

bool lux_vm_register_type(vm_t* vm, const char* type, bool can_be_variable)
{
  if(lux_vm_get_type_s(vm, type) != NULL)
  {
    lux_vm_set_error_s(vm, "Tried to re-register type: '%s'", type);
    return false;
  }

  vmtype_t* t = malloc(sizeof(*t));
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
