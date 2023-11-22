#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include "gribscan.h"
#include "visual.h"
#include "mymalloc.h" // only for mymemstat();

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
  gbuf = malloc(sizeof(double) * onx * ony);
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
  r = gridsave(gbuf, onx, ony, pal, filename, textv);
  free(gbuf);
  //--- end memory section
  return r;
}

  double
windspeed(double u, double v, double p)
{
  return hypot(u, v);
}

  double
ept_bolton(double t, double rh, double p)
{
  t *= 0.1;
  p *= 0.01;
  double es = 6.112 * exp(17.67 * (t - 273.15) / (t - 29.65));
  double e = es * rh * 0.01;
  double ke = log(e/6.112) / 17.67;
  double td = 0.05 * (-5463.0 + 593.0 * ke) / (-1.0 + ke);
  double tlcl = 1.0 / (1.0 / (td - 56.0) + log(t / td) / 800.0) + 56.0;
  double x = 0.622 * e / (p - e);
  double thdl = t * pow(1.0e3 / (p - e), 0.2854) * pow(t / tlcl, 0.28 * x);
  double ept = thdl * exp((3036.0 / tlcl - 1.78) * x * (1.0 + 0.448 * x));
  return ept * 10.0;
}

  gribscan_err_t
project_binop(const grib2secs_t *gsp_rh, double *dbuf_rh,
  grib2secs_t *gsp_t, double *dbuf_t, const outframe_t *ofp, char **textv,
  iparm_t iparm, palette_t pal,
  double (*element_conv)(double t, double rh, double p))
{
  // dbuf と同じ（GRIB格子の）配列長
  size_t npixels = get_npixels(gsp_t);
  //--- begin memory section 1
  double *dbuf_ept = malloc(sizeof(double) * npixels);
  if (dbuf_ept == NULL) return ERR_NOMEM;
  for (size_t i = 0; i < npixels; i++) {
    dbuf_ept[i] = element_conv(dbuf_t[i], dbuf_rh[i], get_vlevel(gsp_t));
  }
  // 出力格子に補間
  bounding_t b;
  double *gbuf;
  char filename[256];
  size_t onx = ofp->xz - ofp->xa + 1;
  size_t ony = ofp->yz - ofp->ya + 1;
  gribscan_err_t r = decode_gds(gsp_t, &b);
  //--- begin memory section 2
  gbuf = malloc(sizeof(double) * onx * ony);
  if (gbuf == NULL) { free(dbuf_ept); return ERR_NOMEM; }
  reproject(gbuf, &b, dbuf_ept, ofp);
  set_parameter(gsp_t, iparm);
  mkfilename(filename, sizeof filename, gsp_t, NULL);
  r = gridsave(gbuf, onx, ony, pal, filename, textv);
  free(gbuf);
  //--- end memory section 2
  free(dbuf_ept);
  //--- end memory section 1
  return r;
}

double
wdir(double u, double v)
{
  return atan2(v, u) / M_PI * 180.0 + 180.0;
}

// 風向だけは成分毎に投影してから算出する
  gribscan_err_t
project_winddir(const grib2secs_t *gsp_u, double *dbuf_u,
  grib2secs_t *gsp_v, double *dbuf_v, const outframe_t *ofp, char **textv)
{
  bounding_t b;
  double *ubuf, *vbuf, *dbuf;
  char filename[256];
  size_t onx = ofp->xz - ofp->xa + 1;
  size_t ony = ofp->yz - ofp->ya + 1;
  gribscan_err_t r = decode_gds(gsp_v, &b);
  //--- begin memory section
  ubuf = malloc(sizeof(double) * onx * ony * 3);
  if (ubuf == NULL) { return ERR_NOMEM; }
  vbuf = ubuf + onx * ony;
  dbuf = vbuf + onx * ony;
  reproject(ubuf, &b, dbuf_u, ofp);
  reproject(vbuf, &b, dbuf_v, ofp);
  for (size_t ij = 0; ij < onx*ony; ij++) {
    dbuf[ij] = (hypot(ubuf[ij], vbuf[ij]) > 33.0) ?
      wdir(ubuf[ij], vbuf[ij]) : nan("");
  }
  set_parameter(gsp_v, IPARM_WD);
  mkfilename(filename, sizeof filename, gsp_v, NULL);
  r = gridsave(dbuf, onx, ony, PALETTE_Z, filename, textv);
  free(ubuf);
  //--- end memory section
  return r;
}

static FILE *text_fp = NULL;

  void
textout_winds(const grib2secs_t *gsp_u, double *dbuf_u,
  grib2secs_t *gsp_v, double *dbuf_v)
{
  bounding_t b;
  struct tm time;
  char ftvl[256];
  if (text_fp == NULL) return;
  get_reftime(&time, gsp_u);
  time_t itime = timegm(&time);
  itime += get_ftime(gsp_u) * 60;
  strftime(ftvl, sizeof ftvl, "%Y-%m-%dT%H:%MZ", gmtime(&itime));
  double vlev = get_vlevel(gsp_u);
  if (vlev == VLEVEL_Z10M) {
    strncat(ftvl, "/sfc", sizeof ftvl - strlen("/sfc") - 1);
  } else {
    char *endp = ftvl + strlen(ftvl);
    enum { SMARGIN = 10 };
    snprintf(endp, sizeof ftvl - SMARGIN, "/p%u", (unsigned)(vlev / 100.0));
  }
  decode_gds(gsp_v, &b);
  enum { ISTEP = 8, JSTEP = 8, JMARGIN = 16 };
  for (unsigned j = JMARGIN; j <= (b.nj - JMARGIN); j += JSTEP) {
    double lat = b.n + (b.s - b.n) * j / (b.nj - 1);
    for (unsigned i = 0; i < b.ni; i += ISTEP) {
      double lon = b.w + (b.e - b.w) * i / (b.ni - 1);
      unsigned ij = i + j * b.ni;
      double wsp = hypot(dbuf_u[ij], dbuf_v[ij]) * 0.1;
      double wd = wdir(dbuf_u[ij], dbuf_v[ij]);
      if (lon > 180.0) { lon -= 360.0; }
      fprintf(text_fp,
        "%s/gpv%+d%+d {\"@\":\"gpv%+d%+d\","
        "\"La\":%3.1f,\"Lo\":%3.1f,"
        "\"d\":%3.0f,\"f\":%3.0f,\"ahl\":\"GSM\"}\n",
        ftvl, (int)lat, (int)lon, (int)lat, (int)lon, lat, lon, wd, wsp);
    }
  }
}

  gribscan_err_t
project_winds(const grib2secs_t *gsp_u, double *dbuf_u,
  grib2secs_t *gsp_v, double *dbuf_v, const outframe_t *ofp, char **textv)
{
  textout_winds(gsp_u, dbuf_u, gsp_v, dbuf_v);
  palette_t pal = (get_vlevel(gsp_u) >= 850.e2)
    ? PALETTE_WINDS_SFC : PALETTE_WINDS; 
  if (get_vlevel(gsp_u) == VLEVEL_Z10M) {
    gribscan_err_t r;
    r = project_winddir(gsp_u, dbuf_u, gsp_v, dbuf_v, ofp, textv);
    if (r != GSE_OKAY) { return r; }
  }
  return project_binop(gsp_u, dbuf_u, gsp_v, dbuf_v, ofp, textv,
    IPARM_WINDS, pal, windspeed);
}

  gribscan_err_t
project_ept(const grib2secs_t *gsp_rh, double *dbuf_rh,
  grib2secs_t *gsp_t, double *dbuf_t, const outframe_t *ofp, char **textv)
{
  return project_binop(gsp_rh, dbuf_rh, gsp_t, dbuf_t, ofp, textv,
    IPARM_papT, PALETTE_papT, ept_bolton);
}

typedef struct trap_t {
  // 先に保存せねばならないデータ
  grib2secs_t *keep_gsp; // 変数
  iparm_t keep_parm;
  double keep_vlev;
  long keep_ftime2;
  // あとで来るはずのデータ
  iparm_t wait_parm;
  double wait_vlev;
  long wait_ftime2;
  // 両方揃ったらこの関数を呼ぶ
  enum gribscan_err_t
    (*projecter)(const grib2secs_t *gsp_wait, double *dbuf_wait,
      grib2secs_t *gsp_keep, double *dbuf_keep,
      const outframe_t *ofp, char **textv);
} trap_t;

enum { N_TRAPS = 9 };
static trap_t traps[N_TRAPS] = {
  { NULL, IPARM_T, 925.e2, 360L, IPARM_RH, 925.e2, 360L, project_ept },
  { NULL, IPARM_T, 850.e2, 360L, IPARM_RH, 850.e2, 360L, project_ept },
  { NULL, IPARM_U, VLEVEL_Z10M, 360L, IPARM_V, VLEVEL_Z10M, 360L, project_winds },
  { NULL, IPARM_U, 925.e2, 360L, IPARM_V, 925.e2, 360L, project_winds },
  { NULL, IPARM_U, 850.e2, 360L, IPARM_V, 850.e2, 360L, project_winds },
  { NULL, IPARM_U, 500.e2, 360L, IPARM_V, 500.e2, 360L, project_winds },
  { NULL, IPARM_U, 300.e2, 360L, IPARM_V, 300.e2, 360L, project_winds },
  { NULL, IPARM_U, 200.e2, 360L, IPARM_V, 200.e2, 360L, project_winds },
  { NULL, IPARM_U, 100.e2, 360L, IPARM_V, 100.e2, 360L, project_winds }
};

  gribscan_err_t
check_traps(const struct grib2secs *gsp, double *dbuf,
  outframe_t *ofp, char **textv)
{
  size_t npixels = get_npixels(gsp);
  iparm_t iparm_gsp = get_parameter(gsp);
  double vlev_gsp = get_vlevel(gsp);
  long ftime2_gsp = get_ftime(gsp) + get_duration(gsp);
  gribscan_err_t r = GSE_OKAY;
  for (int i = 0; i < N_TRAPS; i++) {
    if (iparm_gsp == traps[i].keep_parm && vlev_gsp == traps[i].keep_vlev
    && ftime2_gsp == traps[i].keep_ftime2) {
      if (traps[i].keep_gsp) {
        if (traps[i].keep_gsp->omake) myfree(traps[i].keep_gsp->omake);
        del_grib2secs(traps[i].keep_gsp);
      }
      traps[i].keep_gsp = dup_grib2secs(gsp);
      traps[i].keep_gsp->omake = mydup(dbuf, sizeof(double) * npixels);
    }
    if (traps[i].keep_gsp && iparm_gsp == traps[i].wait_parm
    && vlev_gsp == traps[i].wait_vlev
    && ftime2_gsp == traps[i].wait_ftime2) {
      traps[i].projecter(gsp, dbuf, traps[i].keep_gsp,
        traps[i].keep_gsp->omake, ofp, textv);
      myfree(traps[i].keep_gsp->omake);
      del_grib2secs(traps[i].keep_gsp);
      traps[i].keep_gsp = NULL;
    }
  }
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
  if ((dbuf = malloc(sizeof(double) * npixels)) == NULL) {
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
  //--- end memory section
  free(dbuf);
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
  if (ftime + dura > 360) goto END_SKIP;
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
