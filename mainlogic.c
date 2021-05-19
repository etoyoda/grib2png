#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "gribscan.h"

// エラーは 0
  unsigned long
get_npixels(const struct grib2secs *gsp)
{
  if (gsp->drslen == 0)
    return 0;
  return ui4(gsp->drs + 5);
}

  enum gribscan_err_t
convsec7(const struct grib2secs *gsp)
{
  unsigned long npixels;
  double *dbuf;
  if ((npixels = get_npixels(gsp)) == 0) {
    fprintf(stderr, "DRS missing\n");
    return ERR_BADGRIB;
  }
  if ((dbuf = malloc(sizeof(double) * npixels)) == NULL) {
    return ERR_NOMEM;
  }
  //--- 
  //---
  free(dbuf);
  return GSE_OKAY;
}

  enum gribscan_err_t
checksec7(const struct grib2secs *gsp)
{
  struct tm t;
  char sreftime[24];
  unsigned long iparm;
  double vlev;
  long ftime, dura;
  get_reftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
  iparm = get_parameter(gsp);
  ftime = get_ftime(gsp);
  vlev = get_vlevel(gsp);
  dura = get_duration(gsp);
  // 長すぎる予報時間は最初に捨ててしまう
  if (ftime + dura > 720) return GSE_OKAY;
  // 要素と面の複合フィルタ
  switch (iparm) {
  case IPARM_T:
  case IPARM_RH:
    if (!(vlev == 50000.0 || vlev == 85000.0 || vlev == 92500.0)) {
      return GSE_OKAY;
    }
    break;
  case IPARM_RAIN:
    if (vlev != 101325.0) return GSE_OKAY;
    break;
  case IPARM_Pmsl:
    if (vlev != 101324.0) return GSE_OKAY;
    break;
  case IPARM_Z:
    if (!(vlev == 50000.0 || vlev == 85000.0)) return GSE_OKAY;
    break;
  default:
    return GSE_OKAY;
  }
  printf("b%s %6s f%-+5ld d%-+5ld v%-8.1lf\n",
    sreftime, param_name(iparm), ftime, dura, vlev);
  return convsec7(gsp);
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
