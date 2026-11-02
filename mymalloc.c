#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mymalloc.h"

int mymalloc_verbose = 0;
static size_t asize = 0;
static int acount = 0;

  void *
mymalloc(size_t size)
{
  void *ptr;
  ptr = malloc(size);
  asize += size;
  acount++;
  if (mymalloc_verbose) {
    fprintf(stderr, "#mymalloc %d %zu %p\n", acount, size, ptr);
  }
  return ptr;
}

  void *
mydup(const void *ptr, size_t size)
{
  void *r;
  r = mymalloc(size);
  if (r == NULL) return NULL;
  memcpy(r, ptr, size);
  return r;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"

  void *
myrealloc(void *ptr, size_t size)
{
  void *r = realloc(ptr, size);
  if (mymalloc_verbose) {
    fprintf(stderr, "#myrealloc %d %zu %p %p\n", acount, size, ptr, r);
  }
  return r;
}

#pragma GCC diagnostic pop

  void
myfree(void *ptr)
{
  if (mymalloc_verbose) {
    fprintf(stderr, "#myfree %d %p\n", acount, ptr);
  }
  acount--;
  free(ptr);
}

  void
mymemstat(void)
{
  if (mymalloc_verbose) {
    fprintf(stderr, "#mymalloc remaining %d blocks total %zu bytes\n",
      acount, asize);
  }
}
