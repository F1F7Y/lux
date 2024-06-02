#include "lexer.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define WHITESPACE " \t\n\r"
#define CHARTOKENS "(){}[]+-*/\\<>!=@#$%^&|,.;"

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

static void lux_lexer_token_check_integer(lexer_t* lex, token_t* token)
{
  for(int i = 0; i < token->length; i++)
  {
    if(!isdigit(token->buf[i]))
    {
      return;
    }
  }

  token->ivalue = strtol(token->buf, NULL, 10);
  token->type = TT_INT;
}

static void lux_lexer_token_check_float(lexer_t* lex, token_t* token)
{
  for(int i = 0; i < token->length; i++)
  {
    if(!isdigit(token->buf[i]) && token->buf[i] != '.')
    {
      return;
    }
  }
  char* end = NULL;
  token->fvalue = strtof(token->buf, &end);
  token->type = TT_FLOAT;
  token->length = end - token->buf;
  lex->cursor = end;
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
  
  // Skip comments
  if(*c == '/' && *(c+1) == '/')
  {
    for(; *c != '\n' && *c != '\r'; c++){}
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
  }

  lex->cursor = c;

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

  if(token->length == 1)
  {
    for(char* w = CHARTOKENS; *w != '\0'; w++)
    {
      if(*token->buf == *w)
      {
        token->type = TT_TOKEN;
        break;
      }
    }
  }

  lex->cursor = c;

  lux_lexer_token_check_float(lex, token);
  lux_lexer_token_check_integer(lex, token);

  lex->column += token->length;
  return token->type;
}
