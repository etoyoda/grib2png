#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "gribscan.h"

/*
 * 4 オクテットでパラメタを次の構造で表現する
 * 構造:
 * 0xFF000000: 予約 (暫定ゼロ、ローカルの処理に使用)
 * 0x00FF0000: discipline
 * 0x0000FF00: category
 * 0x000000FF: parameter
 * ~0: エラー
 * 用例:
 * 0x00000000: temperature
 * 0x00000101: relative humidity
 * 0x00000108: total precipitation
 * 0x00000202: u component of wind
 * 0x00000203: v component of wind
 * 0x0000020c: relative vorticity
 * 0x0000020d: relative divergence
 * 0x00000301: sea level pressure
 * 0x00000305: geopotential height
 */
  unsigned long
get_parameter(const struct grib2secs *gsp)
{
  unsigned pdst;
  unsigned long r;
  r = gsp->discipline << 16;
  if (gsp->pdslen == 0)
    return ~0;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 8:
    r |= ui2(gsp->pds + 9);
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return ~0;
  }
  return r;
}

  long
get_ftime(const struct grib2secs *gsp)
{
  long r;
  unsigned tunits;
  unsigned pdst;
  if (gsp->pdslen == 0)
    return LONG_MAX;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 8:
    tunits = gsp->pds[17];
    r = ui4(gsp->pds + 18);
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return LONG_MAX;
  }
  switch (tunits) {
  case 0: break;
  case 1: r *= 60; break;
  case 2: r *= (60 * 24); break;
  case 10: r *= (3 * 60); break;
  case 11: r *= (6 * 60); break;
  case 12: r *= (12 * 60); break;
  case 13: r /= 60; break;
  default:
    return LONG_MAX;
  }
  return r;
}

  long
get_duration(const struct grib2secs *gsp)
{
  long r;
  unsigned tunits;
  unsigned pdst;
  unsigned nranges;
  if (gsp->pdslen == 0)
    return LONG_MAX;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
    return 0;
  case 8:
    nranges = gsp->pds[41];
    tunits = gsp->pds[48];
    r = ui4(gsp->pds + 49);
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return LONG_MAX;
  }
  if (nranges != 1) {
    fprintf(stderr, "unsupported n time ranges %u > 1\n", nranges);
    return LONG_MAX;
  }
  switch (tunits) {
  case 0: break;
  case 1: r *= 60; break;
  case 2: r *= (60 * 24); break;
  case 10: r *= (3 * 60); break;
  case 11: r *= (6 * 60); break;
  case 12: r *= (12 * 60); break;
  case 13: r /= 60; break;
  default:
    return LONG_MAX;
  }
  return r;
}

  double
float40(const unsigned char *ptr)
{
  if (ptr[0] == 255) {
    return nan("");
  } else if ((0x80 & ptr[0]) != 0) {
    return pow(10.0, ptr[0] & 0x7F) * ui4(ptr + 1);
  } else {
    return pow(10.0, -ptr[0]) * ui4(ptr + 1);
  }
}

  double
get_vlevel(const struct grib2secs *gsp)
{
  double r;
  unsigned pdst, vtype;
  const unsigned char *vlevptr;
  if (gsp->pdslen == 0)
    return LONG_MAX;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 8:
    vtype = gsp->pds[22];
    vlevptr = gsp->pds + 23;
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return nan("");
    break;
  }
  switch (vtype) {
  case 1:
    r = 101325.0;
    break;
  case 100:
    r = float40(vlevptr);
    break;
  case 101:
    r = 101324.0;
    break;
  case 103:
    r = 101320.0 - float40(vlevptr);
    break;
  default:
    fprintf(stderr, "unsupported vertical level type %u\n", vtype);
    return nan("");
    break;
  }
  return r;
}

  const char *
param_name(unsigned iparm)
{
  static char buf[32];
  switch (iparm) {
  case 0x000000: return "T";
  case 0x000006: return "dT";
  case 0x000101: return "RH";
  case 0x000107: return "RR1H";
  case 0x000108: return "RAIN";
  case 0x000202: return "U";
  case 0x000203: return "V";
  case 0x000204: return "PSI";
  case 0x000205: return "CHI";
  case 0x000208: return "VVPa";
  case 0x00020c: return "rVOR";
  case 0x00020d: return "rDIV";
  case 0x000300: return "Pres";
  case 0x000301: return "Pmsl";
  case 0x000305: return "Z";
  default:
    sprintf(buf, "p%02u-%03u-%03u-%03u",
      iparm >> 24, (iparm >> 16) & 0xFF,
      (iparm >> 8) & 0xFF, iparm & 0xFF);
    return buf;
  }
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
//  if (ftime <= 720 && vlev >= 100000.0) {
    printf("b%s %6s f%-+5ld d%-+5ld v%-8.1lf\n",
      sreftime, param_name(iparm), ftime, dura, vlev);
//  }
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
