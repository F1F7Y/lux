#include "private.h"

#include <stdio.h>

void lux_debug_dump_code_all(vm_t* vm)
{
  for(closure_t* c = vm->functions; c != NULL; c = c->next)
  {
    lux_debug_dump_code(c);
  }
}

void lux_debug_dump_code(closure_t* closure)
{
  printf("Dumping closure: %s %s(", closure->rettype->name, closure->name);
  for(int i = 0; i < closure->numargs; i++)
  {
    printf("%s", closure->args[i]->name);
    if(i < closure->numargs - 1)
    {
      printf(", ");
    }
  }
  printf(") (index: %d)\n", closure->index);

  if(closure->native)
  {
    printf("  Closure is native: %p\n", closure->callback);
    return;
  }

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
    printf("  ");
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
        const unsigned char to = *(unsigned char*)(cursor + 1);
        const int ivalue = *(int*)(cursor + 2);
        const float fvalue = *(float*)(cursor + 2);
        printf("ldi    %d %d  // r[%d] <- %d // %f\n", to, ivalue, to, ivalue, fvalue);
        cursor += 6;
      }
      break;
      case OP_CALL:
      {
        const unsigned char idx = *(unsigned char*)(cursor + 1);
        printf("call   %d  // call r[%d]\n", idx, idx);
        cursor += 2;
      }
      break;
      case OP_MOV:
      {
        const unsigned char from = *(unsigned char*)(cursor + 1);
        const unsigned char to = *(unsigned char*)(cursor + 2);
        printf("mov    %d %d  // r[%d] <- r[%d]\n", from, to, to, from);
        cursor += 3;
      }
      break;
      case OP_ADDI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("addi   %d %d %d  // r[%d] <- r[%d] + r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_SUBI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("subi   %d %d %d  // r[%d] <- r[%d] - r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_MULI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("muli   %d %d %d  // r[%d] <- r[%d] * r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_DIVI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("divi   %d %d %d  // r[%d] <- r[%d] / r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_RET:
      {
        printf("ret\n");
        cursor += 1;
      }
      break;
      case OP_ITOF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        printf("itof %d %d  // r[%d] <- (float)r[%d]\n", lv, rv, rv, lv);
        cursor += 3;
      }
      break;
      case OP_ADDF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("addf   %d %d %d  // r[%d] <- r[%d] + r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_SUBF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("subf   %d %d %d  // r[%d] <- r[%d] - r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_MULF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("mulf   %d %d %d  // r[%d] <- r[%d] * r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_DIVF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("divf   %d %d %d  // r[%d] <- r[%d] / r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_FTOI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        printf("ftoi %d %d  // r[%d] <- (int)r[%d]\n", lv, rv, rv, lv);
        cursor += 3;
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