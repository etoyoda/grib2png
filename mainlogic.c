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

int verbose = 0;

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

#if 0
  void
printa(double *ary, size_t n, char *name)
{
  double min, max, sum, sqsm, absm, avr;
  min = HUGE_VAL;
  max = -HUGE_VAL;
  sum = sqsm = absm = 0.0;
  for (size_t i=0; i<n; i++) {
    sum += ary[i];
    absm += fabs(ary[i]);
    if (ary[i] < min) min = ary[i];
    if (ary[i] > max) max = ary[i];
  }
  avr = sum/n;
  for (size_t i=0; i<n; i++) {
    sqsm += (ary[i]-avr)*(ary[i]-avr);
  }
  printf("%-6s min%.3g max%.3g avg%.3g sd%.3g avg.abs%.3g\n",
  name, min, max, avr, sqrt(sqsm/n), absm/n);
}
  printa(u, npixels, "u");
  printa(v, npixels, "v");
  printa(p, npixels, "p");
#endif

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
  size_t bni = b.ni;
  // 東西周期条件を確認
  if (b.wraplon == 0) {
    fprintf(stderr, "regional data\n");
    return ERR_BADGRIB;
  }
  // u, v は 0.1m/s 単位 (SIの10倍値)で与えられる
  double *u = strap->gsp_u->omake;
  double *v = strap->gsp_v->omake;
  // pmsl は 0.1hPa = 10Pa 単位 (SIの1/10値)で与えられる
  // 以下、気圧やその微分の次元の式は、SIの1/10で計算する
  double *pmsl = strap->gsp_pmsl->omake;
  // 緩和法の結果として返される、変数としての流線気圧 [10Pa]
  double *p = mymalloc(sizeof(double)*npixels);
  // laplace p の式の右辺、緩和法のターゲット [10Pa/m2]
  double *rhs = mymalloc(sizeof(double)*npixels);
  // pの次回サイクルの補正量 [10Pa]
  double *cor = mymalloc(sizeof(double)*npixels);
  if ((rhs==NULL)||(p==NULL)||(cor==NULL)) { return ERR_NOMEM; }
  memcpy(p, pmsl, sizeof(double)*npixels);
  // 緯度一度の長さ deglat = (6371.e3*M_PI)/180.0 の逆数 [1/m]
  const double invdeg = 180.0/(M_PI*6371.e3);
  // 密度 [100 kg/m3] - SIの1/100値にしているのはu,pmslの単位の皺寄せ
  double rho = 0.01 * 101325.0 * 28.96e-3 / (8.31432 * 273.15);
  // 摩擦: fとの比率
  double nfric_ratio = 0.4;

  for (size_t j=1; j<(b.nj-1); j++) {
    size_t jp1 = j+1;
    size_t jm1 = j-1;
    double invdy = -invdeg/b.dj;
    double lat = bp_lat(&b,j);
    double invdx = invdeg/(b.di*cosdeg(lat));
    double f = 4.0*M_PI/86400.0*sindeg(bp_lat(&b,j));
    double is_tropical = 0.5+0.5*tanh((22.5-fabs(lat))*0.5);
    for (size_t i=0; i<bni; i++) {
      size_t ip1 = (i+1)%bni;
      size_t im1 = (i+bni-1)%bni;
      double rhofzeta = rho * f * (
        (u[i+jp1*bni] - u[i+jm1*bni]) * 0.5 * invdy
      - (v[ip1+j*bni] - v[im1+j*bni]) * 0.5 * invdx
      );
      double nfric = fabs(f) * nfric_ratio;
      double friction = rho * nfric * (
        (u[ip1+j*bni] - u[im1+j*bni]) * 0.5 * invdy
      + (v[i+jp1*bni] - v[i+jm1*bni]) * 0.5 * invdx
      );
      double laplace_p = (
        (p[i+jp1*bni] + p[i+jm1*bni] - 2.0*p[i+j*bni]) * invdy * invdy
       +(p[ip1+j*bni] + p[im1+j*bni] - 2.0*p[i+j*bni]) * invdx * invdx
      );
      double windythr = 150.0 - is_tropical*50.0;
      double is_windy = 0.5+0.5*tanh(
        (hypot(u[i+j*bni],v[i+j*bni])-windythr)*0.5
      );
      // 風が強い場合 laplace_p 
      double mix = 0.60 - is_windy*0.60;
      rhs[i+j*bni] = mix*(rhofzeta+friction) + (1.-mix)*laplace_p;
    }
  }

  const size_t NITER = 200;
  const double shuusoku = 0.015;
  double accel = 0.25;
  double first2res = 0.0;
  double diftoomuch = 20.0;
  for (size_t iter=0; iter<NITER; iter++) {
    double sum2res = 0.0;
    double sum2dif = 0.0;
    for (size_t j=1; j<b.nj-1; j++) {
      size_t jp1 = j+1;
      size_t jm1 = j-1;
      double invdydy = pow(invdeg/b.dj, 2.0);
      double invdxdx = pow(invdeg/(b.di*cosdeg(bp_lat(&b,j))), 2.0);
      double dxdx = accel/invdxdx;
      for (size_t i=0; i<bni; i++) {
        size_t ip1 = (i+1)%bni;
        size_t im1 = (i+bni-1)%bni;
        double laplace_p = (
          (p[i+jp1*bni]+p[i+jm1*bni]-2.0*p[i+j*bni]) * invdydy
        + (p[ip1+j*bni]+p[im1+j*bni]-2.0*p[i+j*bni]) * invdxdx
        );
        double residual = (rhs[i+j*bni] - laplace_p)*dxdx;
        cor[i+j*bni] = residual;
        sum2res += residual*residual;
      }
    }
    for (size_t j=1; j<b.nj-1; j++) {
      for (size_t i=0; i<bni; i++) {
        p[i+j*bni] -= cor[i+j*bni];
        sum2dif += pow(p[i+j*bni]-pmsl[i+j*bni], 2.0);
      }
    }
    double dif = sqrt(sum2dif/npixels);
    if (verbose) {
      printf("iter=%zu avgres=%g avgdif=%g\n", iter, sqrt(sum2res/npixels), dif);
    }
    if (iter == 0) {
      first2res = sum2res;
    } else if (isnan(sum2res) || isinf(sum2res)) {
      fprintf(stderr, "explosion %g\n", sum2res);
      return ERR_BADGRIB;
    } else if (sum2res > 2.0*first2res) {
      accel *= 0.125;
      fprintf(stderr, "detected exploding; accel := %g\n", accel);
    } else if (dif > diftoomuch) {
      fprintf(stderr, "too much diff %g\n", dif);
      break;
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
  //return 8;
  return GSE_OKAY;
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
      case 'v':
        verbose++;
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
