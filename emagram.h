// emagram.h

typedef struct {
  float p;
  float x;
  float y;
} level_t;

typedef struct {
  char name[64];
  size_t ttd_size;
  size_t ttd_count;
  level_t *ttd;
  size_t uv_size;
  size_t uv_count;
  level_t *uv;
  size_t z_size;
  size_t z_count;
  level_t *z;
  float pmsl;
} obs_t;

typedef enum {
  GR_SKEWT = 0,
  GR_POTEMP = 1,
  GR_EMAGRAM = 2
} grtype_t;

extern double t2td(double t, double rh, double p);
extern int draw_emagram(obs_t *obs, size_t obs_count);
extern int setgraphtype(grtype_t gt);
extern int set_zw_profile(int i);
