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

void
xyconv(float tc, float p, float *xp, float *yp)
{
  float y = (3.0f - log10f(p)) * Y_SPAN;
  float x = (tc - -40.0f) * 0.0125 * X_SPAN;
  *yp = Y_BOTTOM + y;
  *xp = X_LEFT + x + y;
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

  int
draw_emagram_frame(void)
{
  setrgb(255, 255, 255);
  box(0, 0, 1023, 1023);
  setlinewidth(1.0f);
  setrgb(200, 0, 200);
  setfontsize(20);
  for (float t=-54.0f; t<=36.0f; t+=6.0f) {
    move_tp(t, 100.0f);
    for (float p=100.0f; p<=1000.0f; p+=25.0f){
      line_tp(t, p);
    }
    move_tp(t, 1060.0f);
    { char buf[32]; sprintf(buf, "%3d", (int)t); symbol(buf); }
  }
  setfontsize(18);
  setrgb(0, 0, 255);
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
  setrgb(0, 200, 0);
  for (float th=250.0f; th<=390.0f; th+=20.0f) {
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
  setrgb(255, 255, 255);
  box(0, 0, 100, 1023);
  box(900, 0, 1023, 1023);
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
draw_profile(obs_t *obs)
{
  setlinewidth(2.0f);
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
  return 0;
}


  int
draw_emagram(obs_t *obs, size_t obs_count)
{
  openpl();
  draw_emagram_frame();
  for (size_t i=0; i<obs_count; i++) {
    setrgb(0, 0, i*31);
    draw_profile(&(obs[i]));
  }
  closepl();
  return 0;
}
