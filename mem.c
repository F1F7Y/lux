#include "private.h"

#include <stddef.h>
#include <string.h> 

void* xalloc(vm_t* vm, unsigned int size)
{
  if(size == 0)
  {
    return NULL;
  }

  xmemchunk_t* l = NULL;
  for(xmemchunk_t* m = vm->freemem; m != NULL; m = m->next)
  {
    if(m->size > size + sizeof(xmemchunk_t)) // Avalible size is larger than required, cut into 2
    {
      xmemchunk_t* n = (xmemchunk_t*)((char*)m + size + sizeof(xmemchunk_t)); 
      n->size = m->size - size - sizeof(xmemchunk_t);
      n->next = m->next;
      if(l == NULL)
      {
        vm->freemem = n;
      }
      else
      {
        l->next = n;
      }
      m->size = size;
      return (void*)((char*)m + sizeof(xmemchunk_t));
    }
    else if(m->size >= size) // Avalible size is the same or slightly larger than reguired, dont cut it
    {
      if(l == NULL)
      {
        vm->freemem = m->next;
      }
      else
      {
        l->next = m->next;
      }
      return (void*)((char*)m + sizeof(xmemchunk_t));
    }

    l = m;
  }

  return NULL;
}

void* xrealloc(vm_t* vm, void* ptr, unsigned int size)
{
  if(ptr == NULL)
  {
    return xalloc(vm, size);
  }

  if(ptr != NULL && size == 0)
  {
    xfree(vm, ptr);
    return NULL;
  }

  xmemchunk_t* p = (xmemchunk_t*)((char*)ptr - sizeof(xmemchunk_t));

  void* nptr = xalloc(vm, size);
  memcpy(nptr, ptr, p->size > size ? size : p->size);
  xfree(vm, ptr);
  return nptr;
#if 0
  // Check if we can simply expand this chunk
  xmemchunk_t* l = NULL;
  for(xmemchunk_t* m = vm->freemem; m != NULL; m = m->next)
  {
    if((xmemchunk_t*)((char*)m + m->size + sizeof(xmemchunk_t)) == p && m->size >= size)
    {
      if(p->size + sizeof(xmemchunk_t) + m->size - size < sizeof(xmemchunk_t)) // Consume the neighbour
      {
        p->size += m->size + sizeof(xmemchunk_t);
        if(l != NULL)
        {
          l->next = m->next;
        }
        else
        {
          vm->freemem = m->next;
        }
      }
      else // Split the neighbour
      {
        
      }
      return ptr;
    }
    l = m;
  }

  // Check if we can simpy expand the chunk otherwise alloc a new one and memcpy
#endif
  return NULL;
}

void xfree(vm_t* vm, void* ptr)
{
  if(ptr == NULL)
  {
    return;
  }

  xmemchunk_t* f = (xmemchunk_t*)((char*)ptr - sizeof(xmemchunk_t));

  bool integrated = false;

  // Check wheter we are to the right of a free chunk
  xmemchunk_t* l1 = NULL;
  for(xmemchunk_t* m = vm->freemem; m != NULL; m = m->next)
  {
    if((xmemchunk_t*)((char*)m + m->size + sizeof(xmemchunk_t)) == f)
    {
      m->size += f->size + sizeof(xmemchunk_t);
      f = m;
      integrated = true;
      break;
    }

    l1 = m;
  }

  // Check whether we are to the left of a free chunk;
  xmemchunk_t* l2 = NULL;
  for(xmemchunk_t* m = vm->freemem; m != NULL; m = m->next)
  {
    if(f == (xmemchunk_t*)((char*)m - f->size - sizeof(xmemchunk_t)))
    {
      f->size += m->size + sizeof(xmemchunk_t);
      
      if(l2 != NULL)
      {
        l2->next = m->next;
      }
      else
      {
        vm->freemem = m->next;
      }

      if(!integrated)
      {
        f->next = vm->freemem;
        vm->freemem = f;
      }

      integrated = true;
      break;
    }

    l2 = m;
  }

  if(integrated)
  {
    return;
  }
  // We werent integrated into an existing entry so append us
  f->next = vm->freemem;
  vm->freemem = f;
}