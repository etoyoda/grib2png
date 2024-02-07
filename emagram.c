#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "emagram.h"

int verbose = 0;

#define streq(a,b) (strcmp((a),(b))==0)

size_t obs_size, obs_count;
obs_t *obs = NULL;

  int
obs_init(void)
{
  obs_size = 8;
  obs = calloc(sizeof(obs_t), obs_size);
  for (size_t i=0; i<obs_size; i++) {
    obs[i].name[0] = '\0';
    obs[i].ttd_size = 0;
    obs[i].ttd_count = 0;
    obs[i].ttd = NULL;
    obs[i].uv_size = 0;
    obs[i].uv_count = 0;
    obs[i].uv = NULL;
    obs[i].z_size = 0;
    obs[i].z_count = 0;
    obs[i].z = NULL;
    obs[i].pmsl = 0.0f;
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
      // ttd
      obsp->ttd_size = 4096;
      obsp->ttd = calloc(sizeof(obs_t), obsp->ttd_size);
      for (int j=0; j<obsp->ttd_size; j++) { obsp->ttd[j].p = 0.0f; }
      // z
      obsp->z_size = 4096;
      obsp->z = calloc(sizeof(obs_t), obsp->z_size);
      for (int j=0; j<obsp->z_size; j++) { obsp->z[j].p = 0.0f; }
      // uv
      obsp->uv_size = 4096;
      obsp->uv = calloc(sizeof(obs_t), obsp->uv_size);
      for (int j=0; j<obsp->uv_size; j++) { obsp->uv[j].p = 0.0f; }
      // title
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
    if (p == VLEV_Z2M) { return 0; } // skip for now
    level_t *lp = level_find(obsp->ttd, obsp->ttd_size, p);
    if (lp==NULL) return -1;
    lp->x = val;
    return 0;
  } else if (streq("RH", sparm)) {
    if (p == VLEV_Z2M) { return 0; } // skip for now
    level_t *lp = level_find(obsp->ttd, obsp->ttd_size, p);
    if (lp==NULL) return -1;
    lp->y = val;
    return 0;
  } else if (streq("Z", sparm)) {
    level_t *lp = level_find(obsp->z, obsp->z_size, p);
    if (lp==NULL) return -1;
    lp->x = val;
    return 0;
  } else if (streq("U", sparm)) {
    if (p == VLEV_Z10M) { return 0; } // skip for now
    level_t *lp = level_find(obsp->uv, obsp->uv_size, p);
    if (lp==NULL) return -1;
    lp->x = val;
    return 0;
  } else if (streq("V", sparm)) {
    if (p == VLEV_Z10M) { return 0; } // skip for now
    level_t *lp = level_find(obsp->uv, obsp->uv_size, p);
    if (lp==NULL) return -1;
    lp->y = val;
    return 0;
  } else if (streq("Pmsl", sparm)) {
    obsp->pmsl = val;
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
    char title[64];
    float ftime = (atof(sftime) + atof(sdura))/60.0f;
    int plus = '+';
    if (*smemb!='-') plus = '-';
    snprintf(title, 64, "%s%c%02.0f %s", sreftime, plus, ftime, sname);
    // プロファイルを探索
    obs_t *obsp = obs_find(title);
    if (obsp == NULL) continue; // 収容不能な数のプロファイルは捨てる
    obs_store(obsp, sparm, svlev, sval);
  }
  fclose(fp);
  return 0;
}

  int
level_compare(const void *ap, const void *bp)
{
  const level_t *la = ap;
  const level_t *lb = bp;
  if (la->p > lb->p) return -1;
  if (la->p < lb->p) return 1;
  return 0;
}

// 測高公式により高度差を与える
  float
dz_hydro(float t1, float t2, float p1, float p2)
{
  const float R_g = 287.0f / 9.80665f;  // 乾燥空気の気体定数 [J/kg/K] / 重力 [m/s2]
  // 仮温度 [K], 湿度は面倒なので 60% と仮定している
  float tv = (t1 + t2) * 0.5f;
  tv += 0.4f * powf(2.0f, ((tv - 273.15f) * 0.1f));
  return R_g * tv * logf(p1 / p2);
}

// T-Z 整合性をチェックして必要なら温度中間点を挿入
  int
fix_t_profile(obs_t *obsp)
{
  size_t jlast = obsp->ttd_count-1;
  // 実ゾンデデータを処理した場合の対策：
  // 鉛直30層（経験的閾値）以上あればこの関数は何もしない
  if (jlast > 30u) return 0;
  for (size_t j=1; j<jlast; j++) {
    size_t k0, k1, k2, k3;
    // k0 := j-1 と同じ気圧を持つ点
    for (k0=0; k0<(obsp->z_count-3); k0++) {
      if (obsp->z[k0].p==obsp->ttd[j-1].p) goto K0_FOUND;
    }
    goto NEXT_J; // 探索失敗 - 当該jの処理を打ち切り
K0_FOUND:
    // k1 := j と同じ気圧を持つ点
    for (k1=k0+1; k1<(obsp->z_count-2); k1++) {
      if (obsp->z[k1].p==obsp->ttd[j].p) goto K1_FOUND;
    }
    goto NEXT_J; // 探索失敗 - 当該jの処理を打ち切り
K1_FOUND:
    // k2 := j+1 と同じ気圧を持つ点
    for (k2=k1+1; k2<obsp->z_count-1; k2++) {
      if (obsp->z[k2].p==obsp->ttd[j+1].p) goto K2_FOUND;
    }
    goto NEXT_J; // 探索失敗 - 当該jの処理を打ち切り
K2_FOUND:
    // k3 := j+2 と同じ気圧を持つ点
    for (k3=k2+1; k3<obsp->z_count; k3++) {
      if (obsp->z[k3].p==obsp->ttd[j+2].p) goto K3_FOUND;
    }
    goto NEXT_J; // 探索失敗 - 当該jの処理を打ち切り
    float dz_est, dz;
K3_FOUND:
    dz_est = dz_hydro(obsp->ttd[j].x, obsp->ttd[j+1].x, obsp->ttd[j].p, obsp->ttd[j+1].p);
    dz = obsp->z[k2].x - obsp->z[k1].x;
    if ((dz > 0.998f * dz_est)&&(dz < 1.002f * dz_est)) goto NEXT_J;
    if (verbose) {
      printf("p %g %g t %g %g z %g %g dz %g %g %5.3f\n",
      obsp->ttd[j].p, obsp->ttd[j+1].p,
      obsp->ttd[j].x, obsp->ttd[j+1].x,
      obsp->z[k1].x, obsp->z[k2].x,
      dz, dz_est, dz/dz_est
      );
    }
    jlast++;
    // とりあえず中間点に温度特異点を挿入
    // もうちょっと場所は工夫すべき
    obsp->ttd[jlast].p = 0.5 * (obsp->ttd[j].p + obsp->ttd[j+1].p);
    obsp->ttd[jlast].y = 0.5 * (obsp->ttd[j].y + obsp->ttd[j+1].y);
    obsp->ttd[jlast].x = 0.5 * (obsp->ttd[j].x + obsp->ttd[j+1].x)
      * ((dz*2.0f-dz_est)/dz_est);
NEXT_J: ;
  }
  if (jlast > obsp->ttd_count-1) {
    obsp->ttd_count = jlast+1;
    return 1;
  }
  return 0;
}

  int
obs_conv(void)
{
  obs_count=obs_size;
  for (size_t i=0; i<obs_size; i++) {
    obs_t *obsp = &(obs[i]);
    if (obsp->name[0]=='\0') {
      obs_count = i;
      break;
    }
    // zの有効レベル数obsp->z_countを数えてソート
    obsp->z_count=obsp->z_size;
    for (size_t n=0; n<obsp->z_size; n++) {
      if (obsp->z[n].p==0.0f) { obsp->z_count=n; break; }
    }
    qsort(obsp->z, obsp->z_count, sizeof(level_t), level_compare);
    // uvの有効レベル数obsp->uv_countを数えてソート
    obsp->uv_count=obsp->uv_size;
    for (size_t n=0; n<obsp->uv_size; n++) {
      if (obsp->uv[n].p==0.0f) { obsp->uv_count=n; break; }
    }
    qsort(obsp->uv, obsp->uv_count, sizeof(level_t), level_compare);
    // ttdの有効レベル数obsp->ttd_countを数えてソート
    obsp->ttd_count=obsp->ttd_size;
    for (size_t n=0; n<obsp->ttd_size; n++) {
      if (obsp->ttd[n].p==0.0f) { obsp->ttd_count=n; break; }
    }
    qsort(obsp->ttd, obsp->ttd_count, sizeof(level_t), level_compare);
    // 要素変換: RH を TD でおきかえる
    for (size_t j=0; j<obsp->ttd_count; j++) {
      obsp->ttd[j].y = t2td(obsp->ttd[j].x, obsp->ttd[j].y, obsp->ttd[j].p);
    }
    // T-Z整合性チェック
    if (fix_t_profile(obsp)) {
      // 真を返した場合は温度特異点が追加されているのでソートしなおし
      qsort(obsp->ttd, obsp->ttd_count, sizeof(level_t), level_compare);
    }
  }
  return 0;
}

  int
obs_print(void)
{
  for (size_t i=0; i<obs_size; i++) {
    obs_t *obsp = &(obs[i]);
    if (obsp->name[0]=='\0') break;
    printf("= obs #%zu (%s)\n", i, obsp->name);
    printf("== Pmsl %8.1f\n", obsp->pmsl);
    printf("== t td\n");
    for (size_t j=0; j<obsp->ttd_size; j++) {
      if (obsp->ttd[j].p==0.0f) break;
      printf("%8.1f %8.1f %8.1f\n", obsp->ttd[j].p,
      obsp->ttd[j].x, obsp->ttd[j].y);
    }
    printf("== z\n");
    for (size_t j=0; j<obsp->z_size; j++) {
      if (obsp->z[j].p==0.0f) break;
      printf("%8.1f %8.1f\n", obsp->z[j].p,
      obsp->z[j].x);
    }
    printf("== u v\n");
    for (size_t j=0; j<obsp->uv_size; j++) {
      if (obsp->uv[j].p==0.0f) break;
      printf("%8.1f %8.1f %8.1f\n", obsp->uv[j].p,
      obsp->uv[j].x, obsp->uv[j].y);
    }
  }
  return 0;
}

  int
main(int argc, const char **argv)
{
  obs_init();
  for (int i=1; i<argc; i++) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'e') {
        setgraphtype(GR_EMAGRAM);
      } else if (argv[i][1] == 'p') {
        setgraphtype(GR_POTEMP);
      } else if (argv[i][1] == 'v') {
        verbose++;
      } else {
        fprintf(stderr, "unknown option %s\n", argv[i]);
      }
    } else {
      loadfile(argv[i]);
    }
  }
  obs_conv();
  draw_emagram(obs, obs_count);
  if (verbose > 1) obs_print();
  return 0;
}
