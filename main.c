#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "public.h"
#include "private.h"

int main(int argc, char* argv[])
{
  if(argc < 2)
  {
    printf("Lux script dev\n");
    printf("Usage: <exe> <scripts...>\n");
    return 0;
  }

  vm_t vm;
  lux_vm_init(&vm);
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

  closure_t* fp = lux_vm_get_function(&vm, "main");

  if(!fp)
  {
    printf("Failed to get function: 'main'\n");
    return 0;
  }

  lux_debug_dump_code(fp);

  //printf("Running 'main'\n");
  //lux_vm_call_function(&vm, fp);

  return 0;
}
