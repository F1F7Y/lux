#ifndef _H_PRIVATE
#define _H_PRIVATE

#include "public.h"

#include <assert.h>

#define TRY(exp) if(!exp) {return false;}

/*
 * Instructions are variable sized always being at least 1 byte
 * General rule is the result is always stored in the last register
*/
enum
{            // Size |                      | Usage
  OP_NOP,    // 1    | <1op>                | No operation
  OP_LDI,    // 6    | <1op,1reg,4value>    | Load value into register
  OP_CALL,   // 2    | <1op,1reg>           | Call function
  OP_RET,    // 1    | <1op>                | Return from function
  OP_MOV,    // 3    | <1op,1reg,1reg>      | Move value of register
  OP_ADDI,   // 4    | <1op,1reg,1reg,1reg> | Add two integers
  OP_SUBI,   // 4    | <1op,1reg,1reg,1reg> | Subtract two integers
  OP_MULI,   // 4    | <1op,1reg,1reg,1reg> | Multiply two integers
  OP_DIVI,   // 4    | <1op,1reg,1reg,1reg> | Divide two integers
  OP_ITOF,   // 3    | <1op,1reg,1reg>      | Cast an int to a float
  OP_ADDF,   // 4    | <1op,1reg,1reg,1reg> | Add two floats
  OP_SUBF,   // 4    | <1op,1reg,1reg,1reg> | Subtract two floats
  OP_MULF,   // 4    | <1op,1reg,1reg,1reg> | Multiply two floats
  OP_DIVF,   // 4    | <1op,1reg,1reg,1reg> | Divide two floats
  OP_FTOI,   // 3    | <1op,1reg,1reg>      | Cast a float to an int
};

typedef struct lexer_s lexer_t;
typedef struct compiler_s compiler_t;
typedef struct token_s token_t;

/* compiler.c */
typedef struct cpvar_s
{
  char name[128];
  vmtype_t* type;
  unsigned char r;
  int z;
} cpvar_t;

enum
{
  RS_NOT_USED, // Register isn't being used
  RS_VARIABLE, // Register is used by a variable
  RS_GENERIC,  // Register is used for generic operations
};

typedef struct compiler_s
{
  vm_t* vm;     // vm that owns us
  lexer_t* lex; // Lexer for the file we're compiling
  int r[256];  // Keeps track of in use registers
  int z;        // Counts nested scopes
  cpvar_t vars[128]; // Local vars;
  int vc;       // Number of vars
} compiler_t;

void lux_compiler_init(compiler_t* comp, vm_t* vm, lexer_t* lex);
bool lux_compiler_compile_file(compiler_t* comp);

void lux_compiler_clear_registers(compiler_t* comp);
bool lux_compiler_alloc_register_generic(compiler_t* comp, unsigned char* reg);
bool lux_compiler_alloc_register_variable(compiler_t* comp, unsigned char* reg);
void lux_compiler_free_register_generic(compiler_t* comp, unsigned char reg);
void lux_compiler_free_register_variable(compiler_t* comp, unsigned char reg);

bool lux_compiler_register_var(compiler_t* comp, vmtype_t* type, token_t* name, cpvar_t** var);
cpvar_t* lux_compiler_get_var(compiler_t* comp, token_t* name);
void lux_compiler_enter_scope(compiler_t* comp);
void lux_compiler_leave_scope(compiler_t* comp);

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
  vm_t* vm;            // vm that owns us
  char* buffer;        // Start of buffer
  char* buffer_end;    // End of buffer ( '\0' at all times or big bad )
  unsigned int length; // Total length of buffer
  char* cursor;        // Cursor in the buffer
  int column;          // Current column
  int line;            // Current line
  token_t lasttoken;   // Last token
  bool token_avalible; // If lasttoken is next
} lexer_t;

void lux_lexer_init(lexer_t* lex, vm_t* vm, char* buffer);
int  lux_lexer_get_token(lexer_t* lex, token_t* token);
bool lux_lexer_expect_token(lexer_t* lex, char token);
void lux_lexer_unget_last_token(lexer_t* lex);
bool lux_lexer_is_reserved(token_t* token);
bool lux_token_is_c(token_t* token, char c);
bool lux_token_is_str(token_t* token, const char* str);

/* vm.c */
typedef struct vmtype_s
{
  char name[128];
  bool can_be_variable;
  vmtype_t* next;
} vmtype_t;

typedef struct closure_s
{
  char name[128];
  bool native;
  vmtype_t* rettype;
  int numargs;
  vmtype_t* args[12];
  int index;
  char* code;
  int used;
  int allocated;
  closure_t* next;
} closure_t;

typedef struct vmframe_s
{
  vm_t* vm;
  closure_t* closure;
  vmregister_t r[256];
  vmframe_t* next;
} vmframe_t;

bool lux_vm_call_function_internal(vm_t* vm, closure_t* func, vmframe_t* frame);

bool      lux_vm_register_type(vm_t* vm, const char* type, bool can_be_variable);
vmtype_t* lux_vm_get_type_s(vm_t* vm, const char* type);
vmtype_t* lux_vm_get_type_t(vm_t* vm, token_t* type);

closure_t* lux_vm_register_function_s(vm_t* vm, const char* name, vmtype_t* rettype);
closure_t* lux_vm_register_function_t(vm_t* vm, token_t* name, vmtype_t* rettype);
closure_t* lux_vm_get_function_s(vm_t* vm, const char* name);
closure_t* lux_vm_get_function_t(vm_t* vm, token_t* name);

void lux_vm_closure_append_byte(vm_t* vm, closure_t* closure, unsigned char byte);
void lux_vm_closure_append_int(vm_t* vm, closure_t* closure, int i);
void lux_vm_closure_append_float(vm_t* vm, closure_t* closure, float f);
bool lux_vm_closure_last_byte_is(vm_t* vm, closure_t* closure, char b);
void lux_vm_closure_finish(vm_t* vm, closure_t* closure);

void lux_vm_set_error(vm_t* vm, char* error);
void lux_vm_set_error_s(vm_t* vm, char* error, const char* str1);
void lux_vm_set_error_t(vm_t* vm, char* error, token_t* token);
void lux_vm_set_error_ss(vm_t* vm, char* error, const char* str1, const char* str2);
void lux_vm_set_error_ts(vm_t* vm, char* error, token_t* token, const char* str);
void lux_vm_set_error_st(vm_t* vm, char* error, const char* str, token_t* token);

/* interpreter.c */
bool lux_vm_interpret_frame(vm_t* vm, vmframe_t* frame);

/* debug.c */
void lux_debug_dump_code_all(vm_t* vm);
void lux_debug_dump_code(closure_t* closure);

/* mem.c */
typedef struct xmemchunk_s
{
  unsigned int size;
  xmemchunk_t* next;
} xmemchunk_t;

void* xalloc(vm_t* vm, unsigned int size);
void* xrealloc(vm_t* vm, void* ptr, unsigned int size);
void  xfree(vm_t* vm, void* ptr);

#endif