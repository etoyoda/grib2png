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

struct grib2secs {
  const unsigned char *ids, *gds, *pds, *drs, *bms, *ds;
  size_t idslen, gdslen, pdslen, drslen, bmslen, dslen;
  unsigned discipline;
};

/* --- mainlogic.c --- */

extern gribscan_err_t
  argscan(int argc, const char **argv);

extern time_t timegm6(unsigned y, unsigned m, unsigned d,
  unsigned h, unsigned n, unsigned s);

extern gribscan_err_t
  checksec7(const struct grib2secs *gsp);

/* --- gribscan.c --- */

extern unsigned long
  ui4(const unsigned char *buf);
extern int
  si2(const unsigned char *buf);
extern unsigned
  ui2(const unsigned char *buf);
extern const char *
  showtime(char *buf, size_t size, const struct tm *t);
extern const char *
  param_name(unsigned iparm);

extern void
  get_reftime(struct tm *tp, const struct grib2secs *gsp);
extern long
  get_ftime(const struct grib2secs *gsp);
extern long
  get_duration(const struct grib2secs *gsp);
extern unsigned long
  get_parameter(const struct grib2secs *gsp);
extern double
  get_vlevel(const struct grib2secs *gsp);

extern gribscan_err_t
  scandata(const char *fnam);
