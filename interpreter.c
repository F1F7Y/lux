#include "private.h"

#include <stdio.h>

//-----------------------------------------------
// Interprets a vmframe_t closure stream
// Returns false on fatal error
//-----------------------------------------------
bool lux_vm_interpret_frame(vm_t* vm, vmframe_t* frame)
{
  unsigned char* code = frame->closure->code;
  unsigned char* cursor = code;
  unsigned char* code_end = code + frame->closure->used;

  while(cursor < code_end)
  {
    switch(*cursor)
    {
      case OP_NOP:
      {
        cursor += 1;
      }
      break;
      case OP_LDI:
      {
        frame->r[*(unsigned char*)(cursor + 1)].ivalue = *(int*)(cursor + 2);
        cursor += 6;
      }
      break;
      case OP_CALL:
      {
        closure_t* closure = NULL;
        for(closure = vm->functions; closure != NULL; closure = closure->next)
        {
          if(closure->index == frame->r[*(unsigned char*)(cursor + 1)].ivalue)
          {
            break;
          }
        }
        TRY(lux_vm_call_function_internal(vm, closure, frame));
        cursor += 2;
      }
      break;
      case OP_RET:
      {
        //printf("Function %s returning %d (%f)\n", frame->closure->name, frame->r[0].ivalue, frame->r[0].fvalue);
        return true;
      }
      break;
      case OP_MOV:
      {
        frame->r[*(unsigned char*)(cursor + 2)].ivalue = frame->r[*(unsigned char*)(cursor + 1)].ivalue;
        cursor += 3;
      }
      break;
      case OP_ADDI:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = frame->r[*(unsigned char*)(cursor + 1)].ivalue + frame->r[*(unsigned char*)(cursor + 2)].ivalue;
        cursor += 4;
      }
      break;
      case OP_SUBI:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = frame->r[*(unsigned char*)(cursor + 1)].ivalue - frame->r[*(unsigned char*)(cursor + 2)].ivalue;
        cursor += 4;
      }
      break;
      case OP_MULI:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = frame->r[*(unsigned char*)(cursor + 1)].ivalue * frame->r[*(unsigned char*)(cursor + 2)].ivalue;
        cursor += 4;
      }
      break;
      case OP_DIVI:
      {
        assert(frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = frame->r[*(unsigned char*)(cursor + 1)].ivalue / frame->r[*(unsigned char*)(cursor + 2)].ivalue;
        cursor += 4;
      }
      break;
      case OP_MOD:
      {
        assert(frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = frame->r[*(unsigned char*)(cursor + 1)].ivalue % frame->r[*(unsigned char*)(cursor + 2)].ivalue;
        cursor += 4;
      }
      break;
      case OP_ITOF:
      {
        frame->r[*(unsigned char*)(cursor + 2)].fvalue = (float)frame->r[*(unsigned char*)(cursor + 1)].ivalue;
        cursor += 3;
      }
      break;
      case OP_ADDF:
      {
        frame->r[*(unsigned char*)(cursor + 3)].fvalue = frame->r[*(unsigned char*)(cursor + 1)].fvalue + frame->r[*(unsigned char*)(cursor + 2)].fvalue;
        cursor += 4;
      }
      break;
      case OP_SUBF:
      {
        frame->r[*(unsigned char*)(cursor + 3)].fvalue = frame->r[*(unsigned char*)(cursor + 1)].fvalue - frame->r[*(unsigned char*)(cursor + 2)].fvalue;
        cursor += 4;
      }
      break;
      case OP_MULF:
      {
        frame->r[*(unsigned char*)(cursor + 3)].fvalue = frame->r[*(unsigned char*)(cursor + 1)].fvalue * frame->r[*(unsigned char*)(cursor + 2)].fvalue;
        cursor += 4;
      }
      break;
      case OP_DIVF:
      {
        assert(frame->r[*(unsigned char*)(cursor + 2)].fvalue);
        frame->r[*(unsigned char*)(cursor + 3)].fvalue = frame->r[*(unsigned char*)(cursor + 1)].fvalue / frame->r[*(unsigned char*)(cursor + 2)].fvalue;
        cursor += 4;
      }
      break;
      case OP_FTOI:
      {
        frame->r[*(unsigned char*)(cursor + 2)].ivalue = (int)frame->r[*(unsigned char*)(cursor + 1)].fvalue;
        cursor += 3;
      }
      break;
      case OP_EQI:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].ivalue == frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        cursor += 4;
      }
      break;
      case OP_NEQI:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].ivalue != frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        cursor += 4;
      }
      break;
      case OP_EQF:
      {
        frame->r[*(unsigned char*)(cursor + 3)].fvalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].fvalue == frame->r[*(unsigned char*)(cursor + 2)].fvalue);
        cursor += 4;
      }
      break;
      case OP_NEQF:
      {
        frame->r[*(unsigned char*)(cursor + 3)].fvalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].fvalue != frame->r[*(unsigned char*)(cursor + 2)].fvalue);
        cursor += 4;
      }
      break;
      case OP_LTI:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].ivalue < frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        cursor += 4;
      }
      break;
      case OP_LTEI:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].ivalue <= frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        cursor += 4;
      }
      break;
      case OP_MTI:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].ivalue > frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        cursor += 4;
      }
      break;
      case OP_MTEI:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].ivalue >= frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        cursor += 4;
      }
      break;
      case OP_LTF:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].fvalue < frame->r[*(unsigned char*)(cursor + 2)].fvalue);
        cursor += 4;
      }
      break;
      case OP_LTEF:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].fvalue <= frame->r[*(unsigned char*)(cursor + 2)].fvalue);
        cursor += 4;
      }
      break;
      case OP_MTF:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].fvalue > frame->r[*(unsigned char*)(cursor + 2)].fvalue);
        cursor += 4;
      }
      break;
      case OP_MTEF:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].fvalue >= frame->r[*(unsigned char*)(cursor + 2)].fvalue);
        cursor += 4;
      }
      break;
      case OP_LAND:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].ivalue && frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        cursor += 4;
      }
      break;
      case OP_LOR:
      {
        frame->r[*(unsigned char*)(cursor + 3)].ivalue = (bool)(frame->r[*(unsigned char*)(cursor + 1)].ivalue || frame->r[*(unsigned char*)(cursor + 2)].ivalue);
        cursor += 4;
      }
      break;
      case OP_JMP:
      {
        cursor = code + *(int*)(cursor + 1);
      }
      break;
      case OP_BEQZ:
      {
        if(frame->r[*(unsigned char*)(cursor + 1)].ivalue == 0)
        {
          cursor = code + *(int*)(cursor + 2);
        }
        else
        {
          cursor += 6;
        }
      }
      break;
      default:
      {
        lux_vm_set_error(frame->vm, "Unknown opcode");
        return false;
      }
    }
  }
  assert(false);
  return false;
}
