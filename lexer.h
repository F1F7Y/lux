#ifndef _H_LEXER
#define _H_LEXER

#include "types.h"

enum
{
  TT_EOF,   // End of file
  TT_NAME,  // Multi character name
  TT_TOKEN, // Single character token ( eg '!' or '<' )
  TT_INT,   // Integer
  TT_FLOAT  // Float
};

typedef struct token_s
{
  int type;            // Type of the token
  char* buf;           // Pointer to first char
  unsigned int length; // Length of the token
  int ivalue;          // Integer value, only valid when TT_INT
  float fvalue;        // Float value, only valid when TT_FLOAT
  int column;          // Column pointing to first char
  int line;            // Line
} token_t;

typedef struct lexer_s
{
  char* buffer;        // Start of buffer
  char* buffer_end;    // End of buffer ( '\0' at all times or big bad )
  unsigned int length; // Total length of buffer
  char* cursor;        // Cursor in the buffer
  int column;          // Current column
  int line;            // Current line
} lexer_t;

void lux_lexer_init(lexer_t* lex, char* buffer);

int lux_lexer_get_token(lexer_t* lex, token_t* token);

#endif