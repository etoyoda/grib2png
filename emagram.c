#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define streq(a,b) (strcmp((a),(b))==0)

typedef struct {
  float p;
  float x;
  float y;
} level_t;

typedef struct {
  char name[32];
  size_t ttd_size;
  level_t *ttd;
  size_t uv_size;
  level_t *uv;
  size_t z_size;
  level_t *z;
} obs_t;

size_t obs_size;
obs_t *obs = NULL;

  int
obs_init(void)
{
  obs_size = 8;
  obs = calloc(sizeof(obs_t), obs_size);
  for (size_t i=0; i<obs_size; i++) {
    obs[i].name[0] = '\0';
    obs[i].ttd_size = 0;
    obs[i].ttd = NULL;
    obs[i].uv_size = 0;
    obs[i].uv = NULL;
    obs[i].z_size = 0;
    obs[i].z = NULL;
  }
  return 0;
}

  obs_t *
obs_find(const char *title)
{
  for (size_t i=0; i<obs_size; i++) {
    obs_t *obsp = &(obs[i]);
    if (streq(obsp->name, title)) {
      return obsp;
    }
    if (obsp->name[0] == '\0') {
      obsp->ttd_size = 4096;
      obsp->ttd = calloc(sizeof(obs_t), obsp->ttd_size);
      for (int j=0; j<obsp->ttd_size; j++) { obsp->ttd[j].p = 0.0f; }
      obsp->uv_size = 4096;
      obsp->uv = calloc(sizeof(obs_t), obsp->uv_size);
      for (int j=0; j<obsp->uv_size; j++) { obsp->uv[j].p = 0.0f; }
      strncpy(obsp->name, title, sizeof(obsp->name));
      obsp->name[sizeof(obsp->name)-1] = '\0';
      return obsp;
    }
  }
  return NULL;
}

#define VLEV_MSL 1013.25f
#define VLEV_Z2M 1013.125f
#define VLEV_Z10M 1012.125f

  float
vlev(const char *svlev)
{
  if (svlev[0]=='p') { return (float)atof(svlev+1);
  } else if (streq(svlev, "msl")) { return VLEV_MSL;
  } else if (streq(svlev, "z2")) { return VLEV_Z2M;
  } else if (streq(svlev, "z10")) { return VLEV_Z10M;
  } else { return nanf("");
  }
}

  level_t *
level_find(level_t *levels, size_t sz, float p)
{
  for (size_t i=0; i<sz; i++) {
    if (levels[i].p == p) { return &(levels[i]); }
    if (levels[i].p == 0.0f) {
      levels[i].p = p;
      levels[i].x = levels[i].y = nanf("");
      return &(levels[i]);
    }
  }
  return NULL;
}

  int
obs_store(obs_t *obsp, const char *sparm, const char *svlev, const char *sval)
{
  float p = vlev(svlev);
  float val = atof(sval);
  if (streq("T", sparm)) {
    level_t *lp = level_find(obsp->ttd, obsp->ttd_size, p);
    if (lp==NULL) return -1;
    lp->x = val;
    return 0;
  } else if (streq("RH", sparm)) {
    level_t *lp = level_find(obsp->ttd, obsp->ttd_size, p);
    if (lp==NULL) return -1;
    lp->y = val;
    return 0;
  }
  return -1;
}

  int
loadfile(const char *fnam)
{
  FILE *fp;
  char buf[1024];
  fp=fopen(fnam, "rt");
  if(fp==NULL) { perror(fnam); return -1; }
  while (fgets(buf, sizeof buf, fp) != NULL) {
    // 入力の各行をCSV 8列に分解
    char *sreftime, *sparm, *sftime, *sdura, *svlev, *smemb, *sname, *sval;
    sreftime = strtok(buf, ",");
    sparm = strtok(NULL, ","); while (isspace(*sparm)) { sparm++; }
    sftime = strtok(NULL, ",");
    sdura = strtok(NULL, ",");
    svlev = strtok(NULL, ",");
    { char *p; p=svlev;
      while ((*p)&&!isspace(*p)) { p++; };
      if (isspace(*p)) { *p='\0'; }
    }
    smemb = strtok(NULL, ",");
    sname = strtok(NULL, ","); while (isspace(*sname)) { sname++; }
    sval = strtok(NULL, ",\r\n");
    // 観測識別名titleを生成
    char title[32];
    float ftime = (atof(sftime) + atof(sdura))/60.0f;
    snprintf(title, 32, "%s+%g %s", sreftime, ftime, sname);
    // プロファイルを探索
    obs_t *obsp = obs_find(title);
    if (obsp == NULL) continue; // 収容不能な数のプロファイルは捨てる
    obs_store(obsp, sparm, svlev, sval);
  }
  fclose(fp);
  return 0;
}

  int
obs_print(void)
{
  for (size_t i=0; i<obs_size; i++) {
    obs_t *obsp = &(obs[i]);
    if (obsp->name[0]=='\0') break;
    printf("= obs #%zu (%s)\n", i, obsp->name);
    for (size_t j=0; j<obsp->ttd_size; j++) {
      if (obsp->ttd[j].p==0.0f) break;
      printf("%8.1f %8.1f %8.1f\n", obsp->ttd[j].p,
      obsp->ttd[j].x, obsp->ttd[j].y);
    }
  }
}

  int
main(int argc, const char **argv)
{
  obs_init();
  for (int i=1; i<argc; i++) {
    if (argv[i][0] == '-') {
      fprintf(stderr, "unknown option %s\n", argv[i]);
    } else {
      loadfile(argv[i]);
    }
  }
  obs_print();
  return 0;
}
