#include "private.h"

#include <string.h>
#include <stdio.h>

bool lux_vm_init(vm_t* vm)
{
  memset(vm->lasterror, 0, 256);
  return true;
}

bool lux_vm_load(vm_t* vm, char* buf)
{
  lexer_t lexer;
  lux_lexer_init(&lexer, vm, buf);

  compiler_t comp;
  lux_compiler_init(&comp, vm, &lexer);
  TRY(lux_compile_file(&comp));
  printf("Compilation succeeded\n");

  return true;
}

void lux_vm_set_error(vm_t* vm, char* error)
{
  strncpy(vm->lasterror, error, 256);
  vm->lasterror[255] = '\0';
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
