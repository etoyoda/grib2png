#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

  void *
mymalloc(size_t size)
{
  void *ptr;
  ptr = malloc(size);
  fprintf(stderr, "#mymalloc %zu %p\n", size, ptr);
  return ptr;
}

  void
myfree(void *ptr)
{
  fprintf(stderr, "#myfree %p\n", ptr);
  free(ptr);
}

