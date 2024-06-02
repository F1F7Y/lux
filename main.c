#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  lux_lexer_init(&lexer, buf);

  token_t token;
  while(lux_lexer_get_token(&lexer, &token) != TT_EOF)
  {
    if(!token.length)
    {
      break;
    }

    char temp[1024];
    strncpy(temp, token.buf, token.length);
    temp[token.length] = '\0';
    printf("'%-16s' %3d %3d\n", temp, token.line, token.column);
  }

  fclose(f);
  return 0;
}
