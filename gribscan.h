#define HAS_TIMEGM 1

/* === gribscan.h === */

typedef enum gribscan_err_t {
  GSE_OKAY = 0,
  GSE_SKIP,
  GSE_JUSTWARN,
  ERR_NOINPUT,
  ERR_OUTPAT,
  ERR_REGEX,
  ERR_TOOMANYCFG,
  ERR_OVERRUN,
  ERR_NOMEM,
  ERR_IO,
  ERR_BADGRIB
} gribscan_err_t;

/* --- mainlogic.c --- */

struct cfgout_t {
  unsigned ctr;
  unsigned gen;
  unsigned par;
  unsigned ft;
  unsigned lev;
  time_t rt;
  float *acc;
  float *wgt;
};

extern enum gribscan_err_t
  argscan(int argc, const char **argv);

extern time_t timegm6(unsigned y, unsigned m, unsigned d,
  unsigned h, unsigned n, unsigned s);

/* --- gribscan.c --- */

enum gribscan_err_t
  scandata(const char *fnam);

extern const char *showtime(char *buf, size_t size, const struct tm *t);
