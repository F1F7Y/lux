#ifndef _H_PUBLIC
#define _H_PUBLIC

#include <stdbool.h>

/* vm.c */
typedef struct vmtype_s vmtype_t;
typedef struct closure_s closure_t;
typedef struct vmframe_s vmframe_t;
typedef struct vm_s vm_t;

typedef union vmregister_u
{
  int ivalue;
  float fvalue;
} vmregister_t;

typedef struct vm_s
{
  char lasterror[256];
  vmtype_t* types;
  closure_t* functions;
  vmframe_t* frames;
} vm_t;

bool lux_vm_init(vm_t* vm);
bool lux_vm_load(vm_t* vm, char* buf);

closure_t* lux_vm_get_function(vm_t* vm, const char* name);
bool lux_vm_call_function(vm_t* vm, closure_t* func, vmregister_t* ret);

#endif