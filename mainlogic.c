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
  double (*element_conv)(double t, double rh, double p),
  double *ubuf)
{
  // dbuf と同じ（GRIB格子の）配列長
  size_t npixels = get_npixels(gsp_t);
  //--- begin memory section 1
  double *dbuf_ept = mymalloc(sizeof(double) * npixels);
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
  gbuf = mymalloc(sizeof(double) * onx * ony);
  if (gbuf == NULL) { myfree(dbuf_ept); return ERR_NOMEM; }
  reproject(gbuf, &b, dbuf_ept, ofp);
  set_parameter(gsp_t, iparm);
  mkfilename(filename, sizeof filename, gsp_t, NULL);
  r = gridsave(gbuf, onx, ony, pal, filename, textv, ubuf);
  myfree(gbuf);
  //--- end memory section 2
  myfree(dbuf_ept);
  //--- end memory section 1
  return r;
}

double
wdir(double u, double v)
{
  return atan2(v, u) / M_PI * 180.0 + 180.0;
}

// smoothes any two-dimensional array src into dest
  void
smooth49(double *dest, const double *src, size_t nx, size_t ny)
{
  memcpy(dest, src, nx*sizeof(double)*4);
#pragma omp parallel for
  for (size_t j = 3; j < (ny-3); j++) {
    dest[0+j*nx] = src[0+j*nx];
    dest[1+j*nx] = src[1+j*nx];
    dest[2+j*nx] = src[2+j*nx];
    for (size_t i = 3; i < (nx-3); i++) {
      double sum;
      sum = 0.0;
      for (size_t jc = j-3; jc <= j+3; jc++) {
        for (size_t ic = i-3; ic <= i+3; ic++) {
          sum += src[ic+jc*nx];
        }
      }
      dest[i+j*nx] = sum / 49.0;
    }
    dest[(nx-3)+j*nx] = src[(nx-3)+j*nx];
    dest[(nx-2)+j*nx] = src[(nx-2)+j*nx];
    dest[(nx-1)+j*nx] = src[(nx-1)+j*nx];
  }
  memcpy(dest+(ny-3)*nx, src+(ny-3)*nx, nx*sizeof(double)*3);
}

// コマンドラインオプション -gv で風向と一緒に渦度を算出
int gflg_rvor_with_wd = 0;

// 風向だけは成分毎に投影してから算出する
  gribscan_err_t
project_winddir(const grib2secs_t *gsp_u, double *dbuf_u,
  grib2secs_t *gsp_v, double *dbuf_v, const outframe_t *ofp, char **textv,
  double **ubufptr)
{
  bounding_t b;
  gribscan_err_t r;
  double *ubuf, *vbuf, *dbuf;
  char filename[256];
  size_t onx = ofp->xz - ofp->xa + 1;
  size_t ony = ofp->yz - ofp->ya + 1;
  r = decode_gds(gsp_v, &b);
  if (r != GSE_OKAY) { return r; }
  ubuf = mymalloc(sizeof(double) * onx * ony * 3);
  if (ubuf == NULL) { return ERR_NOMEM; }
  vbuf = ubuf + onx * ony;
  dbuf = vbuf + onx * ony;
  reproject(dbuf, &b, dbuf_u, ofp);
  switch (get_parameter(gsp_u)) {
  case IPARM_U:
    smooth49(ubuf, dbuf, onx, ony);
    break;
  case IPARM_V: default:
    smooth49(vbuf, dbuf, onx, ony);
    break;
  }
  reproject(dbuf, &b, dbuf_v, ofp);
  switch (get_parameter(gsp_v)) {
  case IPARM_U:
    smooth49(ubuf, dbuf, onx, ony);
    break;
  case IPARM_V: default:
    smooth49(vbuf, dbuf, onx, ony);
    break;
  }
#pragma omp parallel for
  for (size_t ij = 0; ij < onx*ony; ij++) {
    dbuf[ij] = wdir(ubuf[ij], vbuf[ij]);
  }
  set_parameter(gsp_v, IPARM_WD);
  mkfilename(filename, sizeof filename, gsp_v, NULL);
  r = gridsave(dbuf, onx, ony, PALETTE_WD, filename, textv, NULL);
  if (r != GSE_OKAY) goto QUIT;
  // 渦度を算出描画
  if (gflg_rvor_with_wd) {
    for (size_t i = 0; i < onx; i++) {
      dbuf[i+0*onx] = dbuf[i+(ony-1)*onx] = 0.0;
    }
    for (size_t j = 1; j < (ony-1); j++) {
      double spacing = onx / 40.0 * 0.1;
      dbuf[0+j*onx] = dbuf[(onx-1)+j*onx] = 0.0;
      for (size_t i = 1; i < (onx-1); i++) {
        dbuf[i+j*onx] = spacing * (
          -(ubuf[i+(j+1)*onx] - ubuf[i+(j-1)*onx])
          +(vbuf[(i+1)+j*onx] - vbuf[(i-1)+j*onx])
        );
      }
    }
    set_parameter(gsp_v, IPARM_rVOR);
    mkfilename(filename, sizeof filename, gsp_v, NULL);
    r = gridsave(dbuf, onx, ony, PALETTE_rVOR, filename, textv, NULL);
  }
QUIT:
  *ubufptr = ubuf;
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

// 強風軸を描く: -gj オプション
int gflg_jet_lower = 0;

  gribscan_err_t
project_winds(const grib2secs_t *gsp_u, double *dbuf_u,
  grib2secs_t *gsp_v, double *dbuf_v, const outframe_t *ofp, char **textv)
{
  gribscan_err_t r;
  double *ubuf;
  double vlev;
  ubuf = NULL;
  textout_winds(gsp_u, dbuf_u, gsp_v, dbuf_v);
  vlev = get_vlevel(gsp_u);
  palette_t pal = (vlev >= 850.e2) ? PALETTE_WINDS_SFC : PALETTE_WINDS; 
  r = project_winddir(gsp_u, dbuf_u, gsp_v, dbuf_v, ofp, textv, &ubuf);
  if (r != GSE_OKAY) { return r; }
  r = project_binop(gsp_u, dbuf_u, gsp_v, dbuf_v, ofp, textv,
    IPARM_WINDS, pal, windspeed, ubuf);
  if (ubuf) { myfree(ubuf); }
  return r;
}

  gribscan_err_t
project_ept(const grib2secs_t *gsp_rh, double *dbuf_rh,
  grib2secs_t *gsp_t, double *dbuf_t, const outframe_t *ofp, char **textv)
{
  return project_binop(gsp_rh, dbuf_rh, gsp_t, dbuf_t, ofp, textv,
    IPARM_papT, PALETTE_papT, ept_bolton, NULL);
}

/*=== traps - 2個データを受けて処理する関数を起動する機構 ===*/

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

enum { N_TRAPS = 19 };
static trap_t traps[N_TRAPS] = {
  { NULL, IPARM_T, 925.e2, 360L, IPARM_RH, 925.e2, 360L, project_ept },
  { NULL, IPARM_T, 850.e2, 360L, IPARM_RH, 850.e2, 360L, project_ept },
  { NULL, IPARM_U, VLEVEL_Z10M, 360L, IPARM_V, VLEVEL_Z10M, 360L, project_winds },
  { NULL, IPARM_U, 925.e2, 360L, IPARM_V, 925.e2, 360L, project_winds },
  { NULL, IPARM_U, 850.e2, 360L, IPARM_V, 850.e2, 360L, project_winds },
  { NULL, IPARM_U, 500.e2, 360L, IPARM_V, 500.e2, 360L, project_winds },
  { NULL, IPARM_U, 300.e2, 360L, IPARM_V, 300.e2, 360L, project_winds },
  { NULL, IPARM_U, 200.e2, 360L, IPARM_V, 200.e2, 360L, project_winds },
  { NULL, IPARM_U, 100.e2, 360L, IPARM_V, 100.e2, 360L, project_winds },
  { NULL, IPARM_U, 925.e2, 1440L, IPARM_V, 925.e2, 1440L, project_winds },
  { NULL, IPARM_U, 925.e2, 2880L, IPARM_V, 925.e2, 2880L, project_winds },
  { NULL, IPARM_U, 925.e2, 4320L, IPARM_V, 925.e2, 4320L, project_winds },
  { NULL, IPARM_U, 925.e2, 5760L, IPARM_V, 925.e2, 5760L, project_winds },
  { NULL, IPARM_U, 925.e2, 7200L, IPARM_V, 925.e2, 7200L, project_winds },
  { NULL, IPARM_T, 925.e2, 1440L, IPARM_RH, 925.e2, 1440L, project_ept },
  { NULL, IPARM_T, 925.e2, 2880L, IPARM_RH, 925.e2, 2880L, project_ept },
  { NULL, IPARM_T, 925.e2, 4320L, IPARM_RH, 925.e2, 4320L, project_ept },
  { NULL, IPARM_T, 925.e2, 5760L, IPARM_RH, 925.e2, 5760L, project_ept },
  { NULL, IPARM_T, 925.e2, 7200L, IPARM_RH, 925.e2, 7200L, project_ept }
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
    // traps[i] のkeep条件に合致するなら gsp を複製して保存する
    if (iparm_gsp == traps[i].keep_parm && vlev_gsp == traps[i].keep_vlev
    && ftime2_gsp == traps[i].keep_ftime2) {
      // ありえないが一応：既に keep されているなら破棄
      if (traps[i].keep_gsp) {
        if (traps[i].keep_gsp->omake) myfree(traps[i].keep_gsp->omake);
        del_grib2secs(traps[i].keep_gsp);
      }
      traps[i].keep_gsp = dup_grib2secs(gsp);
      traps[i].keep_gsp->omake = mydup(dbuf, sizeof(double) * npixels);
    }
    // traps[i] のkeepが既にあって wait 条件に合致するなら関数を駆動してから
    // keep を破棄
    if (traps[i].keep_gsp && iparm_gsp == traps[i].wait_parm
    && vlev_gsp == traps[i].wait_vlev
    && ftime2_gsp == traps[i].wait_ftime2) {
      r = traps[i].projecter(gsp, dbuf, traps[i].keep_gsp,
        traps[i].keep_gsp->omake, ofp, textv);
      myfree(traps[i].keep_gsp->omake);
      del_grib2secs(traps[i].keep_gsp);
      traps[i].keep_gsp = NULL;
      if (r != GSE_OKAY) break;
    }
  }
  return r;
}

typedef struct sfctrap_t {
  long ftime2;
  grib2secs_t *gsp_pmsl;
  grib2secs_t *gsp_u;
  grib2secs_t *gsp_v;
} sfctrap_t;

enum { N_SFCTRAPS = 1 };
sfctrap_t sfctrap[N_SFCTRAPS] = {
  { 360, NULL, NULL, NULL }
};

  double
cosdeg(double deg)
{
  return cos(deg * M_PI / 180.0);
}

  double
sindeg(double deg)
{
  return sin(deg * M_PI / 180.0);
}

  gribscan_err_t
sfcanal(struct sfctrap_t *strap, outframe_t *ofp, char **textv)
{
  size_t npixels = get_npixels(strap->gsp_u);
  bounding_t b;
  gribscan_err_t r;
  r = decode_gds(strap->gsp_u, &b);
  if (r != GSE_OKAY) { return r; }
  if (npixels != b.ni*b.nj) {
    fprintf(stderr, "npixels %zu != GDS %zu * %zu %zu\n",
    npixels, b.ni, b.nj, b.ni*b.nj);
    return ERR_BADGRIB;
  }
  // 東西端の処理が変わるので
  if (b.wraplon == 0) {
    fprintf(stderr, "regional data\n");
    return ERR_BADGRIB;
  }
puts("@@@");
  // 水平差分計算。南北端では南北微分をゼロにする
  double *u = strap->gsp_u->omake;
  double *v = strap->gsp_v->omake;
  double *rhs = mymalloc(sizeof(double)*npixels);
  double *p = mymalloc(sizeof(double)*npixels);
  double *cor = mymalloc(sizeof(double)*npixels);
  if ((rhs==NULL)||(p==NULL)||(cor==NULL)) { return ERR_NOMEM; }
  double *pmsl = strap->gsp_pmsl->omake;
  memcpy(p, pmsl, sizeof(double)*npixels);
  // degree_lat = 6371.e3 * (M_PI/180.0)
  double invdeg = 180.0/(M_PI*6371.e3);
  double rho = 1.0e-3;

  for (size_t j=1; j<(b.nj-1); j++) {
    size_t jp1 = j+1;
    size_t jm1 = j-1;
    double invdy = -invdeg/b.dj;
    double invdx = invdeg/(b.di*cosdeg(bp_lat(&b,j)));
    double f = 4.0*M_PI/86400.0*sindeg(bp_lat(&b,j));
    // need map factor
    for (size_t i=0; i<b.ni; i++) {
      size_t ip1 = (i+1)%b.ni;
      size_t im1 = (i+b.ni-1)%b.ni;
      double rhofzeta = rho * f * (
        (u[i+jp1*b.ni] - u[i+jm1*b.ni]) * 0.5 * invdy
      - (v[ip1+j*b.ni] - v[im1+j*b.ni]) * 0.5 * invdx
      );
      double laplace_p = (
        (p[i+(j+1)*b.ni] + p[i+(j-1)*b.ni] - 2.0*p[i+j*b.ni]) * invdy * invdy
       +(p[ip1+j*b.ni] + p[im1+j*b.ni] - 2.0*p[i+j*b.ni]) * invdx * invdx
      );
      rhs[i+j*b.ni] = 0.75 * rhofzeta + 0.25 * laplace_p;
    }
  }

  const size_t NITER = 10000;
  const double accel = 30.0;
  const double shuusoku = 0.001;
  double first2res = 0.0;
  for (size_t iter=0; iter<NITER; iter++) {
    double sum2res = 0.0;
    double sum2dif = 0.0;
    for (size_t j=1; j<b.nj-1; j++) {
      double invdydy = pow(invdeg/b.dj, 2.0);
      double invdxdx = pow(invdeg/(b.di*cosdeg(bp_lat(&b,j))), 2.0);
      double dxdx = accel/invdeg;
      for (size_t i=0; i<b.ni; i++) {
        size_t ip1 = (i+1)%b.ni;
        size_t im1 = (i+b.ni-1)%b.ni;
        double laplace_p = (
          (p[i+(j+1)*b.ni] + p[i+(j-1)*b.ni] - 2.0*p[i+j*b.ni]) * invdydy
+         (p[ip1+j*b.ni] + p[im1+j*b.ni] - 2.0*p[i+j*b.ni]) * invdxdx
        );
        double residual = rhs[i+j*b.ni] - laplace_p;
        cor[i+j*b.ni] = residual*dxdx;
        sum2res += residual*residual;
      }
    }
    for (size_t j=1; j<b.nj-1; j++) {
      for (size_t i=0; i<b.ni; i++) {
        p[i+j*b.ni] -= cor[i+j*b.ni];
        sum2dif += pow(p[i+j*b.ni]-pmsl[i+j*b.ni], 2.0);
      }
    }
    printf("iter=%zu st.res=%g st.dif=%g\n", iter, sqrt(sum2res/npixels),
      sqrt(sum2dif/npixels));
    if (iter == 0) {
      first2res = sum2res;
    } else if (sum2res < shuusoku*first2res) {
      break;
    }
  }

  // save
  size_t onx = ofp->xz - ofp->xa + 1;
  size_t ony = ofp->yz - ofp->ya + 1;
  double *gbuf = mymalloc(sizeof(double) * onx * ony);
  if (gbuf == NULL) { return ERR_NOMEM; }
  reproject(gbuf, &b, p, ofp);
  set_parameter(strap->gsp_v, IPARM_Pres);
  char filename[256];
  mkfilename(filename, sizeof filename, strap->gsp_v, NULL);
  gridsave(gbuf, onx, ony, PALETTE_Pmsl, filename, textv, NULL);
  
  myfree(gbuf);
  myfree(p);
  myfree(rhs);
  return 8;
  //return GSE_OKAY;
}

  gribscan_err_t
check_sfcanal(const struct grib2secs *gsp, double *dbuf,
  outframe_t *ofp, char **textv)
{
  // phase 1: filter
  double vlev_gsp = get_vlevel(gsp);
  if ((vlev_gsp != 101324.0) && (vlev_gsp != VLEVEL_Z10M)) {
    return GSE_OKAY;
  }
  long ftime2_gsp = get_ftime(gsp) + get_duration(gsp);
  iparm_t iparm_gsp = get_parameter(gsp);
  gribscan_err_t r = GSE_OKAY;
  size_t npixels = get_npixels(gsp);
  for (int i=0; i<N_SFCTRAPS; i++) {
    // phase 2: keep
    grib2secs_t **gspp;
    if (sfctrap[i].ftime2 != ftime2_gsp) { continue; }
    gspp = NULL;
    switch (iparm_gsp) {
    case IPARM_U: gspp = &(sfctrap[i].gsp_u); break;
    case IPARM_V: gspp = &(sfctrap[i].gsp_v); break;
    case IPARM_Pmsl: gspp = &(sfctrap[i].gsp_pmsl); break;
    default: ; break;
    }
    if (gspp == NULL) { continue; }
    if (*gspp) {
      del_grib2secs(*gspp); *gspp = NULL;
    }
    *gspp = dup_grib2secs(gsp);
    (*gspp)->omake = mydup(dbuf, sizeof(double) * npixels);
    // phase 3: fire
    if (!sfctrap[i].gsp_u || !sfctrap[i].gsp_v || !sfctrap[i].gsp_pmsl) {
      continue;
    }
    r = sfcanal(&(sfctrap[i]), ofp, textv);
    myfree(sfctrap[i].gsp_u->omake);
    myfree(sfctrap[i].gsp_v->omake);
    myfree(sfctrap[i].gsp_pmsl->omake);
    del_grib2secs(sfctrap[i].gsp_u);
    del_grib2secs(sfctrap[i].gsp_v);
    del_grib2secs(sfctrap[i].gsp_pmsl);
    sfctrap[i].gsp_u = sfctrap[i].gsp_v = sfctrap[i].gsp_pmsl = NULL;
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
        }
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
