#include <stdio.h>
#include <math.h>
#include "plot.h"
#include "emagram.h"

// 気温t[K], 気圧p[hPa]に対して温位 [K] を与える
  double
potemp(double t, double p)
{
  double th = t * pow(1.0e3 / p, 0.2854);
  return th;
}

// 温位t[K], 気圧p[hPa]に対して気温 [K] を与える
  double
inv_potemp(double th, double p)
{
  double t = th * pow(p / 1.0e3, 0.2854);
  return t;
}

// 気温t[K], 相対湿度rh[%], 気圧p[hPa]に対して TD [K] を与える
  double
t2td(double t, double rh, double p)
{
  double es = 6.112 * exp(17.67 * (t - 273.15) / (t - 29.65));
  double e = es * rh * 0.01;
  double lne = log(e/6.112);
  double td = (17.67*273.15-lne*29.65)/(17.67-lne);
  return td;
}

// 気温t[K], 相対湿度rh[%], 気圧p[hPa]に対して温位 [K] を与える
// Bolton(1980) による。
// https://doi.org/10.1175/1520-0493(1980)108%3C1046:TCOEPT%3E2.0.CO;2
// 動作範囲の注意:
// Bolton が精度を検証しているのは -35℃ .. 35℃ まで。かなり高温まで使えるが、
// 96℃ 100%1atmで 2.9e50 を返し、98℃までに爆発する（infやnanを返す）
  double
ept_bolton(double t, double rh, double p)
{
  double es = 6.112 * exp(17.67 * (t - 273.15) / (t - 29.65));
  double e = es * rh * 0.01;
  double ke = log(e/6.112) / 17.67;
  double td = 0.05 * (-5463.0 + 593.0 * ke) / (-1.0 + ke);
  double tlcl = 1.0 / (1.0 / (td - 56.0) + log(t / td) / 800.0) + 56.0;
  double x = 0.622 * e / (p - e);
  double thdl = t * pow(1.0e3 / (p - e), 0.2854) * pow(t / tlcl, 0.28 * x);
  double ept = thdl * exp((3036.0 / tlcl - 1.78) * x * (1.0 + 0.448 * x));
  return ept;
}

// 相当温位 ept[K] の（気温の関数としての）逆関数。
// 気圧は p[hPa] で与える。相対湿度は 100% とする。
// 第一推定値 t1 からステップ tstep で振って二分法で求める。
  double
inv_ept_core(double ept, double p, double t1, double tstep)
{
  const double rh = 100.0;
  const double TSTEP_MIN = 0.1;
  double ept1, ept2;
  // 上側の関数値。
  // 上側になっていなければ tstep だけ上がったところでやりなおし
SWEEP_UP:
  ept1 = ept_bolton(t1, rh, p);
  if (ept1 < ept) {
    t1 += tstep;
    goto SWEEP_UP;
  } else if (isnan(ept1)) {
    // 高温すぎるばあいは NaN を返す
    return ept1;
  }
  // 下側の関数値。
  // 適当なステップ tstep だけ下がったところで関数値を試算して、
  // 下側になっていなければ t1-tstep を上側としてやりなおし
SWEEP_DOWN:
  ept2 = ept_bolton(t1-tstep, rh, p);
  if (ept2 > ept) {
    t1 -= tstep;
    ept1 = ept2;
    goto SWEEP_DOWN;
  }
  if (tstep < TSTEP_MIN) {
    // 収束していれば ept1, ept2 で線形補間して返す
    return t1-((ept1-ept)/(ept1-ept2))*tstep;
  } else {
    // 収束していなければステップを半分にしてやりなおし
    return inv_ept_core(ept, p, t1, 0.5*tstep);
  }
}

// 相当温位 ept の（気温の関数としての）逆関数。
// 気圧は p [hPa] で与える。相対湿度は 100% とする。
  double
inv_ept(double ept, double p)
{
  // 第一推定値として、過小な値が望ましく、273.15 K を与える。
  // ステップは 32K とする。float64 的に切りの良い数字であり、
  // 32*6 = 96℃までの範囲がアルゴリズム的に破綻せず動くことを狙ったもの。
  return inv_ept_core(ept, p, 273.15, 32.0);
}

double plevs[] = { 1000.0, 925.0, 850.0, 700.0, 500.0, 
  300.0, 250.0, 200.0, 150.0, 100.0 };
unsigned nplevs = sizeof(plevs)/sizeof(double);

#ifdef TESTMAIN1
  int
main(int argc, char **argv)
{
  // 相当温位近似式の適用限界のチェック
  for (double tc = 70.0; tc < 200.0; tc+=1.0) {
    double t = tc + 273.15;
    double ept = ept_bolton(t, 100.0, 1013.25);
    printf("tc=%g t=%g ept=%g\n", tc, t, ept);
    if (isnan(ept)) break;
  }
  // 温位の逆関数のテスト
  for (double th_c = -60.0; th_c <= 180.0; th_c += 20.0) {
    printf("= TH %5.1f %5.1f\n", th_c, th_c+273.15);
    for (unsigned iz = 0; iz < nplevs; iz++) {
      double t = inv_potemp(th_c+273.15, plevs[iz]);
      printf("%6.1f\t%5.1f\t%5.1f\n", plevs[iz], t, t-273.15);
    }
  }
  // 相当温位の逆関数のテスト
  for (double ept_c = -50.0; ept_c <= 130.0; ept_c += 20.0) {
    printf("= EPT %5.1f %5.1f\n", ept_c, ept_c+273.15);
    for (unsigned iz = 0; iz < nplevs; iz++) {
      double t = inv_ept(ept_c+273.15, plevs[iz]);
      printf("%6.1f\t%5.1f\t%5.1f\n", plevs[iz], t, t-273.15);
    }
  }
  return 0;
}
#endif /* TESTMAIN1 */

#define Y_BOTTOM  100.0f
#define Y_SPAN 700.0f
#define X_LEFT 100.0f
#define X_SPAN 800.0f
#define X_RIGHT (X_LEFT + X_SPAN)

static grtype_t cgrtype = GR_SKEWT;
  int
setgraphtype(grtype_t gt)
{
  cgrtype = gt;
  return 0;
}

void
xyconv(float tc, float p, float *xp, float *yp)
{
  float y = (3.0f - log10f(p)) * Y_SPAN;
  if (cgrtype == GR_EMAGRAM) {
    float x = (tc - -70.0f) * 0.0091f * X_SPAN;
    *yp = Y_BOTTOM + y;
    *xp = X_LEFT + x;
  } else if (cgrtype == GR_POTEMP) {
    float x = (potemp(tc+273.15, p) - 230.0f) * 0.01f * X_SPAN;
    *yp = Y_BOTTOM + y;
    *xp = X_LEFT + x;
  } else {  // GR_SKEWT and default
    float x = (tc - -40.0f) * 0.0125f * X_SPAN;
    *yp = Y_BOTTOM + y;
    *xp = X_LEFT + x + y;
  }
//printf(":: %8.1f %8.1f %8.1f %8.1f\n", (double)tc, (double)p, *xp, *yp);
}

void
move_tp(float tc, float p)
{
  float x, y;
  xyconv(tc, p, &x, &y);
  moveto(x, y);
}

void
line_tp(float tc, float p)
{
  float x, y;
  xyconv(tc, p, &x, &y);
  lineto(x, y);
}

void
move_tzp(float tc, float p)
{
  float x, xz, y;
  xyconv(tc, 1000.0f, &xz, &y);
  xyconv(tc, p, &x, &y);
  moveto(xz, y);
}

void
line_tzp(float tc, float p)
{
  float x, xz, y;
  xyconv(tc, 1000.0f, &xz, &y);
  xyconv(tc, p, &x, &y);
  lineto(xz, y);
}

void
arrow(float tc, float p, float u, float v)
{
  const float stem = 40.0f;
  const float barb = 20.0f;
  float wf = hypotf(u, v);
  unsigned knot = roundf(wf * 3600.0f / 1852.0f / 5.0f) * 5.0f;
  if ((knot == 0) && (wf > 0.5)) { knot = 5u; }
  float xdum, x0, ydum, y0;
  xyconv(tc, 1000.0f, &x0, &ydum);
  xyconv(tc, p, &xdum, &y0);
  if (knot == 0) {
    moveto(x0+2, y0);
    lineto(x0, y0+2);
    lineto(x0-2, y0);
    lineto(x0, y0-2);
    lineto(x0+2, y0);
    return;
  }
  float eu = -u / wf;
  float ev = -v / wf;
  moveto(x0, y0);
  lineto(x0+eu*stem, y0+ev*stem);
  float fu = 0.342f * eu + 0.940f * ev;
  float fv = -0.940f * eu + 0.342f * ev;

  // SHORTER BARBS
  switch (knot) {
  case 5: case 15: case 55:
    moveto(x0+0.85f*eu*stem, y0+0.85f*ev*stem);
    lineto(x0+0.85f*eu*stem+0.5f*fu*barb, y0+0.85f*ev*stem+0.5f*fv*barb);
    break;
  case 25: case 65: case 105:
    moveto(x0+0.70f*eu*stem, y0+0.70f*ev*stem);
    lineto(x0+0.70f*eu*stem+0.5f*fu*barb, y0+0.70f*ev*stem+0.5f*fv*barb);
    break;
  case 35: case 75: case 115: case 155:
    moveto(x0+0.55f*eu*stem, y0+0.55f*ev*stem);
    lineto(x0+0.55f*eu*stem+0.5f*fu*barb, y0+0.55f*ev*stem+0.5f*fv*barb);
    break;
  case 45: case 85: case 125: case 165:
    moveto(x0+0.40f*eu*stem, y0+0.40f*ev*stem);
    lineto(x0+0.40f*eu*stem+0.5f*fu*barb, y0+0.40f*ev*stem+0.5f*fv*barb);
    break;
  case 95: case 135: case 175:
    moveto(x0+0.25f*eu*stem, y0+0.25f*ev*stem);
    lineto(x0+0.25f*eu*stem+0.5f*fu*barb, y0+0.25f*ev*stem+0.5f*fv*barb);
    break;
  case 145: case 185:
    moveto(x0+0.10f*eu*stem, y0+0.10f*ev*stem);
    lineto(x0+0.10f*eu*stem+0.5f*fu*barb, y0+0.10f*ev*stem+0.5f*fv*barb);
    break;
  }

  // MAIN BARBS
  switch (knot) {
  case 190: case 195:
    moveto(x0+0.2f*eu*stem, y0+0.2f*ev*stem);
    lineto(x0+0.2f*eu*stem+fu*barb, y0+0.2f*ev*stem+fv*barb);
  case 180: case 185:
  case 140: case 145:
    moveto(x0+0.35f*eu*stem, y0+0.35f*ev*stem);
    lineto(x0+0.35f*eu*stem+fu*barb, y0+0.35f*ev*stem+fv*barb);
  case 170: case 175:
  case 130: case 135:
  case 90: case 95:
    moveto(x0+0.4f*eu*stem, y0+0.4f*ev*stem);
    lineto(x0+0.4f*eu*stem+fu*barb, y0+0.4f*ev*stem+fv*barb);
  case 160: case 165:
  case 120: case 125:
  case 80: case 85:
  case 40: case 45:
    moveto(x0+0.55f*eu*stem, y0+0.55f*ev*stem);
    lineto(x0+0.55f*eu*stem+fu*barb, y0+0.55f*ev*stem+fv*barb);
  case 150: case 155:
  case 110: case 115:
  case 70: case 75:
  case 30: case 35:
    moveto(x0+0.7f*eu*stem, y0+0.7f*ev*stem);
    lineto(x0+0.7f*eu*stem+fu*barb, y0+0.7f*ev*stem+fv*barb);
  case 100: case 105:
  case 60: case 65:
  case 20: case 25:
    moveto(x0+0.85f*eu*stem, y0+0.85f*ev*stem);
    lineto(x0+0.85f*eu*stem+fu*barb, y0+0.85f*ev*stem+fv*barb);
  case 50: case 55:
  case 10:
    moveto(x0+eu*stem, y0+ev*stem);
    lineto(x0+eu*stem+fu*barb, y0+ev*stem+fv*barb);
  }

  // PENNANTS
  if (knot >= 50) {
    moveto(x0+eu*stem+fu*barb, y0+ev*stem+fv*barb);
    lineto(x0+0.85f*eu*stem, y0+0.85f*ev*stem);
  }
  if (knot >= 100) {
    moveto(x0+0.85f*eu*stem+fu*barb, y0+0.85f*ev*stem+fv*barb);
    lineto(x0+0.7f*eu*stem, y0+0.7f*ev*stem);
  }
  if (knot >= 150) {
    moveto(x0+0.7f*eu*stem+fu*barb, y0+0.7f*ev*stem+fv*barb);
    lineto(x0+0.55f*eu*stem, y0+0.55f*ev*stem);
  }

}

  int
draw_emagram_frame(void)
{
  // 全画面白で塗りつぶし
  setrgb(255, 255, 255);
  box(0, 0, 1023, 1023);
  // 等温線
  setlinewidth(1.0f);
  setrgb(220, 64, 220);
  setfontsize(20);
  for (float t=-42.0f; t<=36.0f; t+=6.0f) {
    move_tp(t, 100.0f);
    for (float p=100.0f; p<=1000.0f; p+=25.0f){
      line_tp(t, p);
    }
    move_tp(t, 1060.0f);
    { char buf[32]; sprintf(buf, "%+3d", (int)t); symbol(buf); }
  }
  for (float t=-66.0f; t<=-48.0f; t+=6.0f) {
    move_tp(t, 70.0f);
    for (float p=100.0f; p<=500.0f; p+=25.0f){
      line_tp(t, p);
    }
    move_tp(t, 70.0f);
    { char buf[32]; sprintf(buf, "%+3d", (int)t); symbol(buf); }
  }
  // 乾燥断熱線（等温位線）
  setfontsize(18);
  setrgb(64, 64, 255);
  for (float th=240.0f; th<=400.0f; th+=20.0f) {
    float t;
    t= inv_potemp(th, 100.0f);
    move_tp(t-273.15f, 100.0f);
    for (float p=100.0f; p<=1000.0f; p+=25.0f){
      t = inv_potemp(th, p);
      line_tp(t-273.15f, p);
    }
    t= inv_potemp(th, 98.0f);
    move_tp(t-273.15f, 98.0f);
    { char buf[32]; sprintf(buf, "%3u", (unsigned)th); symbol(buf); }
  }
  // 湿潤断熱線（等相当温位線）
  setrgb(64, 255, 64);
  for (float th=250.0f; th<=410.0f; th+=20.0f) {
    float t;
    t = inv_ept(th, 100.0f);
    move_tp(t-273.15f, 100.0f);
    for (float p=100.0f; p<=1000.0f; p+=25.0f){
      t = inv_ept(th, p);
      line_tp(t-273.15f, p);
    }
    t= inv_potemp(th, 98.0f);
    move_tp(t-273.15f, 98.0f);
    { char buf[32]; sprintf(buf, "%3u", (unsigned)th); symbol(buf); }
  }
  // 左右を白箱でマスク
  setrgb(255, 255, 255);
  box(0, 0, 100, 1023);
  box(900, 0, 1023, 1023);
  // 等圧線
  setlinewidth(1.0f);
  setrgb(0, 0, 0);
  setfontsize(28);
  for (unsigned iz = 0; iz < nplevs; iz++) {
    move_tzp(-40.0f, plevs[iz]);
    line_tzp(40.0f, plevs[iz]);
    move_tzp(-48.0f, plevs[iz]);
    { char buf[32]; sprintf(buf, "%4u", (unsigned)plevs[iz]); symbol(buf); }
  }
  return 0;
}

  int
profile_color(unsigned i)
{
  switch (i) {
  case 0u:
    setrgb(180u, 0u, 104u);
    break;
  case 1u:
    setrgb(255u, 40u, 0u);
    break;
  case 2u:
    setrgb(255u, 153u, 0u);
    break;
  case 3u:
    setrgb(240u, 235u, 0u);
    break;
  case 4u:
    setrgb(0u, 65u, 255u);
    break;
  case 5u:
    setrgb(33u, 140u, 255u);
    break;
  case 6u:
    setrgb(150u, 200u, 255u);
    break;
  default:
  case 7u:
    setrgb(210u, 190u, 252u);
    break;
  }
  return 0;
}

static int selected_idx = 0;

  int
set_zw_profile(int i)
{
  selected_idx = i;
  return 0;
}

  int
draw_profile(obs_t *obs, unsigned i)
{
  profile_color(i);
  // display t and td
  setlinewidth(3.0f);
  move_tp(obs->ttd[0].x-273.15f, obs->ttd[0].p);
  for (size_t j=0; j<obs->ttd_count; j++) {
    if (isnan(obs->ttd[j].x)) break;
    line_tp(obs->ttd[j].x-273.15f, obs->ttd[j].p);
  }
  move_tp(obs->ttd[0].y-273.15f, obs->ttd[0].p);
  for (size_t j=0; j<obs->ttd_count; j++) {
    if (isnan(obs->ttd[j].y)) break;
    line_tp(obs->ttd[j].y-273.15f, obs->ttd[j].p);
  }
  // display name of obs
  setfontsize(24);
  moveto(7.0f, 1000.0f-(float)i*24.0f);
  symbol(obs->name);
  // skip z and wind
  if (i != selected_idx) return 0;
  // display z
  setfontsize(18);
  for (size_t j=0; j<obs->z_count; j++) {
    char text[32];
    unsigned p = roundf(obs->z[j].p);
    switch (p) {
    case 1000: case 925: case 850: case 700: case 500:
    case 300: case 250: case 200: case 150: case 100:
      move_tzp(33.0f, p*0.99f);
      snprintf(text, 32, "%5.0fm", obs->z[j].x);
      symbol(text);
      break;
    default:
      break;
    }
  }
  // display wind
  setlinewidth(1.0f);
  for (size_t j=0; j<obs->uv_count; j++) {
    arrow(46.0f, obs->uv[j].p, obs->uv[j].x, obs->uv[j].y);
  }
  return 0;
}

  int
draw_emagram(obs_t *obs, size_t obs_count)
{
  openpl();
  draw_emagram_frame();
  for (size_t i=0; i<obs_count; i++) {
    draw_profile(&(obs[i]), i);
  }
  closepl();
  return 0;
}
