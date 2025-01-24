#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "plot.h"

#define eputs(s) (fputs((s),stderr))

int
prjmer(float lon, float lat, float *xp, float *yp)
{
  float phi2,y;
  *xp = (lon+180.0)*(1024.0/360.0);
  phi2 = 0.5*M_PI*lat/180.0f;
  y = log(tan(M_PI_4+phi2));
  y = 180.0f*y/M_PI;
  *yp = (y+180.0)*(1024.0/360.0);
}

int
coast2(FILE *ifp)
{
  int r;
  char tok[16];
  unsigned long nln, lnsz, c, cl, i;
  float lat, lon, x, y;
  openpl();
  // preamble
  r = fscanf(ifp, "%15s%lu", tok, &nln);
  if (2!=r) { eputs("ERR while reading preabmle\n"); goto fail; };
  // fprintf(stdout, "%15s=%lu\n", tok, nln);
  // loop for lines
  cl = 0u;
nextline:
  r = fscanf(ifp, "%lu%lu", &i, &lnsz);
  if (2!=r) { eputs("ERR while reading line size\n"); goto fail; }
  if (i!=cl) {
    fprintf(stderr,"line number mismatch %lu!=%lu\n", i, cl);
    goto fail;
  }
  fprintf(stdout, "LINE %lu %lu\n", cl, lnsz);
  // point
  c = 0u;
nextpoint:
  r = fscanf(ifp, "%f%f", &lon, &lat);
  if (2!=r) { eputs("ERR while reading point\n"); goto fail; }
  // fprintf(stdout, "%+8.3f %+7.3f\n", lon, lat);
  prjmer(lon, lat, &x, &y);
  if (c==0u) {
    moveto(x, y);
  } else {
    lineto(x, y);
  }
  c++;
  if (c<lnsz) goto nextpoint;
  cl++;
  if (cl<nln) goto nextline;
  // end of input
  r = closepl();
  return r;
fail:
  closepl();
  return 1;
}

int
coast1(const char *filename) {
  fprintf(stdout, "open <%s>\n", filename);
  FILE *ifp = fopen(filename, "rt");
  if (NULL==ifp) { perror(filename); return 1; }
  int r = coast2(ifp);
  if (0!=fclose(ifp)) { perror(filename); return 1; }
  return r;
}

int
main(int argc, const char **argv)
{
  const char *filename = NULL;
  for (int i=1; argv[i]; i++) {
    if (!filename) { filename = argv[i]; }
  }
  if (!filename) filename = "/dev/stdin";
  int r = coast1(filename);
  return r;
}
