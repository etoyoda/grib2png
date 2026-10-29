#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "gribscan.h"

// NOTE: GRIB IS BIG ENDIAN

// store 32bit unsigned integer onto pointer ptr 
  void
set_ui4(unsigned char *ptr, unsigned long u)
{
  if (u > 0xFFFFFFFFul) u = 0xFFFFFFFFul;
  ptr[0] = (u >> 24) & 0xFFu;
  ptr[1] = (u >> 16) & 0xFFu;
  ptr[2] = (u >> 8) & 0xFFu;
  ptr[3] = u & 0xFFu;
}

// store 32bit signed integer onto pointer ptr 
  void
set_si4(unsigned char *ptr, long i)
{
  unsigned long u;
  if (i > 0x7FFFFFFFul) {
    u = 0xFFFFFFFFul;
  } else if (i >= 0) {
    u = i;
  } else {
    u = -i;
    u |= 0x80000000ul;
  }
  set_ui4(ptr, u);
}

// store 32bit float onto pointer ptr 
  void
set_float32(unsigned char *ptr, float x)
{
  union { float f; uint32_t u; } buf;
  buf.f = x;
  set_ui4(ptr, buf.u);
}
