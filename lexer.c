#include "lexer.h"

#include <stdbool.h>

#define WHITESPACE " \t\n\r"
#define CHARTOKENS "(){}[]+-*/\\<>!=@#$%^&|"

void lux_lexer_init(lexer_t* lex, char* buffer)
{
  char* c = buffer;
  for(; *c != '\0'; c++) {}
  lex->buffer = buffer;
  lex->buffer_end = c;
  lex->length = c - buffer;
  lex->cursor = buffer;
  lex->column = 0;
  lex->line = 1;
}

int lux_lexer_get_token(lexer_t* lex, token_t* token)
{
  // Skip over whitespace
  char* c = lex->cursor;
  for(; *c != '\0'; c++)
  {
    bool whitespace = false;
    for(char* w = WHITESPACE; *w != '\0'; w++)
    {
      if(*c == *w)
      {
        lex->column++;
        whitespace = true;
        break;
      }
    }
    if(*c == '\r')
    {
      if(*(c+1) == '\n')
      {
        c++;
      }
      lex->line++;
      lex->column = 1;
    }
    else if(*c == '\n')
    {
      lex->line++;
      lex->column = 1;
    }

    if(!whitespace)
    {
      break;
    }
  }
  lex->cursor = c;
  
  if(*lex->cursor == '\0')
  {
    token->type = TT_EOF;
    token->buf = lex->cursor;
    token->length = 1;
    token->line = lex->line;
    token->column = lex->column;
    return token->type;
  }

  // Get token
  for(;; c++)
  {
    if(*c == '\0')
    {
      break;
    }
    bool end = false;
    for(char* w = WHITESPACE; *w != '\0'; w++)
    {
      if(*c == *w)
      {
        end = true;
        break;
      }
    }
    for(char* w = CHARTOKENS; *w != '\0'; w++)
    {
      if(*c == *w)
      {
        end = true;
        break;
      }
    }
    if(end)
    {
      break;
    }
  }

  if(!(c - lex->cursor))
  {
    c++;
  }

  token->type = TT_NAME;
  token->buf = lex->cursor;
  token->length = c - lex->cursor;
  token->line = lex->line;
  token->column = lex->column;

  lex->column += token->length;
  lex->cursor = c;
  return token->type;
}
