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
  fclose(f);

  lexer_t lexer;
  lux_lexer_init(&lexer, buf);

  printf("%-32s | line | column | type\n", "value");

  token_t token;
  while(lux_lexer_get_token(&lexer, &token) != TT_EOF)
  {
    if(!token.length)
    {
      break;
    }
    switch(token.type)
    {
      case TT_INT:
      {
        printf("%-32d | %4d | %6d | int\n", token.ivalue, token.line, token.column);
      }
      break;
      case TT_FLOAT:
      {
        printf("%-32f | %4d | %6d | float\n", token.fvalue, token.line, token.column);
      }
      break;
      case TT_TOKEN:
      {
        printf("%-32c | %4d | %6d | token\n", *token.buf, token.line, token.column);
      }
      break;
      default:
      {
        char temp[1024];
        strncpy(temp, token.buf, token.length);
        temp[token.length] = '\0';
        printf("%-32s | %4d | %6d | %3d\n", temp, token.line, token.column, token.type);
      }
    }
  }

  free(buf);
  return 0;
}
