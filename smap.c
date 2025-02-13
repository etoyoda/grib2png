#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mymalloc.h"
#include "plot.h"
#include <time.h>
#include "gribscan.h"

#define eputs(s) (fputs((s),stderr))
#define strhead(s, pat) (0==strncmp((s),(pat),strlen(pat)))

int
prjmer_raw(float lon, float lat, float *xp, float *yp)
{
  float phi2,y;
  *xp = (lon+180.0)*(1024.0/360.0);
  phi2 = 0.5*M_PI*lat/180.0f;
  y = log(tan(M_PI_4+phi2));
  y = 180.0f*y/M_PI;
  *yp = (y+180.0)*(1024.0/360.0);
  return 0;
}

static float bbx0=0.0f, bbxf=1.0f, bby0=0.0f, bbyf=1.0f;

int
setbbox(float lon1, float lon2, float lat1, float lat2)
{
  float x1, y1, x2, y2;
  prjmer_raw(lon1, lat1, &x1, &y1);
  prjmer_raw(lon2, lat2, &x2, &y2);
  float xmax, ymax;
  bbx0 = fminf(x1, x2);
  xmax = fmaxf(x1, x2);
  bby0 = fminf(y1, y2);
  ymax = fmaxf(y1, y2);
  bbxf = 1024.0/(xmax-bbx0);
  bbyf = 1024.0/(ymax-bby0);
  printf("setbbox %g %g %g %g > %g %g %g %g\n", lon1, lon2, lat1, lat2,
  bbx0, bbxf, bby0, bbyf);
  return 0;
}

int
prjmer(float lon, float lat, float *xp, float *yp)
{
   int r;
   float x, y;
   r = prjmer_raw(lon, lat, &x, &y);
   *xp = (x-bbx0)*bbxf;
   *yp = (y-bby0)*bbyf;
   return r;
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

//--- begin module

struct pldata_t {
  long gtime;
  size_t dslen;
  unsigned char *ds;
  struct bounding_t bnd;
};

struct collect_t {
  long ftime;
  struct pldata_t cll, clm, clh;
  struct pldata_t rain1, rain2;
  struct pldata_t u, v;
  struct pldata_t z925, t925, rh925, t850, rh850, rh700, t500, z500, rh300;
};

static struct collect_t coll;


int
pl_isnull(struct pldata_t *plp)
{
  return NULL == plp->ds;
}

void
pl_nullify(struct pldata_t *plp)
{
  plp->gtime = 0L;
  plp->dslen = 0;
  plp->ds = NULL;
  // leave plp->bnd uninitialized; never touch it unless ds is present
}

  char *
printb(char *buf, size_t buflen, struct bounding_t *bnd)
{
  snprintf(buf, buflen, "N %zux%zu D %gx%g La %g:%g Lo %g:%g %u\n",
    bnd->ni, bnd->nj,
    bnd->di, bnd->dj,
    bnd->s, bnd->n,
    bnd->w, bnd->e,
    bnd->wraplon
    );
}

  int
pl_store(struct pldata_t *plp, long gtime, const struct grib2secs *gsp)
{
  int r;
  plp->gtime = gtime;
  plp->dslen = gsp->dslen;
  plp->ds = gsp->ds;
  r = decode_gds(gsp, &(plp->bnd));
  return r;
}

void
coll_clear(struct collect_t *collp)
{
  collp->ftime = 0L;
  pl_nullify(&(collp->cll));
  pl_nullify(&(collp->clm));
  pl_nullify(&(collp->clh));
  pl_nullify(&(collp->rain1));
  pl_nullify(&(collp->rain2));
  pl_nullify(&(collp->u));
  pl_nullify(&(collp->v));
  pl_nullify(&(collp->z925));
  pl_nullify(&(collp->t925));
  pl_nullify(&(collp->rh925));
  pl_nullify(&(collp->t850));
  pl_nullify(&(collp->rh850));
  pl_nullify(&(collp->rh700));
  pl_nullify(&(collp->t500));
  pl_nullify(&(collp->z500));
  pl_nullify(&(collp->rh300));
}

//--- end

// gribscan ライブラリから呼び返される関数。
  gribscan_err_t
checksec7(const struct grib2secs *gsp)
{
  // dimensions
  struct tm t;
  char sreftime[24];
  unsigned long iparm;
  double vlev, memb;
  long ftime, dura, gtime;
  // retrieve PDT metadata
  get_reftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
  iparm = get_parameter(gsp);
  ftime = get_ftime(gsp);
  vlev = get_vlevel(gsp);
  dura = get_duration(gsp);
  memb = get_perturb(gsp);
  gtime = ftime+dura;
  // filter
  if ((ftime==coll.ftime)&&(iparm==IPARM_CLL)) {
    pl_store(&(coll.cll), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_CLM)) {
    pl_store(&(coll.clm), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_CLH)) {
    pl_store(&(coll.clh), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_U)&&(vlev==VLEVEL_Z10M)) {
    pl_store(&(coll.u), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_V)&&(vlev==VLEVEL_Z10M)) {
    pl_store(&(coll.v), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_Z)&&(vlev==925.e2)) {
    pl_store(&(coll.z925), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_T)&&(vlev==925.e2)) {
    pl_store(&(coll.t925), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_RH)&&(vlev==925.e2)) {
    pl_store(&(coll.rh925), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_T)&&(vlev==850.e2)) {
    pl_store(&(coll.t850), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_RH)&&(vlev==850.e2)) {
    pl_store(&(coll.rh850), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_RH)&&(vlev==700.e2)) {
    pl_store(&(coll.rh700), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_T)&&(vlev==500.e2)) {
    pl_store(&(coll.t500), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_Z)&&(vlev==500.e2)) {
    pl_store(&(coll.z500), gtime, gsp);
  } else if ((ftime==coll.ftime)&&(iparm==IPARM_RH)&&(vlev==300.e2)) {
    pl_store(&(coll.rh300), gtime, gsp);
  } else if (iparm==IPARM_RAIN) {
    if (gtime <= coll.ftime) {
      if (pl_isnull(&coll.rain1)) {
        pl_store(&(coll.rain1), gtime, gsp);
      } else if (gtime > coll.rain1.gtime) {
        myfree(coll.rain1.ds);
	pl_store(&(coll.rain1), gtime, gsp);
      } else {
        goto END_SKIP;
      }
    } else {
      if (pl_isnull(&coll.rain2)) {
        pl_store(&(coll.rain2), gtime, gsp);
      } else if (gtime < coll.rain2.gtime) {
        myfree(coll.rain2.ds);
	pl_store(&(coll.rain2), gtime, gsp);
      } else {
        goto END_SKIP;
      }
    }
  } else {
    goto END_SKIP;
  }

  // report
  printf("b%s %6s f%-+5ld d%-+5ld v%-8s m%-+4.3g\n",
    sreftime, param_name(iparm), ftime, dura, level_name(vlev), memb);
  return GSE_OKAY;

END_SKIP:
  myfree(gsp->ds);
  return GSE_SKIP;
}

static const char Synopsis[] = "%s [-f{mins}] -c{coastfile} input ...\n";

int
main(int argc, const char **argv)
{
  const char *coastfile = NULL;
  int r = 0;
  coll_clear(&coll);
  for (int i=1; argv[i]; i++) {
    if (strhead(argv[i], "-f")) {
      coll.ftime = atoi(argv[i]+2);
    } else if (strhead(argv[i], "-c")) {
      coastfile = argv[i]+2;
    } else {
      fprintf(stderr, "file<%s>\n", argv[i]);
      r = grib2scan_by_filename(argv[i]);
    }
  }

  char sbuf[256];
  printb(sbuf, sizeof sbuf, &(coll.u.bnd));
  fputs(sbuf, stdout);
  setbbox(120.0f, 150.0f, 22.0f, 48.0f);

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
