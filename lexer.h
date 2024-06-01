#ifndef _H_LEXER
#define _H_LEXER

typedef struct lexer_s
{
  char* buffer;
  char* buffer_end;
  unsigned int length;
  char* cursor;
} lexer_t;

void lux_init_lexer(lexer_t* lex, char* buffer);

#endif