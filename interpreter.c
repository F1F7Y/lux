#include "private.h"

#include <stdio.h>

bool lux_vm_interpret_frame(vm_t* vm, vmframe_t* frame)
{
  char* code = frame->closure->code;
  char* cursor = code;
  char* code_end = code + frame->closure->used;

  while(cursor < code_end)
  {
    switch(*cursor)
    {
      case OP_NOP:
      {
        continue;
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
        lux_vm_call_function(vm, closure);
        cursor += 2;
      }
      break;
      case OP_RET:
      {
        printf("Function %s returning %d\n", frame->closure->name, frame->r[0]);
        return true;
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