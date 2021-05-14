#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gribscan.h"

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

  enum gribscan_err_t
checksec7(const struct grib2secs *gsp)
{
  printf("%p i%p g%p p%p d%p\n", gsp->ds, gsp->ids, gsp->gds, gsp->pds, gsp->drs);
  return GSE_OKAY;
}
