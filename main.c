#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
      printf("Failed to compile '%s'\n", file);
      printf("At line: %d column: %d\n", vm.errorline, vm.errorcolumn);
      printf("Error: %s\n", vm.lasterror);
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
    goto ret;
  }

  const clock_t start = clock();
  
  printf("Running 'main'\n");
  vmregister_t ret;
  if(!lux_vm_call_function(&vm, fp, &ret))
  {
    printf("Error running code: %s\n", vm.lasterror);
  }
  printf("main returned: %d\n", ret.ivalue);

  const clock_t end = clock();
  printf("Execution took %ld msec\n", (end - start) / (CLOCKS_PER_SEC / 1000));


#endif
ret:
  for(xmemchunk_t* m = vm.freemem; m != NULL; m = m->next)
  {
    printf("MEMCHUNK: %p size: %d next: %p\n", m, m->size, m->next);
  }
  return 0;
}
