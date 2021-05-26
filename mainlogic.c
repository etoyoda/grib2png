#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "gribscan.h"
#include "visual.h"

  void
mkfilename(char *filename, size_t fnlen, const struct grib2secs *gsp)
{
  struct tm time;
  unsigned long param = get_parameter(gsp);
  long ft = get_ftime(gsp);
  long dt = get_duration(gsp);
  time_t itime;
  double vlev = get_vlevel(gsp);
  char vtbuf[32];
  get_reftime(&time, gsp);
  itime = timegm(&time);
  itime += (ft + dt) * 60;
  showtime(vtbuf, sizeof vtbuf, gmtime(&itime));
  snprintf(filename, fnlen, "v%s_f%03lu_%s_%s.png", vtbuf, (ft+dt)/60,
    level_name(vlev), param_name(param));
  printf("# %s\n", filename);
}

typedef struct outframe_t {
  unsigned z; // zoom level
  unsigned xa; // min x axis
  unsigned xz; // max x axis
  unsigned ya; // min x axis
  unsigned yz; // max x axis
} outframe_t;

  double
rad2deg(double x)
{
  return 180.0 * x * M_1_PI;
}

  double
interpol(const double *dbuf, const bounding_t *bp, double lat, double lon)
{
  double ri, rj, fi, fj, weight[4];
  size_t ijofs[4];
  int ceil_ri, floor_ri;
  if (lon < bp->w) { lon += 360.0; } 
  if (lat > bp->n || lat < bp->s) { return nan(""); }
  ri = (lon - bp->w) * (bp->ni - 1) / (bp->e - bp->w);
  if ((lon > bp->e) && !bp->wraplon) { return nan(""); }
  rj = (bp->n - lat) * (bp->nj - 1) / (bp->n - bp->s);
  fi = ri - floor(ri);
  fj = rj - floor(rj);
  // これは起こらないとはおもうんだけど
  if ((floor_ri = floor(ri)) > (bp->ni - 1)) { floor_ri = bp->ni - 1; }
  if ((ceil_ri = ceil(ri)) > (bp->ni - 1)) { ceil_ri = 0; }
  ijofs[0] = floor_ri + floor(rj) * bp->ni;
  weight[0] = 1.0 - hypot(fi, fj);
  if (weight[0] < 0.0) { weight[0] = 0.0; }
  ijofs[1] =  ceil_ri + floor(rj) * bp->ni;
  weight[1] = 1.0 - hypot(1 - fi, fj);
  if (weight[1] < 0.0) { weight[1] = 0.0; }
  ijofs[2] = floor_ri +  ceil(rj) * bp->ni;
  weight[2] = 1.0 - hypot(fi, 1 - fj);
  if (weight[2] < 0.0) { weight[2] = 0.0; }
  ijofs[3] =  ceil_ri +  ceil(rj) * bp->ni;
  weight[3] = 1.0 - hypot(1 - fi, 1 - fj);
  if (weight[3] < 0.0) { weight[3] = 0.0; }
  return (dbuf[ijofs[0]] * weight[0] + dbuf[ijofs[1]] * weight[1]
    + dbuf[ijofs[2]] * weight[2] + dbuf[ijofs[3]] * weight[3])
    / (weight[0] + weight[1] + weight[2] + weight[3]);
}

  // 特定パラメタの場合十進尺度 scale_d を補正する
  // 海面気圧: 0.1 hPa 単位に変換
  // 渦度または発散: 1e-6/s 単位に変換
  // 気温または露点: 0.1 K 単位に変換
  // 積算降水量: 0.1 mm 単位に変換
  void
adjust_scales(iparm_t param, int *scale_e, int *scale_d)
{
  switch (param) {
case IPARM_Pmsl:
    *scale_d += 1;
    break;
case IPARM_rDIV:
case IPARM_rVOR:
    *scale_d -= 6;
    break;
case IPARM_T:
case IPARM_dT:
case IPARM_RAIN:
    *scale_d -= 1;
    break;
default:
    break;
  }
}

  gribscan_err_t
reproject(double *gbuf, const bounding_t *bp, const double *dbuf,
  const outframe_t *ofp)
{
  for (unsigned j = ofp->ya; j <= ofp->yz; j++) {
    double lat = asin(tanh(
      (1.0 - ldexp((int)j + 0.5, -7 - (int)ofp->z)) * M_PI
    ));
    for (unsigned i = ofp->xa; i <= ofp->xz; i++) {
      double lon = 2 * M_PI * (ldexp((int)i + 0.5, -8 - (int)ofp->z) - 0.5);
      size_t ijofs = (ofp->xz - ofp->xa + 1) * (j - ofp->ya) + (i - ofp->xa);
      gbuf[ijofs] = interpol(dbuf, bp, rad2deg(lat), rad2deg(lon));
    }
  }
  return GSE_OKAY;
}

// GPVデータから投影法パラメタを抽出して再投影する
  gribscan_err_t
project_ds(const struct grib2secs *gsp, double *dbuf)
{
  gribscan_err_t r;
  bounding_t b;
  outframe_t of = { 2, 0, 1023, 0, 1023 };
  double *gbuf;
  size_t onx, ony;
  char filename[256];
  char *textv[] = {
    "https://github.com/etoyoda/grib2png",
    "https://www.wis-jma.go.jp/",
    NULL };
  onx = of.xz - of.xa + 1;
  ony = of.yz - of.ya + 1;
  r = decode_gds(gsp, &b);
  gbuf = malloc(sizeof(double) * onx * ony);
  if (gbuf == NULL) { return ERR_NOMEM; }
  reproject(gbuf, &b, dbuf, &of);
  mkfilename(filename, sizeof filename, gsp);
  r = gridsave(gbuf, onx, ony, filename, textv);
  free(gbuf);
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
  if (ftime + dura > 720) return GSE_SKIP;
  // 要素と面の複合フィルタ
  switch (iparm) {
  case IPARM_T:
  case IPARM_RH:
    if (!(vlev == 50000.0 || vlev == 85000.0 || vlev == 92500.0)) {
      return GSE_SKIP;
    }
    break;
  case IPARM_RAIN:
    if (vlev != 101325.0) return GSE_SKIP;
    break;
  case IPARM_Pmsl:
    if (vlev != 101324.0) return GSE_SKIP;
    break;
  case IPARM_Z:
    if (!(vlev == 50000.0)) return GSE_SKIP;
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
  gribscan_err_t r = ERR_NOINPUT;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      r = GSE_OKAY;
    } else {
      r = grib2scan_by_filename(argv[i]);
      if (r != GSE_OKAY) break;
    }
  }
  return r;
}

  int
main(int argc, const char **argv)
{
  gribscan_err_t r;
  r = argscan(argc, argv);
  if (r == ERR_NOINPUT) {
    fprintf(stderr, "usage: %s data output ...\n", argv[0]);
  } else if (r != GSE_OKAY) {
    fprintf(stderr, "%s: exit(%u)\n", argv[0], r);
  }
  return r;
}
