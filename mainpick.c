#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "gribscan.h"
#include "visual.h"
#include "mymalloc.h" // only for mymemstat();

static const char *sfilter = "p[U]=p[V]=p[T]=p[RH]=|||,v10000>,g0=,&&";

struct ptlist_t {
  float lat;
  float lon;
  char tag[16];
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
    r = sscanf(line, "%g%g%s", &(cursor->lat), &(cursor->lon), cursor->tag);
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

/*
 * 改造予定地
 */
  gribscan_err_t
save_data(const struct grib2secs *gsp)
{
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
  char sreftime[24];
  unsigned long iparm;
  double vlev, memb;
  long ftime, dura;
  get_reftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
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
  printf("b%s %6s f%-+5ld d%-+5ld v%-8s m%-+4.3g\n",
    sreftime, param_name(iparm), ftime, dura, level_name(vlev), memb);
  r = save_data(gsp);
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
  fflush(stdout);
  return r;
}

  int
main(int argc, const char **argv)
{
  gribscan_err_t r;
  r = argscan(argc, argv);
  if (r == ERR_NOINPUT) {
    fprintf(stderr, "usage: %s [-ffilter][-pPICKPTS] input ...\n", argv[0]);
  } else if (r != GSE_OKAY) {
    fprintf(stderr, "%s: exit(%u)\n", argv[0], r);
  }
  mymemstat();
  return r;
}
