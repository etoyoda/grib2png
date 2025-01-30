#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mymalloc.h"
#include "plot.h"
#include <time.h>
#include "gribscan.h"

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

// gribscan ライブラリから呼び返される関数。
  gribscan_err_t
checksec7(const struct grib2secs *gsp)
{
  gribscan_err_t r;
  // dimensions
  struct tm t;
  char sreftime[24];
  unsigned long iparm;
  double vlev, memb;
  long ftime, dura;
  r = GSE_OKAY;
  // retrieve PDT metadata
  get_reftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
  iparm = get_parameter(gsp);
  ftime = get_ftime(gsp);
  vlev = get_vlevel(gsp);
  dura = get_duration(gsp);
  memb = get_perturb(gsp);
  // filter
  switch (gribscan_filter(sfilter, iparm, ftime, dura, vlev, memb)) {
    case ERR_FSTACK:
    case GSE_SKIP:
      goto END_SKIP;
      break;
    case GSE_OKAY:
    default:
      /* do nothing */;
  }
  printf("b%s %6s f%-+5ld d%-+5ld v%-8s m%-+4.3g\n",
    sreftime, param_name(iparm), ftime, dura, level_name(vlev), memb);
  goto END_NORMAL;

END_SKIP:
  r = GSE_SKIP;
END_NORMAL:
  myfree(gsp->ds);
  return r;
}

const char Synopsis[] = "%s [-f{mins}] -c{coastfile} input ...\n";

#define strhead(s, pat) (0==strncmp((s),(pat),strlen(pat)))

int
main(int argc, const char **argv)
{
  const char *coastfile = NULL;
  int ftime = 0;
  int r = 0;
  for (int i=1; argv[i]; i++) {
    if (strhead(argv[i], "-f")) {
      ftime=atoi(argv[i]+2);
    } else if (strhead(argv[i], "-c")) {
      coastfile = argv[i]+2;
    } else {
      fprintf(stderr, "file<%s>\n", argv[i]);
      r = grib2scan_by_filename(argv[i]);
    }
  }
  if (coastfile) {
    r = coast1(coastfile);
  } else {
    eputs("coast file unspecified\n");
    r = 1;
    goto ABEND;
  }
  return r;
ABEND:
  fprintf(stderr, Synopsis, argv[0]);
  return r;
}
