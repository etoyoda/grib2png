// visual.h

typedef enum palette_t {
  PALETTE_GSI,
  PALETTE_Pmsl,
  PALETTE_T,
  PALETTE_papT,
  PALETTE_RAIN6,
  PALETTE_RH,
  PALETTE_Z
} palette_t;

  extern int
gridsave(double *gbuf, size_t owidth, size_t oheight, palette_t pal,
  const char *filename, char **textv);
