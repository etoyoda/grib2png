#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include "mymalloc.h"
#include "gribscan.h"
#include "visual.h"
#include "grib2png.h"

unsigned verbose = 0u;
unsigned debug = 0u;

// filename (長さfnlen) に gsp に相当したファイル名を作成格納する
  void
mkfilename(char *filename, size_t fnlen, const struct grib2secs *gsp,
  const char *suffix)
{
  struct tm time;
  unsigned long param = get_parameter(gsp);
  long ft = get_ftime(gsp);
  long dt = get_duration(gsp);
  time_t itime;
  double vlev = get_vlevel(gsp);
  char vtbuf[32];
  if (suffix == NULL) { suffix = ".png"; }
  get_reftime(&time, gsp);
  itime = timegm(&time);
  itime += (ft + dt) * 60;
  showtime(vtbuf, sizeof vtbuf, gmtime(&itime));
  snprintf(filename, fnlen, "v%s_f%03lu_%s_%s%s", vtbuf, (ft+dt)/60,
    level_name(vlev), param_name(param), suffix);
  printf("writing %s\n", filename);
}

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
  // 気温または露点: 0.1 K 単位に変換
  // 積算降水量: 0.1 mm 単位に変換
  void
adjust_scales(iparm_t param, int *scale_e, int *scale_d)
{
  switch (param) {
  // 海面気圧: 0.1 hPa 単位に変換
  // 典型的値域: 9000..10900
  case IPARM_Pmsl:
    *scale_d += 1;
    break;
  // 渦度または発散: 1e-6/s 単位に変換
  // 典型的値域: -1000..1000
  case IPARM_rDIV:
  case IPARM_rVOR:
    *scale_d -= 6;
    break;
  // 気温または露点: 0.1 K 単位に変換
  // 典型的値域: 2500..3200
  case IPARM_T:
  case IPARM_dT:
  // 風速: 0.1 m/s 単位に変換
  // 典型的値域: -1000..1000
  case IPARM_U:
  case IPARM_V:
  // 積算降水量: 0.1 mm 単位に変換
  // 典型的値域: 0..10000
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
#pragma omp parallel for
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
project_ds(const struct grib2secs *gsp, double *dbuf, const outframe_t *ofp,
  char **textv)
{
  bounding_t b;
  double *gbuf;
  char filename[256];
  palette_t pal;
  size_t onx = ofp->xz - ofp->xa + 1;
  size_t ony = ofp->yz - ofp->ya + 1;
  gribscan_err_t r = decode_gds(gsp, &b);
  //--- begin memory section
  gbuf = mymalloc(sizeof(double) * onx * ony);
  if (gbuf == NULL) { return ERR_NOMEM; }
  reproject(gbuf, &b, dbuf, ofp);
  switch (get_parameter(gsp)) {
  case IPARM_Z:    pal = PALETTE_Z;    break;
  case IPARM_RH:   pal = PALETTE_RH;   break;
  case IPARM_T:    pal = PALETTE_T;    break;
  case IPARM_Pmsl: pal = PALETTE_Pmsl; break;
  case IPARM_rVOR: pal = PALETTE_rVOR; break;
  case IPARM_rDIV: pal = PALETTE_rVOR; break;
  case IPARM_VVPa: pal = PALETTE_VVPa; break;
  case IPARM_RAIN:
    if (get_duration(gsp) == 360) {
      pal = PALETTE_RAIN6;
    } else {
      pal = PALETTE_GSI;
    }
    break;
  default:
    pal = PALETTE_GSI;
    break;
  }
  mkfilename(filename, sizeof filename, gsp, NULL);
  r = gridsave(gbuf, onx, ony, pal, filename, textv, NULL);
  myfree(gbuf);
  //--- end memory section
  return r;
}

  gribscan_err_t
convsec7(const struct grib2secs *gsp)
{
  size_t npixels;
  double *dbuf;
  gribscan_err_t r;
  outframe_t outf = { 2, 0, 1023, 0, 1023 };
  char *textv[] = {
    "https://github.com/etoyoda/grib2png",
    "https://www.wis-jma.go.jp/",
    NULL };
  if ((npixels = get_npixels(gsp)) == 0) {
    fprintf(stderr, "DRS missing\n");
    return ERR_BADGRIB;
  }
  iparm_t iparm_gsp = get_parameter(gsp);
  //--- begin memory section
  if ((dbuf = mymalloc(sizeof(double) * npixels)) == NULL) {
    fprintf(stderr, "malloc failed %zu\n", npixels);
    return ERR_NOMEM;
  }
  r = decode_ds(gsp, dbuf, adjust_scales);
  double vlev_gsp = get_vlevel(gsp);
  if (r == GSE_OKAY) {
    if (iparm_gsp == IPARM_RH && vlev_gsp != 700.e2) goto NOSAVE;
    if (iparm_gsp == IPARM_T && vlev_gsp == 925.e2) goto NOSAVE;
    if (iparm_gsp == IPARM_U) goto NOSAVE;
    if (iparm_gsp == IPARM_V) goto NOSAVE;
    r = project_ds(gsp, dbuf, &outf, textv);
NOSAVE: ;
  }
  if (r == GSE_OKAY) {
    r = check_traps(gsp, dbuf, &outf, textv);
  }
  if (r == GSE_OKAY) {
    r = check_sfcanal(gsp, dbuf, &outf, textv);
  }
  //--- end memory section
  myfree(dbuf);
  return r;
}

/* 第1節〜第7節のセット gsp について、
 * 必要であれば convsec7() を呼び出す。
 * この関数は gsp->ds を破棄または保存する。
 */
  gribscan_err_t
checksec7(const struct grib2secs *gsp)
{
  gribscan_err_t r;
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
  if (((ftime + dura) % 1440 == 0) && (vlev == 500.e2 || vlev == 925.e2) &&
  (iparm != IPARM_rVOR)) {
    goto DONT_SKIP;
  }
  if (ftime + dura > 360) goto END_SKIP;
  DONT_SKIP:
  // 解析値も今の所使わないので捨てる
  if (ftime + dura == 0) goto END_SKIP;
  // 要素と面の複合フィルタ
  switch (iparm) {
  case IPARM_T:
    if (!(vlev == 50000.0 || vlev == 85000.0 || vlev == 92500.0
    || vlev == VLEVEL_Z2M
    )) {
      goto END_SKIP;
    }
    break;
  case IPARM_RH:
    if (!(vlev == 70000.0 || vlev == 85000.0 || vlev == 92500.0)) {
      goto END_SKIP;
    }
    break;
  case IPARM_U:
  case IPARM_V:
    if (!(vlev == VLEVEL_Z10M || vlev == 925.e2 || vlev == 850.e2
    || vlev == 500.e2 || vlev == 300.e2 || vlev == 200.e2
    || vlev == 100.e2)) goto END_SKIP;
    break;
  case IPARM_VVPa:
    if (vlev != 70000.0) goto END_SKIP;
    break;
  case IPARM_RAIN:
    if (vlev != 101325.0) goto END_SKIP;
    break;
  case IPARM_Pmsl:
    if (vlev != 101324.0) goto END_SKIP;
    break;
  case IPARM_rDIV:
    if (!(vlev == 250.e2)) goto END_SKIP;
    break;
  case IPARM_rVOR:
    if (!(vlev == 500.e2)) goto END_SKIP;
    break;
  case IPARM_Z:
    if (!(vlev == 200.e2 || vlev == 300.e2
    || vlev == 500.e2 || vlev == 850.e2 || vlev == 925.e2)) goto END_SKIP;
    break;
  case IPARM_CLA:
  case IPARM_CLL:
  case IPARM_CLM:
  case IPARM_CLH:
    break;
  default:
    goto END_SKIP;
  }
  printf("b%s %6s f%-+5ld d%-+5ld v%-8.1lf\n",
    sreftime, param_name(iparm), ftime, dura, vlev);
  r = convsec7(gsp);
  goto END_NORMAL;

END_SKIP:
  r = GSE_SKIP;
END_NORMAL:
  myfree(gsp->ds);
  return r;
}

#define streq(a,b) (0==strcmp((a),(b)))

  int
magic_parameters(const char *arg)
{
  if (streq(arg,"vxmm=")) {
    gmagic_vortex_minmatch = atof(arg+5);
  } else if (streq(arg,"fric=")) {
    gmagic_friction_ratio = atof(arg+5);
  } else {
    return -1;
  }
  return 0;
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
      switch (argv[i][1]) {
      case 't':
        text_fp = fopen(argv[i] + 2, "w");
        break;
      case 'g':
        if (argv[i][2] == 'v') {
          gflg_rvor_with_wd = 1;
        } else if (argv[i][2] == 'j') {
          gflg_jet_lower = 1;
        } else if (argv[i][2] == 'd') {
          debug++;
        } 
        break;
      case 'M':
        if (-1==magic_parameters(argv[i]+2)) {
          goto BADOPT;
        }
        break;
      case 'v':
        verbose++;
        break;
      BADOPT:
      default:
        fprintf(stderr, "%s: unknown option\n", argv[i]);
        break;
      }
    } else {
      r = grib2scan_by_filename(argv[i]);
      if (r != GSE_OKAY) break;
    }
  }
  if (text_fp) { fclose(text_fp); text_fp = NULL; }
  return r;
}

  int
main(int argc, const char **argv)
{
  gribscan_err_t r;
  r = argscan(argc, argv);
  if (r == ERR_NOINPUT) {
    fprintf(stderr, "usage: %s data ...\n", argv[0]);
  } else if (r != GSE_OKAY) {
    fprintf(stderr, "%s: exit(%u)\n", argv[0], r);
  }
  mymemstat();
  return r;
}
