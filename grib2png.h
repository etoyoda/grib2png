// webメルカトル空間中の範囲座標を保持する構造体。
// ポインタを変数名 ofp で持つのが通例
typedef struct outframe_t {
  unsigned z; // zoom level
  unsigned xa; // min x axis
  unsigned xz; // max x axis
  unsigned ya; // min x axis
  unsigned yz; // max x axis
} outframe_t;

extern unsigned verbose;
extern unsigned debug;
extern void
mkfilename(char *filename, size_t fnlen, const struct grib2secs *gsp,
  const char *suffix);
extern gribscan_err_t
reproject(double *gbuf, const bounding_t *bp, const double *dbuf,
  const outframe_t *ofp);

// trapbin.c
extern FILE *text_fp;
extern int gflg_rvor_with_wd;
extern int gflg_jet_lower;
extern gribscan_err_t
check_traps(const struct grib2secs *gsp, double *dbuf,
  outframe_t *ofp, char **textv);

// trapsfc.c
extern int
magic_parameters(const char *arg);

extern gribscan_err_t
check_sfcanal(const struct grib2secs *gsp, double *dbuf,
  outframe_t *ofp, char **textv);
