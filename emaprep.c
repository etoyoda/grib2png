#include <stdio.h>
#include <math.h>

  double
potemp(double t, double p)
{
  double th = t * pow(1.0e3 / p, 0.2854);
  return th;
}

  double
inv_potemp(double th, double p)
{
  double t = th * pow(p / 1.0e3, 0.2854);
  return t;
}

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

  double
inv_ept_core(double ept, double p, double t1, double tstep)
{
  const double rh = 100.0;
  double ept1, ept2;
  ept1 = ept_bolton(t1, rh, p);
SWEEP_DOWN:
  ept2 = ept_bolton(t1-tstep, rh, p);
  if (ept2 > ept) {
    t1 -= tstep;
    ept1 = ept2;
    goto SWEEP_DOWN;
  }
  if (tstep < 0.005) {
    return t1-0.5*tstep;
  } else {
    return inv_ept_core(ept, p, t1, 0.5*tstep);
  }
}

  double
inv_ept(double ept, double p)
{
  return inv_ept_core(ept, p, inv_potemp(ept, p), 16.0);
}

double plevs[] = { 1000.0, 925.0, 850.0, 700.0, 500.0, 
  300.0, 250.0, 200.0, 150.0, 100.0 };
unsigned nplevs = sizeof(plevs)/sizeof(double);

  int
main(int argc, char **argv)
{
  for (double th_c = -60.0; th_c <= 180.0; th_c += 20.0) {
    printf("= TH %5.1f %5.1f\n", th_c, th_c+273.15);
    for (unsigned iz = 0; iz < nplevs; iz++) {
      double t = inv_potemp(th_c+273.15, plevs[iz]);
      printf("%6.1f\t%5.1f\t%5.1f\n", plevs[iz], t, t-273.15);
    }
  }
  for (double ept_c = -50.0; ept_c <= 130.0; ept_c += 20.0) {
    printf("= EPT %5.1f %5.1f\n", ept_c, ept_c+273.15);
    for (unsigned iz = 0; iz < nplevs; iz++) {
      double t = inv_ept(ept_c+273.15, plevs[iz]);
      printf("%6.1f\t%5.1f\t%5.1f\n", plevs[iz], t, t-273.15);
    }
  }
  return 0;
}
