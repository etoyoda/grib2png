#define HAS_TIMEGM 1

/* === gribscan.h === */

typedef enum gribscan_err_t {
  GSE_OKAY = 0,
  GSE_SKIP,
  GSE_JUSTWARN,
  ERR_NOMEM,
  ERR_BADGRIB,
  ERR_NOINPUT,
  ERR_OUTPAT,
  ERR_REGEX,
  ERR_TOOMANYCFG,
  ERR_OVERRUN,
  ERR_IO
} gribscan_err_t;

struct grib2secs {
  // ISはここだけを保存している
  unsigned discipline;
  size_t msglen;
  unsigned char *ids, *gds, *pds, *drs, *bms, *ds;
  size_t idslen, gdslen, pdslen, drslen, bmslen, dslen;
  // デコード結果
  size_t npixels;
};

typedef enum iparm {
  IPARM_T      = 0x000000,
  IPARM_RH     = 0x000101,
  IPARM_RAIN   = 0x000108,
  IPARM_Pmsl   = 0x000301,
  IPARM_Z      = 0x000305,
} iparm_t;

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
  param_name(unsigned long iparm);

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
extern size_t
  get_npixels(const struct grib2secs *gsp);
extern gribscan_err_t
  decode_ds(const struct grib2secs *gsp, double *dbuf);

extern unsigned
  unpackbits(const unsigned char *buf, size_t nbits, size_t pos);

extern gribscan_err_t
  grib2scan_by_filename(const char *fnam);
