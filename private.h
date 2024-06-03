#ifndef _H_PRIVATE
#define _H_PRIVATE

#include "public.h"

#define TRY(exp) if(!exp) {return false;}

typedef struct lexer_s lexer_t;
typedef struct compiler_s compiler_t;

/* compiler.c */
typedef struct compiler_s
{
  vm_t* vm;
  lexer_t* lex;
} compiler_t;

void lux_compiler_init(compiler_t* comp, vm_t* vm, lexer_t* lex);
bool lux_compiler_compile_file(compiler_t* comp);

/* lexer.c */
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
  vm_t* vm;
  char* buffer;        // Start of buffer
  char* buffer_end;    // End of buffer ( '\0' at all times or big bad )
  unsigned int length; // Total length of buffer
  char* cursor;        // Cursor in the buffer
  int column;          // Current column
  int line;            // Current line
} lexer_t;

void lux_lexer_init(lexer_t* lex, vm_t* vm, char* buffer);
int lux_lexer_get_token(lexer_t* lex, token_t* token);
bool lux_lexer_expect_token(lexer_t* lex, char token);

/* vm.c */
void lux_vm_set_error(vm_t* vm, char* error);
void lux_vm_set_error_ss(vm_t* vm, char* error, const char* str1, const char* str2);
void lux_vm_set_error_ts(vm_t* vm, char* error, token_t* token, const char* str);
void lux_vm_set_error_st(vm_t* vm, char* error, const char* str, token_t* token);

#endif