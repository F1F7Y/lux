#ifndef _H_PUBLIC
#define _H_PUBLIC

#include <stdbool.h>

typedef struct vm_s
{
  char lasterror[256];
} vm_t;

bool lux_vm_init(vm_t* vm);
bool lux_vm_load(vm_t* vm, char* buf);

#endif