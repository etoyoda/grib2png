#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
} obs_t;

size_t obs_size;
obs_t *obs = NULL;

  int
obs_init(void)
{
  obs_size = 8;
  obs = calloc(sizeof(obs_t), obs_size);
  for (int i=0; i<obs_size; i++) {
    obs[i].name[0] = '\0';
    obs[i].ttd_size = 0;
    obs[i].ttd = NULL;
    obs[i].uv_size = 0;
    obs[i].uv = NULL;
  }
  return 0;
}

  obs_t *
obs_find(const char *title)
{
  for (int i=0; i<obs_size; i++) {
    if (strcmp(obs[i].name, title)==0) {
      return obs+i;
    }
    if (obs[i].name[0] == '\0') {
      obs[i].ttd_size = 4096;
      obs[i].ttd = calloc(sizeof(obs_t), obs[i].ttd_size);
      for (int j=0; j<obs[i].ttd_size; j++) { obs[i].ttd[j].p = 0.0f; }
      obs[i].uv_size = 4096;
      obs[i].uv = calloc(sizeof(obs_t), obs[i].uv_size);
      for (int j=0; j<obs[i].uv_size; j++) { obs[i].uv[j].p = 0.0f; }
      strncpy(obs[i].name, title, sizeof(obs[i].name));
      obs[i].name[sizeof(obs[i].name)-1] = '\0';
      return obs+i;
    }
  }
  return NULL;
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
    smemb = strtok(NULL, ",");
    sname = strtok(NULL, ","); while (isspace(*sname)) { sname++; }
    sval = strtok(NULL, ",\r\n");
    // 観測識別名titleを生成
    char title[32];
    float ftime = (atof(sftime) + atof(sdura))/60.0f;
    snprintf(title, 32, "%s+%g %s", sreftime, ftime, sname);
    // 観測を探索
    obs_t *obsp = obs_find(title);
    if (obsp == NULL) continue; // 収容不能な数の観測は捨てる
    printf("%s:%s:%s:%s\n", title, sparm, svlev, sval);
  }
  fclose(fp);
  return 0;
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
  return 0;
}
