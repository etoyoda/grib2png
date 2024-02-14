#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include "gribscan.h"
#include "visual.h"
#include "mymalloc.h" // only for mymemstat();

// -dFILENAME オプションを指定すると dumpfile が非ヌルになり出力する
static FILE *dumpfile = NULL;

static const char *sfilter = "p[U]=p[V]=p[T]=p[RH]=|||,v10000>,g0=,&&";

struct ptlist_t {
  float lat;
  float lon;
  char tag[32];
} *ptlist = NULL;

const unsigned maxline = 256;
const unsigned maxpts = 256;

/* 抽出すべき地点リストを入力 fp から読み込む。
 */
  gribscan_err_t
ptlist_read(FILE *fp)
{
  char line[maxline];
  unsigned nl;
  ptlist = calloc(maxpts, sizeof(ptlist[0]));
  if (ptlist == NULL) return ERR_NOMEM;
  nl = 0;
  while (fgets(line, sizeof line, fp) != NULL) {
    int r;
    struct ptlist_t *cursor;
    cursor = ptlist + nl;
    r = sscanf(line, "%g%g%31s", &(cursor->lat), &(cursor->lon), cursor->tag);
    if (r < 3) {
      fprintf(stderr, "W Bad format <%s>\n", line);
    } else {
      nl++;
    }
    if (nl >= (maxpts - 1)) {
      fprintf(stderr, "W Number of points cannot exceed %u\n", maxpts - 2);
      break;
    }
  }
  return GSE_OKAY;
}

/* 抽出すべき地点リストを文字列 filename で指定されるファイルから読み込む。
 * 引数 filename がヌルポインタまたは空文字列であれば、標準入力を代用。
 * 複数回呼び出すと、初回実行時（ptlist がヌルポインタの時）だけ動く。
 */
  gribscan_err_t
ptlist_load(const char *filename)
{
  gribscan_err_t r;
  if (ptlist) return GSE_OKAY;
  if ((filename == NULL) || (filename[0] == '\0')) {
    if (isatty(fileno(stdin))) {
      fputs("Reading points(lat lon name) from stdin:\n", stderr);
    }
    r = ptlist_read(stdin);
  } else {
    FILE *fp;
    fp = fopen(filename, "rt");
    if (fp == NULL) return ERR_IO;
    r = ptlist_read(fp);
    fclose(fp);
  }
  return r;
}

  double
interpol(const double *dbuf, const bounding_t *bp, double lat, double lon)
{
  double ri, rj, fi, fj, weight[4];
  size_t ijofs[4];
  int ceil_ri, floor_ri;
  if (lon < bp->w) { lon += 360.0; } 
  if (lat > bp->n || lat < bp->s) { return nan(""); }
  ri = (lon - bp->w) * (bp->ni - 1) / (bp->e - bp->w);
  if ((lon > bp->e) && !bp->wraplon) { return nan(""); }
  rj = (bp->n - lat) * (bp->nj - 1) / (bp->n - bp->s);
  fi = ri - floor(ri);
  fj = rj - floor(rj);
  // これは起こらないとはおもうんだけど
  if ((floor_ri = floor(ri)) > (bp->ni - 1)) { floor_ri = bp->ni - 1; }
  if ((ceil_ri = ceil(ri)) > (bp->ni - 1)) { ceil_ri = 0; }
  ijofs[0] = floor_ri + floor(rj) * bp->ni;
  weight[0] = 1.0 - hypot(fi, fj);
  if (weight[0] < 0.0) { weight[0] = 0.0; }
  ijofs[1] =  ceil_ri + floor(rj) * bp->ni;
  weight[1] = 1.0 - hypot(1 - fi, fj);
  if (weight[1] < 0.0) { weight[1] = 0.0; }
  ijofs[2] = floor_ri +  ceil(rj) * bp->ni;
  weight[2] = 1.0 - hypot(fi, 1 - fj);
  if (weight[2] < 0.0) { weight[2] = 0.0; }
  ijofs[3] =  ceil_ri +  ceil(rj) * bp->ni;
  weight[3] = 1.0 - hypot(1 - fi, 1 - fj);
  if (weight[3] < 0.0) { weight[3] = 0.0; }
  return (dbuf[ijofs[0]] * weight[0] + dbuf[ijofs[1]] * weight[1]
    + dbuf[ijofs[2]] * weight[2] + dbuf[ijofs[3]] * weight[3])
    / (weight[0] + weight[1] + weight[2] + weight[3]);
}

  // 特定パラメタの場合十進尺度 scale_d を補正する
  // 注： mainlogic.c とは値が異なる
  void
adjust_scales(iparm_t param, int *scale_e, int *scale_d)
{
  switch (param) {
  // 海面気圧: 1 hPa 単位に変換
  // 典型的値域: 900..1090
  case IPARM_Pmsl:
    *scale_d += 2;
    break;
  // 渦度または発散: 1e-6/s 単位に変換
  // 典型的値域: -1000..1000
  case IPARM_rDIV:
  case IPARM_rVOR:
    *scale_d -= 6;
    break;
  // 気温または露点: 1 K 単位に変換
  // 典型的値域: 250..320
  case IPARM_T:
  case IPARM_dT:
    break;
  // 風速: 1 m/s 単位に変換
  // 典型的値域: -100..100
  case IPARM_U:
  case IPARM_V:
    break;
  // 積算降水量: 1 mm 単位に変換
  // 典型的値域: 0..1000
  case IPARM_RAIN:
    break;
default:
    break;
  }
}

/* ptlist に記載された地点リストについて gsp からGPVを抜き出して印字。
 */
  gribscan_err_t
save_data(const struct grib2secs *gsp, const char *title)
{
  bounding_t b;
  unsigned i;
  size_t npixels;
  double *dbuf;
  gribscan_err_t r;
  if ((npixels = get_npixels(gsp)) == 0) {
    fprintf(stderr, "DRS missing\n");
    return ERR_BADGRIB;
  }
  //--- begin memory section
  if ((dbuf = malloc(sizeof(double) * npixels)) == NULL) {
    fprintf(stderr, "malloc failed %zu\n", npixels);
    return ERR_NOMEM;
  }
  r = decode_ds(gsp, dbuf, adjust_scales);
  if (r != GSE_OKAY) return r;
  if (dumpfile) {
    size_t r;
    r = fwrite(dbuf, sizeof(dbuf[0]), npixels, dumpfile);
    if (r != npixels) { perror("gribpick.fwrite"); }
  }
  decode_gds(gsp, &b);
  for (i = 0; i < maxpts; i++) {
    double x;
    if (ptlist[i].tag[0] == '\0') break;
    x = interpol(dbuf, &b, ptlist[i].lat, ptlist[i].lon);
    printf("%s,%8s,%8.5g\n", title, ptlist[i].tag, x);
  }
  return GSE_OKAY;
}

/* 第1節〜第7節のセット gsp について、
 * 必要であれば convsec7() を呼び出す。
 * この関数は gsp->ds を破棄または保存する。
 */
  gribscan_err_t
checksec7(const struct grib2secs *gsp)
{
  gribscan_err_t r;
  struct tm t;
  char title[maxline];
  char sreftime[24];
  unsigned long iparm;
  double vlev, memb;
  long ftime, dura;
  get_reftime(&t, gsp);
  iparm = get_parameter(gsp);
  ftime = get_ftime(gsp);
  vlev = get_vlevel(gsp);
  dura = get_duration(gsp);
  memb = get_perturb(gsp);

  // 要素と面の複合フィルタ
  switch (gribscan_filter(sfilter, iparm, ftime, dura, vlev, memb)) {
    case ERR_FSTACK: 
    case GSE_SKIP: goto END_SKIP; break;
    default:
    case GSE_OKAY: goto SAVE; break;
  }

SAVE:
  showtime(sreftime, sizeof sreftime, &t);
  sprintf(title, "%s,%6s,%-+5ld,%-+5ld,%-8s,%-+4.3g",
    sreftime, param_name(iparm), ftime, dura, level_name(vlev), memb);
  r = save_data(gsp, title);
  goto END_NORMAL;

END_SKIP:
  r = GSE_SKIP;
END_NORMAL:
  myfree(gsp->ds);
  return r;
}

/* コマンドライン引数 argc, argv を左からチェックして、
 * 要すれば入力ファイルを開く。
 * （いずれコマンドラインオプションを処理する建設予定地）
 * grib2scan_by_filename() から checksec7() が第7節の数だけ呼び出される。
 */
  gribscan_err_t
argscan(int argc, const char **argv)
{
  gribscan_err_t r = ERR_NOINPUT;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'p') {
        r = ptlist_load(argv[i] + 2);
        if (r != GSE_OKAY) goto BARF;
      } else if (argv[i][1] == 'f') {
        sfilter = argv[i] + 2;
      } else if (argv[i][1] == 'd') {
        dumpfile = fopen(argv[i] + 2, "wb");
      } else {
        fprintf(stderr, "%s: unknown option\n", argv[i]);
        r = GSE_JUSTWARN;
        goto BARF;
      }
    } else {
      r = ptlist_load(NULL);
      if (r != GSE_OKAY) goto BARF;
      r = grib2scan_by_filename(argv[i]);
      if (r != GSE_OKAY) goto BARF;
    }
  }
BARF:
  if (dumpfile) { fclose(dumpfile); dumpfile = NULL; }
  fflush(stdout);
  return r;
}

  int
main(int argc, const char **argv)
{
  gribscan_err_t r;
  r = argscan(argc, argv);
  if (r == ERR_NOINPUT) {
    fprintf(stderr, "usage: %s [-ffilter][-pPICKPTS][-dDUMPFILE] input ...\n", argv[0]);
  } else if (r != GSE_OKAY) {
    fprintf(stderr, "%s: exit(%u)\n", argv[0], r);
  }
  mymemstat();
  return r;
}
