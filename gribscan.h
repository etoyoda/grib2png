#define HAS_TIMEGM 1

/* === gribscan.h === */

typedef enum gribscan_err_t {
  GSE_OKAY = 0,
  GSE_SKIP,
  GSE_JUSTWARN,
  ERR_UNSUPPORTED = 4,
  ERR_BADGRIB = 8,
  ERR_GENERIC = 16,
  ERR_IO = 'I',
  ERR_NOMEM = 'M',
  ERR_NOINPUT,
  ERR_TOOMANYCFG,
  ERR_OVERRUN,
  ERR_FSTACK
} gribscan_err_t;

typedef struct grib2secs {
  // comes from IS (indicator section) of the GRIB message
  unsigned discipline;
  size_t msglen;
  unsigned char *ids, *gds, *pds, *drs, *bms, *ds;
  size_t idslen, gdslen, pdslen, drslen, bmslen, dslen;
  void *omake;
} grib2secs_t;

#define VLEVEL_Z2M 101302.5
#define VLEVEL_Z10M 101214.5

typedef enum iparm_t {
  IPARM_T      = 0x000000,
  IPARM_papT   = 0x000003,
  IPARM_dT     = 0x000006,
  IPARM_RH     = 0x000101,
  IPARM_RR1H   = 0x000107,
  IPARM_RAIN   = 0x000108,
  IPARM_WD     = 0x000200,
  IPARM_WINDS  = 0x000201,
  IPARM_U      = 0x000202,
  IPARM_V      = 0x000203,
  IPARM_PSI    = 0x000204,
  IPARM_CHI    = 0x000205,
  IPARM_VVPa   = 0x000208,
  IPARM_rVOR   = 0x00020c,
  IPARM_rDIV   = 0x00020d,
  IPARM_Pres   = 0x000300,
  IPARM_Pmsl   = 0x000301,
  IPARM_Z      = 0x000305,
  IPARM_CLA    = 0x000601,
  IPARM_CLL    = 0x000603,
  IPARM_CLM    = 0x000604,
  IPARM_CLH    = 0x000605,
} iparm_t;

// map projection parameters
// currently only equidistant cylindrical is supported.
typedef struct bounding_t {
  double n, w, s, e;
  double di, dj;
  size_t ni, nj;
  int wraplon;
} bounding_t;

/* --- mainlogic.c --- */

extern gribscan_err_t
  argscan(int argc, const char **argv);

extern gribscan_err_t
  checksec7(const grib2secs_t *gsp);

/* --- gribscan.c --- */

extern long si4(const unsigned char *buf);
extern unsigned long ui4(const unsigned char *buf);
extern int si2(const unsigned char *buf);
extern unsigned ui2(const unsigned char *buf);
extern double float40(const unsigned char *buf);
extern double float32(const unsigned char *buf);
extern unsigned
  unpackbits(const unsigned char *buf, size_t nbits, size_t pos);

extern void
  get_reftime(struct tm *tp, const grib2secs_t *gsp);
extern long
  get_ftime(const grib2secs_t *gsp);
extern long
  get_duration(const grib2secs_t *gsp);
extern unsigned long
  get_parameter(const grib2secs_t *gsp);
extern double
  get_vlevel(const grib2secs_t *gsp);
extern double
  get_perturb(const grib2secs_t *gsp);
extern size_t
  get_npixels(const grib2secs_t *gsp);

gribscan_err_t
  set_parameter(grib2secs_t *gsp, iparm_t iparm);

extern gribscan_err_t
  decode_ds(const grib2secs_t *gsp, double *dbuf,
  void (*adjust_scales)(iparm_t param, int *scale_e, int *scale_d));
extern gribscan_err_t
  decode_gds(const grib2secs_t *gsp, bounding_t *bp);
extern double
  bp_lat(bounding_t *bp, double j);
extern double
  bp_lon(bounding_t *bp, double i);

extern const char *
  showtime(char *buf, size_t size, const struct tm *t);
extern const char *
  param_name(unsigned long iparm);

extern const char *level_name(double vlev);

extern grib2secs_t *dup_grib2secs(const grib2secs_t *gsp);
extern void del_grib2secs(grib2secs_t *gsp);

extern gribscan_err_t
  grib2scan_by_filename(const char *fnam);

extern gribscan_err_t
  gribscan_filter(const char *sfilter,
  iparm_t param,
  long ftime,
  long dura,
  double vlev,
  double memb);
