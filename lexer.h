#ifndef _H_LEXER
#define _H_LEXER

enum
{
  TT_EOF,
  TT_NAME
};

typedef struct token_s
{
  int type;
  char* buf;
  unsigned int length;
  int column;
  int line;
} token_t;

typedef struct lexer_s
{
  char* buffer;
  char* buffer_end;
  unsigned int length;
  char* cursor;
  int column;
  int line;
} lexer_t;

void lux_lexer_init(lexer_t* lex, char* buffer);

int lux_lexer_get_token(lexer_t* lex, token_t* token);

#endif