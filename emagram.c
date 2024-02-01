#include <stdio.h>
#include <stdlib.h>

typedef struct {
  float p;
  float x;
  float y;
} level_t;

typedef struct {
  char name[32];
  size_t ttd_size;
  level_t *ttd;
  size_t uv_size;
  level_t *uv;
} obs_t;

  int
main(int argc, char **argv)
{
  for (int i=1; i<argc; i++) {
   
  }
  return 0;
}
