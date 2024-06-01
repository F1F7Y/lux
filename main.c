#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

int main(int argc, char* argv[])
{
  printf("Hello world!\n");

  FILE* f = fopen("draft.lux", "r");
  if(!f)
  {
    printf("Failed to open 'draft.lux'\n");
    return -1;
  }
  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buf = malloc(size + 1);
  fread(buf, size, 1, f);
  buf[size] = '\0';

  lexer_t lexer;
  lux_init_lexer(&lexer, buf);

  fclose(f);
  return 0;
}
