#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gribscan.h"

  enum gribscan_err_t
checksec7(const struct grib2secs *gsp)
{
  struct tm t;
  char sreftime[24];
  mkreftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
  printf("%p %s\n", gsp->ds, sreftime);
  return GSE_OKAY;
}

  enum gribscan_err_t
argscan(int argc, const char **argv)
{
  int i;
  enum gribscan_err_t r;
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      r = GSE_OKAY;
    } else {
      r = scandata(argv[i]);
      if (r != GSE_OKAY) break;
    }
  }
  return r;
}
