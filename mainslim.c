#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "gribscan.h"
#include "visual.h"
#include "mymalloc.h" // only for mymemstat();

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
  double vlev;
  long ftime, dura;
  get_reftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
  iparm = get_parameter(gsp);
  ftime = get_ftime(gsp);
  vlev = get_vlevel(gsp);
  dura = get_duration(gsp);
  // 長すぎる予報時間は最初に捨ててしまう
  if (ftime + dura > 360) goto END_SKIP;
  // 解析値も今の所使わないので捨てる
  if (ftime + dura == 0) goto END_SKIP;
  // 要素と面の複合フィルタ
  switch (iparm) {
  case IPARM_T:
    if (!(vlev == 50000.0 || vlev == 85000.0 || vlev == 92500.0
    || vlev == 101302.5
    )) {
      goto END_SKIP;
    }
    break;
  case IPARM_RH:
    if (!(vlev == 70000.0 || vlev == 85000.0 || vlev == 92500.0)) {
      goto END_SKIP;
    }
    break;
#if 0
  case IPARM_U:
  case IPARM_V:
    if (vlev != 101214.5) goto END_SKIP;
#endif
    break;
  case IPARM_VVPa:
    if (vlev != 70000.0) goto END_SKIP;
    break;
  case IPARM_RAIN:
    if (vlev != 101325.0) goto END_SKIP;
    break;
  case IPARM_Pmsl:
    if (vlev != 101324.0) goto END_SKIP;
    break;
  case IPARM_rVOR:
    if (!(vlev == 50000.0)) goto END_SKIP;
    break;
  case IPARM_Z:
    if (!(vlev == 300.e2 || vlev == 500.e2 || vlev == 925.e2)) goto END_SKIP;
    break;
  default:
    goto END_SKIP;
  }
  printf("b%s %6s f%-+5ld d%-+5ld v%-8.1lf\n",
    sreftime, param_name(iparm), ftime, dura, vlev);
  //r = convsec7(gsp);
  r = 0;
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
      r = GSE_OKAY;
    } else {
      r = grib2scan_by_filename(argv[i]);
      if (r != GSE_OKAY) break;
    }
  }
  return r;
}

  int
main(int argc, const char **argv)
{
  gribscan_err_t r;
  r = argscan(argc, argv);
  if (r == ERR_NOINPUT) {
    fprintf(stderr, "usage: %s data output ...\n", argv[0]);
  } else if (r != GSE_OKAY) {
    fprintf(stderr, "%s: exit(%u)\n", argv[0], r);
  }
  mymemstat();
  return r;
}
