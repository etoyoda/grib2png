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
    fprintf(stderr, "#mymalloc %zu %p\n", size, ptr);
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

  void *
myrealloc(void *ptr, size_t size)
{
  void *r = realloc(ptr, size);
  if (mymalloc_verbose) {
    fprintf(stderr, "#myrealloc %zu %p %p\n", size, ptr, r);
  }
  return r;
}

  void
myfree(void *ptr)
{
  if (mymalloc_verbose) {
    fprintf(stderr, "#myfree %p\n", ptr);
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
