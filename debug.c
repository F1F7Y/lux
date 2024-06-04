#include "private.h"

#include <stdio.h>

void lux_debug_dump_code(closure_t* closure)
{
  printf("Dumping closure: %s\n", closure->name);
  if(closure->code == NULL)
  {
    printf("Closure has no code\n");
    return;
  }

  char* code = closure->code;
  char* cursor = code;
  char* code_end = code + closure->used;

  while(cursor < code_end)
  {
    switch(*cursor)
    {
      case OP_NOP:
      {
        printf("nop\n");
        cursor += 1;
      }
      break;
      case OP_LDI:
      {
        printf("ldi %d %d(%f)\n", *(unsigned char*)(cursor + 1), *(int*)(cursor + 2), *(float*)(cursor + 2));
        cursor += 6;
      }
      break;
      case OP_CALL:
      {
        printf("call %d\n", *(unsigned char*)(cursor + 1));
        cursor += 2;
      }
      break;
      case OP_RET:
      {
        printf("ret\n");
        cursor += 1;
      }
      break;
      default:
      {
        printf("Unknown opcode %c\n", *cursor);
        return;
      }
    }
  }
}