#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gribscan.h"

/*
 * 4 オクテットでパラメタを次の構造で表現する
 * 0xFF000000: 予約 (暫定ゼロ、ローカルの処理に使用)
 * 0x00FF0000: discipline
 * 0x0000FF00: category
 * 0x000000FF: parameter
 * ~0: エラー
 */
  unsigned long
get_parameter(const struct grib2secs *gsp)
{
  unsigned long r;
  r = gsp->discipline << 16;
  if (gsp->pdslen == 0)
    return ~0;
  switch (ui2(gsp->pds + 7)) {
  case 0:
    r |= ui2(gsp->pds + 9);
    break;
  default:
    return ~0;
  }
  return r;
}

  enum gribscan_err_t
checksec7(const struct grib2secs *gsp)
{
  struct tm t;
  char sreftime[24];
  get_reftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
  printf("%p %s %08lx\n", gsp->ds, sreftime, get_parameter(gsp));
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
