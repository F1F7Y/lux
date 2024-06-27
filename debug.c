#include "private.h"

#include <stdio.h>

//-----------------------------------------------
// Dumps bytecode for all closures
//-----------------------------------------------
void lux_debug_dump_code_all(vm_t* vm)
{
  for(closure_t* c = vm->functions; c != NULL; c = c->next)
  {
    lux_debug_dump_code(c);
  }
}

//-----------------------------------------------
// Dumps bytecode for a specific closure
//-----------------------------------------------
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
  printf(") (index: %d) %d Bytes\n", closure->index, closure->used);

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

  unsigned char* code = closure->code;
  unsigned char* cursor = code;
  unsigned char* code_end = code + closure->used;

  while(cursor < code_end)
  {
    printf("%4ld  ", cursor - code);
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
      case OP_MOD:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("mod    %d %d %d  // r[%d] <- r[%d] / r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_RET:
      {
        printf("ret\n");
        cursor += 1;
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
      case OP_ITOF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        printf("itof   %d %d  // r[%d] <- (float)r[%d]\n", lv, rv, rv, lv);
        cursor += 3;
      }
      break;
      case OP_FTOI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        printf("ftoi   %d %d  // r[%d] <- (int)r[%d]\n", lv, rv, rv, lv);
        cursor += 3;
      }
      break;
      case OP_EQI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("eqi    %d %d %d  // r[%d] <- (bool)(r[%d] == r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_NEQI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("neqi   %d %d %d  // r[%d] <- (bool)(r[%d] != r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_EQF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("eqf    %d %d %d  // r[%d] <- (bool)(r[%d] == r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_NEQF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("neqf   %d %d %d  // r[%d] <- (bool)(r[%d] != r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_LTI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("lti    %d %d %d  // r[%d] <- (bool)(r[%d] < r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_LTEI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("ltei   %d %d %d  // r[%d] <- (bool)(r[%d] <= r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_MTI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("mti    %d %d %d  // r[%d] <- (bool)(r[%d] > r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_MTEI:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("mtei   %d %d %d  // r[%d] <- (bool)(r[%d] >= r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_LTF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("ltf    %d %d %d  // r[%d] <- (bool)(r[%d] < r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_LTEF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("ltef   %d %d %d  // r[%d] <- (bool)(r[%d] <= r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_MTF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("mtf    %d %d %d  // r[%d] <- (bool)(r[%d] > r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_MTEF:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("mtef   %d %d %d  // r[%d] <- (bool)(r[%d] >= r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_LAND:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("land   %d %d %d  // r[%d] <- (bool)(r[%d] >= r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_LOR:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("lor    %d %d %d  // r[%d] <- (bool)(r[%d] >= r[%d])\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_BAND:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("band   %d %d %d  // r[%d] <- r[%d] & r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_BXOR:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("bxor   %d %d %d  // r[%d] <- r[%d] ^ r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_BOR:
      {
        const unsigned char lv = *(unsigned char*)(cursor + 1);
        const unsigned char rv = *(unsigned char*)(cursor + 2);
        const unsigned char res = *(unsigned char*)(cursor + 3);
        printf("bor    %d %d %d  // r[%d] <- r[%d] | r[%d]\n", lv, rv, res, res, lv, rv);
        cursor += 4;
      }
      break;
      case OP_JMP:
      {
        const int offset = *(int*)(cursor + 1);
        printf("jmp    %d\n", offset);
        cursor += 5;
      }
      break;
      case OP_BEQZ:
      {
        const unsigned char r = *(unsigned char*)(cursor + 1);
        const int offset = *(int*)(cursor + 2);
        printf("beqz   %d %d\n", r, offset);
        cursor += 6;
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
