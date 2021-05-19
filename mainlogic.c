#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "gribscan.h"

// エラーは 0
  size_t
get_npixels(const struct grib2secs *gsp)
{
  if (gsp->drslen == 0)
    return 0;
  return ui4(gsp->drs + 5);
}

  gribscan_err_t
decode_ds(const struct grib2secs *gsp, double *dbuf)
{
  size_t npixels;
  unsigned drstempl;
  float refv;
  unsigned char *refv_eqv;
  int scale_e;
  int scale_d;
  unsigned width;
  size_t i;
  double max;
  if ((gsp->drslen == 0) || (gsp->dslen == 0)) {
    fprintf(stderr, "missing DRS %zu DS %zu\n", gsp->drslen, gsp->dslen);
    return ERR_BADGRIB;
  }
  npixels = ui4(gsp->drs + 5);
  drstempl = ui2(gsp->drs + 9);
  if (drstempl != 0) {
    fprintf(stderr, "unsupported DRS template 5.%u\n", drstempl);
    return ERR_BADGRIB;
  }
  refv_eqv = (unsigned char *)&refv;
  refv_eqv[3] = gsp->drs[11];
  refv_eqv[2] = gsp->drs[12];
  refv_eqv[1] = gsp->drs[13];
  refv_eqv[0] = gsp->drs[14];
  scale_e = si2(gsp->drs + 15);
  scale_d = si2(gsp->drs + 17);
  width = gsp->drs[19];
  max = 0; //d
  for (i = 0; i < npixels; i++) {
    dbuf[i] = (refv + ldexp(unpackbits(gsp->ds + 5, width, i), scale_e))
      * pow(10.0, -scale_d);
    if (dbuf[i] > max) max = dbuf[i]; //d
  }
  printf("refv %g e %d d %d w %u max %g\n", refv, scale_e, scale_d, width, max);
  return GSE_OKAY;
}

  gribscan_err_t
convsec7(const struct grib2secs *gsp)
{
  size_t npixels;
  double *dbuf;
  gribscan_err_t r;
  if ((npixels = get_npixels(gsp)) == 0) {
    fprintf(stderr, "DRS missing\n");
    return ERR_BADGRIB;
  }
  //--- begin memory commit
  if ((dbuf = malloc(sizeof(double) * npixels)) == NULL) {
    fprintf(stderr, "malloc failed %zu\n", npixels);
    return ERR_NOMEM;
  }
  r = decode_ds(gsp, dbuf);
  //--- end memory commit
  free(dbuf);
  return r;
}

  gribscan_err_t
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

  gribscan_err_t
argscan(int argc, const char **argv)
{
  int i;
  gribscan_err_t r;
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
