#include "private.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define WHITESPACE " \t\n\r"
#define CHARTOKENS "(){}[]+-*/\\<>!?=@#$%^&|,.;"

static const char* reserved_tokens[] =
{
  "true", "false"
};

//-----------------------------------------------
// Initilazes the lexer_t struct
//-----------------------------------------------
void lux_lexer_init(lexer_t* lex, vm_t* vm, char* buffer)
{
  char* c = buffer;
  for(; *c != '\0'; c++) {}
  lex->vm = vm;
  lex->buffer = buffer;
  lex->buffer_end = c;
  lex->length = c - buffer;
  lex->cursor = buffer;
  lex->column = 0;
  lex->line = 1;
  memset(&lex->lasttoken, 0, sizeof(token_t));
  lex->token_avalible = false;
}

//-----------------------------------------------
// Check if a token is an integer
// If it is convert it to TT_INT
//-----------------------------------------------
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

//-----------------------------------------------
// Check if a token is a float
// If it is conver it to TT_FLOAT
//-----------------------------------------------
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

//-----------------------------------------------
// Check if a token is a float
// If it is conver it to TT_BOOL
//-----------------------------------------------
static void lux_lexer_token_check_bool(lexer_t* lex, token_t* token)
{
  if(lux_token_is_str(token, "true"))
  {
    token->ivalue = 1;
  }
  else if(lux_token_is_str(token, "false"))
  {
    token->ivalue = 0;
  }
  else
  {
    return;
  }
  token->type = TT_BOOL;
}

//-----------------------------------------------
// Gets a token and returns its type
//-----------------------------------------------
int lux_lexer_get_token(lexer_t* lex, token_t* token)
{
  if(lex->token_avalible)
  {
    lex->token_avalible = false;
    memcpy(token, &lex->lasttoken, sizeof(token_t));
    return token->type;
  }

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
    token->buf = "<eof>";
    token->length = 5;
    token->line = lex->line;
    token->column = lex->column;
    memcpy(&lex->lasttoken, token, sizeof(token_t));
    return token->type;
  }

  // Skip comments
  if(*c == '/' && *(c+1) == '/')
  {
    for(; *c != '\n' && *c != '\r' && *c != '\0'; c++){}
    lex->cursor = c;
    return lux_lexer_get_token(lex, token);
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
  lux_lexer_token_check_bool(lex, token);

  lex->column += token->length;

  memcpy(&lex->lasttoken, token, sizeof(token_t));
  return token->type;
}

//-----------------------------------------------
// Gets a token and returns false if it isn't
// what we wanted
//-----------------------------------------------
bool lux_lexer_expect_token(lexer_t* lex, char token)
{
  token_t tk;
  lux_lexer_get_token(lex, &tk);

  if(tk.length == 1 && *tk.buf == token)
  {
    return true;
  }

  char temp[2];
  temp[0] = token;
  temp[1] = '\0';
  lux_vm_set_error_st(lex->vm, "Expected '%s', got '%s'", temp, &tk);
  return false;
}

//-----------------------------------------------
// Ungets the last token
// Asserts if there already is a token waiting
//-----------------------------------------------
void lux_lexer_unget_last_token(lexer_t* lex)
{
  assert(!lex->token_avalible);
  lex->token_avalible = true;
}

//-----------------------------------------------
// Returns true if token is a reserved word
//-----------------------------------------------
bool lux_lexer_is_reserved(token_t* token)
{
  for(int i = 0; i < sizeof(reserved_tokens)/sizeof(*reserved_tokens); i++)
  {
    const char* rt = reserved_tokens[i];
    if(!strncmp(rt, token->buf, token->length) && strlen(rt) == token->length)
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------
// Returns true if token is c
//-----------------------------------------------
bool lux_token_is_c(token_t* token, char c)
{
  return token->length == 1 && token->type == TT_TOKEN && *token->buf == c;
}

//-----------------------------------------------
// Returns true if token is str
//-----------------------------------------------
bool lux_token_is_str(token_t* token, const char* str)
{
  if(token->type != TT_NAME)
  { return false; }

  return strlen(str) == token->length && !strncmp(str, token->buf, token->length);
}
