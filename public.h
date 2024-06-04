#ifndef _H_PUBLIC
#define _H_PUBLIC

#include <stdbool.h>

/* vm.c */
typedef struct vmtype_s vmtype_t;
typedef struct functionproto_s functionproto_t;

typedef struct vm_s
{
  char lasterror[256];
  vmtype_t* types;
  functionproto_t* functions;
} vm_t;

bool lux_vm_init(vm_t* vm);
bool lux_vm_load(vm_t* vm, char* buf);

functionproto_t* lux_vm_get_function(vm_t* vm, const char* name);
bool lux_vm_call_function(vm_t* vm, functionproto_t* func);

#endif