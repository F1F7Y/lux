#include "lexer.h"

void lux_init_lexer(lexer_t* lex, char* buffer)
{
  char* c = buffer;
  for(; *c != '\0'; c++) {}
  lex->buffer = buffer;
  lex->buffer_end = c;
  lex->length = c - buffer;
  lex->cursor = buffer;
}
