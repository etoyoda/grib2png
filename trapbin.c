#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "mymalloc.h"
#include "visual.h"
#include "gribscan.h"
#include "grib2png.h"

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

FILE *text_fp = NULL;

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
