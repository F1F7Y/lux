#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "public.h"
#include "private.h"

#define MEMSIZE 1024 * 8

int main(int argc, char* argv[])
{
  if(argc < 2)
  {
    printf("Lux script dev\n");
    printf("Usage: <exe> <scripts...>\n");
    return 0;
  }

  void* mem = malloc(MEMSIZE);

  vm_t vm;
  lux_vm_init(&vm, mem, MEMSIZE);
  for(int i = 1 ; i < argc; i++)
  {
    const char* file = argv[i];
    printf("Loading %s\n", file);
    FILE* f = fopen(file, "r");
    if(!f)
    {
      printf("Failed to open %s, skipping\n", file);
      continue;
    }

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc(size + 1);
    fread(buf, size, 1, f);
    buf[size] = '\0';
    fclose(f);

    if(!lux_vm_load(&vm, buf))
    {
      printf("Failed to compile %s with error: %s\n", file, vm.lasterror);
      return 0;
    }

    free(buf);
  }

  printf("Compiled\n");

#if 1
  lux_debug_dump_code_all(&vm);
#endif
#if 1
  closure_t* fp = lux_vm_get_function(&vm, "main");

  if(!fp)
  {
    printf("Failed to get function: 'main'\n");
    return 0;
  }

  printf("Running 'main'\n");
  vmregister_t ret;
  if(!lux_vm_call_function(&vm, fp, &ret))
  {
    printf("Error running code: %s\n", vm.lasterror);
  }
  printf("main returned: %d\n", ret.ivalue);
#endif
  {
#if 0
    for(xmemchunk_t* m = vm.freemem; m != NULL; m = m->next)
    {
      printf("MEMCHUNK: %p size: %d next: %p\n", m, m->size, m->next);
    }

    printf("alloc start\n");

    void* ptrs[16];

    for(int i = 0; i < 16; i++)
    {
      ptrs[i] = xalloc(&vm, 100);
    }
    for(int i = 0; i < 16; i++)
    {
      if(i % 5 == 0) continue;
      xfree(&vm, ptrs[i]);
    }

    for(int i = 0; i < 16; i++)
    {
      if(i % 5 != 0) continue;
      xfree(&vm, ptrs[i]);
    }

    printf("alloc end\n");
#endif
    for(xmemchunk_t* m = vm.freemem; m != NULL; m = m->next)
    {
      printf("MEMCHUNK: %p size: %d next: %p\n", m, m->size, m->next);
    }
  }
  return 0;
}
