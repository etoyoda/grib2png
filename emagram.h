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

extern double t2td(double t, double rh, double p);
extern int draw_emagram(obs_t *obs, size_t obs_count);
