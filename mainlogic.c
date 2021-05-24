#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "gribscan.h"

typedef struct bounding_t {
  double n, w, s, e;
  double di, dj;
  size_t ni, nj;
} bounding_t;

  gribscan_err_t
decode_gds(const struct grib2secs *gsp, bounding_t *bp)
{
  size_t npixels, gpixels;
  unsigned gsysno, gdt, unit;
  npixels = get_npixels(gsp);
  if (gsp->gdslen == 0) {
    fprintf(stderr, "GDS missing\n");
    return ERR_BADGRIB;
  }
  if ((gsysno = gsp->gds[5]) != 0) {
    fprintf(stderr, "Unsupported GDS#5 %u\n", gsysno);
    return ERR_UNSUPPORTED;
  }
  if ((gpixels = ui4(gsp->gds + 6)) != npixels) {
    fprintf(stderr, "Pixels unmatch DRS %zu != GDS %zu\n", npixels, gpixels);
    return ERR_UNSUPPORTED;
  }
  if ((gdt = ui2(gsp->gds + 12)) != 0) {
    fprintf(stderr, "Unsupported GDT 5.%u\n", gdt);
    return ERR_UNSUPPORTED;
  }
  if ((unit = ui4(gsp->gds + 38)) != 0) {
    fprintf(stderr, "Unsupported unit dividend %u\n", unit);
    return ERR_UNSUPPORTED;
  }
  if ((unit = ui4(gsp->gds + 42)) != 0xFFFFFFFF) {
    fprintf(stderr, "Unsupported unit divisor %u\n", unit);
    return ERR_UNSUPPORTED;
  }
  bp->ni = si4(gsp->gds + 30);
  bp->nj = si4(gsp->gds + 34);
  if (npixels != bp->ni * bp->nj) {
    fprintf(stderr, "Unsupported npixels %zu != Ni %zu * Nj %zu\n", 
      npixels, bp->ni, bp->nj);
    return ERR_UNSUPPORTED;
  }
  bp->n = si4(gsp->gds + 46) / 1.0e6;
  bp->w = si4(gsp->gds + 50) / 1.0e6;
  bp->s = si4(gsp->gds + 55) / 1.0e6;
  bp->e = si4(gsp->gds + 59) / 1.0e6;
  bp->di = si4(gsp->gds + 63) / 1.0e6;
  bp->dj = si4(gsp->gds + 67) / 1.0e6;
  if (abs(abs((bp->e - bp->w) / bp->ni) - abs(bp->di)) > 1.0e-6) {
    fprintf(stderr, "GDS E %g - W %g != Ni %zu * Di %g\n",
      bp->e, bp->w, bp->ni, bp->di);
    return ERR_UNSUPPORTED;
  }
  if (abs(abs((bp->n - bp->s) / bp->nj) - abs(bp->dj)) > 1.0e-6) {
    fprintf(stderr, "GDS N %g - S %g != Nj %zu * Dj %g\n",
      bp->e, bp->w, bp->ni, bp->di);
    return ERR_UNSUPPORTED;
  }
  
  return GSE_OKAY;
}

  gribscan_err_t
project_ds(const struct grib2secs *gsp, double *dbuf)
{
  gribscan_err_t r;
  bounding_t b;
  r = decode_gds(gsp, &b);
  printf("::%p %g %g %g %g %zu %zu\n", gsp->gds, b.n, b.s, b.e, b.w,
    b.ni, b.nj);
  return r;
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
  if (r == GSE_OKAY) {
    r = project_ds(gsp, dbuf);
  }
  //--- end memory commit
  free(dbuf);
  return r;
}

/* 第1節〜第7節のセット gsp について、
 * 必要であれば convsec7() を呼び出す。
 */
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
    if (!(vlev == 50000.0)) return GSE_OKAY;
    break;
  default:
    return GSE_OKAY;
  }
  printf("b%s %6s f%-+5ld d%-+5ld v%-8.1lf\n",
    sreftime, param_name(iparm), ftime, dura, vlev);
  return convsec7(gsp);
}

/* コマンドライン引数 argc, argv を左からチェックして、
 * 要すれば入力ファイルを開く。
 * （いずれコマンドラインオプションを処理する建設予定地）
 * grib2scan_by_filename() から checksec7() が第7節の数だけ呼び出される。
 */
  gribscan_err_t
argscan(int argc, const char **argv)
{
  int i;
  gribscan_err_t r;
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      r = GSE_OKAY;
    } else {
      r = grib2scan_by_filename(argv[i]);
      if (r != GSE_OKAY) break;
    }
  }
  return r;
}
