// visual.h

typedef enum palette_t {
  PALETTE_GSI,
  PALETTE_Pmsl,
  PALETTE_T,
  PALETTE_papT,
  PALETTE_RAIN6,
  PALETTE_RH,
  PALETTE_rVOR,
  PALETTE_VVPa,
  PALETTE_WINDS,
  PALETTE_WINDS_SFC,
  PALETTE_Z,
  PALETTE_WD
} palette_t;

  extern int
gridsave(double *gbuf, size_t owidth, size_t oheight, palette_t pal,
  const char *filename, char **textv);
