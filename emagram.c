#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  obs_size = 256;
  obs = malloc(sizeof(obs_t) * obs_size);
  for (int i=0; i<obs_size; i++) {
    obs[i].name[0] = 0;
    obs[i].ttd_size = 0;
    obs[i].ttd = NULL;
    obs[i].uv_size = 0;
    obs[i].uv = NULL;
  }
  return 0;
}

  int
loadfile(const char *fnam)
{
  FILE *fp;
  char buf[1024];
  fp=fopen(fnam, "rt");
  if(fp==NULL) { perror(fnam); return -1; }
  while (fgets(buf, sizeof buf, fp) != NULL) {
    char *sreftime, *sparm, *sftime, *sdura, *svlev, *smemb, *sname, *sval;
    sreftime = strtok(buf, ",");
    sparm = strtok(NULL, ",");
    sftime = strtok(NULL, ",");
    sdura = strtok(NULL, ",");
    svlev = strtok(NULL, ",");
    smemb = strtok(NULL, ",");
    sname = strtok(NULL, ",");
    sval = strtok(NULL, ",\r\n");
    printf("%s: %s: %s\n", sreftime, sftime, sval);
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
