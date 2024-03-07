#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "mymalloc.h"
#include "visual.h"
#include "gribscan.h"
#include "grib2png.h"

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
  const double deglat = (6371.e3*M_PI)/180.0;
  const double invdeg = 180.0/(M_PI*6371.e3);
  // 密度 [100 kg/m3] - SIの1/100値にしているのはu,pmslの単位の皺寄せ
  double rho = 0.01 * 101325.0 * 28.96e-3 / (8.31432 * 273.15);
  // 摩擦: fとの比率
  double nfric_ratio = 0.4;

  // ファイル出力用バッファ(使いまわす)
  char filename[256];
  size_t onx = ofp->xz - ofp->xa + 1;
  size_t ony = ofp->yz - ofp->ya + 1;
  double *gbuf = mymalloc(sizeof(double) * onx * ony);
  if (gbuf == NULL) { return ERR_NOMEM; }

  // 遠心力項
  double *cfug = mymalloc(sizeof(double)*npixels);
  double *turn = mymalloc(sizeof(double)*npixels);
  if ((cfug==NULL)||(turn==NULL)) { return ERR_NOMEM; }
  for (size_t i=0; i<npixels; i++) { cfug[i] = turn[i] = 0.0; }

  for (size_t j=0; j<b.nj; j++) {
    double lat = bp_lat(&b,j);
    for (size_t i=0; i<bni; i++) {
      size_t ip1 = (i+1)%bni;
      // 低気圧回転
      if ((lat*v[i+j*bni]<=0.0)&&(lat*v[ip1+j*bni]>0.0)) {
        turn[i+j*bni] += 1.0;
      }
      // 高気圧回転
      if ((lat*v[i+j*bni]>=0.0)&&(lat*v[ip1+j*bni]<0.0)) {
        turn[i+j*bni] -= 1.0;
      }
    }
  }
  double match_min = 0.5;
  double vmaxpmagic = 0.5;
  for (size_t i=0; i<bni; i++) {
    for (size_t j=4; j<b.nj-1-4; j++) {
      double lat = bp_lat(&b,j);
      double is_tropical = 0.5+0.5*tanh((22.5-fabs(lat))*0.5);
      size_t jp1 = j+1;
      if ((lat*u[i+j*bni]<=0.0)&&(lat*u[i+jp1*bni]>0.0)) {
        // 低気圧回転
        turn[i+j*bni] += 2.0;
      }
      if ((lat*u[i+j*bni]>=0.0)&&(lat*u[i+jp1*bni]<0.0)) {
        // 高気圧回転
        turn[i+j*bni] -= 2.0;
      }
      if (!((turn[i+j*bni]==3.0)||(turn[i+j*bni]==-3.0))) {
        continue;
      }
      size_t ip1 = (i+1)%bni;
      double ci = (double)i+fabs(v[i+j*bni])/fabs(v[i+j*bni]-v[ip1+j*bni]);
      double cj = (double)j+fabs(u[i+j*bni])/fabs(u[i+j*bni]-u[i+jp1*bni]);
      double n1, n2, n3, n4, s1, s2, s3, m1, m2, m3;
      n1=n2=n3=n4=s1=s2=s3=m1=m2=m3=0.0;
      double p0 = pmsl[i+j*bni];
      double p4 = 0.0;
      for (size_t rj=j-4; rj<=j+4; rj++) {
        for (ssize_t ris=(ssize_t)i-4; ris<=(ssize_t)i+4; ris++) {
          size_t ri = (size_t)(ris+bni)%bni;
          double d = hypot(((double)ris-ci)*cosdeg(lat), (double)rj-cj);
          double isd0 = 0.5+0.5*tanh((50.e3-d*deglat)/20.e3);
          double isd1 = 0.5+0.5*tanh((150.e3-d*deglat)/20.e3) - isd0;
          double isd2 = 0.5+0.5*tanh((250.e3-d*deglat)/20.e3) - isd1;
          double isd3 = 0.5+0.5*tanh((350.e3-d*deglat)/20.e3) - isd2;
          double isd4 = 0.5+0.5*tanh((500.e3-d*deglat)/50.e3) - isd3;
          n1 += isd1;
          n2 += isd2;
          n3 += isd3;
          n4 += isd4;
          // 北半球で低気圧回転の方向ベクトル
          // jは南に増えることに注意
          double unit_i = ((double)rj-cj)/d;
          double unit_j = ((double)ris-ci)*cosdeg(lat)/d;
          double wspd = hypot(u[ri+rj*bni],v[ri+rj*bni]);
          double iprod = u[ri+rj*bni]*unit_i+v[ri+rj*bni]*unit_j;
          // 南半球で反転
          if (lat<0.0) { iprod = -iprod; }
          double match = iprod / wspd;
          m1 += isd1 * match;
          m2 += isd2 * match;
          m3 += isd3 * match;
          s1 += isd1 * iprod;
          s2 += isd2 * iprod;
          s3 += isd3 * iprod;
          p4 += isd4 * pmsl[ri+rj*bni];
          if (pmsl[ri+rj*bni] < p0) {
            p0 += isd2 * (pmsl[ri+rj*bni]-p0);
          }
        }
      }
      m1 /= n1; s1 /= n1;
      m2 /= n2; s2 /= n2;
      m3 /= n3; s3 /= n3;
      p4 /= n4;
      double rmax, vmax;
      if (m2 > match_min) {
        if (s2>s1*1.5) {
          rmax = 200.e3; vmax = s2; goto CYCLONE;
        }
      }
      if (m1 > match_min) {
        rmax = 100.e3; vmax = s1; goto CYCLONE;
      }
      if (m3 < -match_min) {
        rmax = 100.e3; vmax = -70.0; goto ANTI_CYCLONE;
      }
      // skip this vortex candidate
      continue;
    CYCLONE:
      vmax *= 1.0 + is_tropical * 1.0;
      double vmaxp = sqrt((p4-p0)/(0.1*rho)) * vmaxpmagic;
      if (vmaxp > vmax) {
        vmax = vmaxp;
        rmax = 100.e3;
      }
    ANTI_CYCLONE:
      if (verbose) {
        printf("vortex %+6.1f %+6.1f"
        " %3d %3d %3d %4d %4d %4d %5.1f %5.1f %4d\n",
        bp_lat(&b,cj), bp_lon(&b,ci),
        (int)round(m1*100.), (int)round(m2*100.), (int)round(m3*100.),
        (int)round(s1), (int)round(s2), (int)round(s3),
        (p4-p0)*0.1,
        rmax*0.001, (int)round(vmax));
      }
      for (size_t rj=j-4; rj<=j+4; rj++) {
        for (ssize_t ris=(ssize_t)i-4; ris<=(ssize_t)i+4; ris++) {
          size_t ri = (size_t)(ris+bni)%bni;
          double d = hypot(((double)ris-ci)*cosdeg(lat), (double)rj-cj)
           * deglat;
          double isinside = 0.5+0.5*tanh((rmax*1.5-d)/(0.5*deglat));
          if (vmax<0.0) {
            cfug[ri+rj*bni] -= 0.1*rho*vmax*vmax/(rmax*rmax)*isinside;
          } else {
            cfug[ri+rj*bni] += 0.1*rho*vmax*vmax/(rmax*rmax)*isinside;
          }
        }
      }
    }
  }

  if (verbose) {
    printa(cfug, npixels, "cfug");
  }
  if (verbose>=2) {
    reproject(gbuf, &b, cfug, ofp);
    for(size_t i=0; i<onx*ony; i++) { gbuf[i] *= 1.0e10; }
    set_parameter(strap->gsp_v, IPARM_PSI);
    mkfilename(filename, sizeof filename, strap->gsp_v, NULL);
    gridsave(gbuf, onx, ony, PALETTE_rVOR, filename, textv, NULL);
  }

  // forcing term
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
      double mix = 0.85 - is_windy*0.85;
      rhs[i+j*bni] = mix*(rhofzeta+friction+cfug[i+j*bni])
      + (1.-mix)*laplace_p;
    }
  }

  if (verbose) {
    printa(rhs, npixels, "rhs");
  }
  if (verbose>=2) {
    reproject(gbuf, &b, rhs, ofp);
    for(size_t i=0; i<onx*ony; i++) { gbuf[i] *= 1.0e10; }
    set_parameter(strap->gsp_v, IPARM_CHI);
    mkfilename(filename, sizeof filename, strap->gsp_v, NULL);
    gridsave(gbuf, onx, ony, PALETTE_rVOR, filename, textv, NULL);
  }

  myfree(turn);
  myfree(cfug);

  const size_t NITER = 200;
  const double converge_mr = 0.9;
  double accel = 0.25;
  double firstres = 0.0;
  double movavrres = HUGE_VAL; 
  double diftoomuch = 20.0;
  double low_magic = 0.5;
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
        double is_low = 0.5+0.5*tanh(990.e1-pmsl[i+j*bni]);
        cor[i+j*bni] = residual-low_magic*is_low*(pmsl[i+j*bni]-p[i+j*bni]);
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
    double res = sqrt(sum2res/npixels);
    if (verbose) {
      printf("iter=%04zu avgres=%8.3f %8.3f %8.3f avgdif=%8.3f\n", iter, res, movavrres, res/movavrres, dif);
    }
    if (iter == 0) {
      firstres = res;
      movavrres = res * 4.0;
    } else if (isnan(sum2res) || isinf(sum2res)) {
      fprintf(stderr, "explosion %g: abort\n", sum2res);
      return ERR_BADGRIB;
    } else if (res > 2.0*firstres) {
      accel *= 0.25;
      fprintf(stderr, "explosion; accel := %g\n", accel);
    } else if (dif > diftoomuch) {
      fprintf(stderr, "too much diff %g\n", dif);
      break;
    } else if (res > converge_mr*movavrres) {
      break;
    }
    movavrres = 0.25 * (3. * movavrres + res);
  }

  // pを保存
  reproject(gbuf, &b, p, ofp);
  set_parameter(strap->gsp_v, IPARM_Pres);
  mkfilename(filename, sizeof filename, strap->gsp_v, NULL);
  gridsave(gbuf, onx, ony, PALETTE_Pmsl, filename, textv, NULL);

  myfree(gbuf);
  myfree(p);
  myfree(rhs);
  if (debug) {
    return 8;
  }
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
