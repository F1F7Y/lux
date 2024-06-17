#include "private.h"

#include <stddef.h>
#include <string.h>

#define MINCHUNKMEM 32

//-----------------------------------------------
// Tries to allocate a chunk of 'size' bytes
// Returns NULL on failure
//-----------------------------------------------
void* xalloc(vm_t* vm, unsigned int size)
{
  if(size == 0)
  {
    return NULL;
  }

  xmemchunk_t* l = NULL;
  for(xmemchunk_t* m = vm->freemem; m != NULL; m = m->next)
  {
    if(m->size > size + sizeof(xmemchunk_t) + MINCHUNKMEM) // Avalible size is larger than required, cut into 2
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

//-----------------------------------------------
// Tries to reallocate a chunk to 'size' bytes
// If 'ptr' is NULL and 'size' is not 0 it allocates
// new memory
// If 'ptr' is not NULL and 'size' is 0 it frees
// the pointer
// Returns NULL on failure
//-----------------------------------------------
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

  if(size > p->size)
  {
    // Try to expand or just realloc
    xmemchunk_t* l = NULL;
    for(xmemchunk_t* m = vm->freemem; m != NULL; m = m->next)
    {
      if((xmemchunk_t*)((char*)p + p->size + sizeof(xmemchunk_t)) == m && m->size >= size)
      {
        if(l != NULL)
        {
          l->next = m->next;
        }
        else
        {
          vm->freemem = m->next;
        }

        if(p->size + sizeof(xmemchunk_t) + m->size - size > sizeof(xmemchunk_t) + MINCHUNKMEM)
        {
          xmemchunk_t* n = (xmemchunk_t*)((char*)p + sizeof(xmemchunk_t) + size);
          n->size = p->size + m->size - size;
          n->next = vm->freemem;
          vm->freemem = n;
          p->size = size;
        }
        else
        {
          p->size += sizeof(xmemchunk_t) + m->size;
        }
        return ptr;
      }
      l = m;
    }

    void* nptr = xalloc(vm, size);
    if(nptr == NULL)
    {
      return NULL;
    }
    memcpy(nptr, ptr, p->size > size ? size : p->size);
    xfree(vm, ptr);
    return nptr;
  }
  else if(size < p->size)
  {
    if(p->size - size > sizeof(xmemchunk_t) + MINCHUNKMEM)
    {
      xmemchunk_t* n = (xmemchunk_t*)((char*)p + sizeof(xmemchunk_t) + size);
      n->next = NULL;
      n->size = p->size - size - sizeof(xmemchunk_t);
      xfree(vm, (void*)((char*)n + sizeof(xmemchunk_t)));
      p->size = size;
    }
    return ptr;
  }
  else
  {
    return ptr;
  }

  return NULL;
}

//-----------------------------------------------
// Frees 'ptr'
//-----------------------------------------------
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
