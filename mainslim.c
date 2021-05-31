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
  long ftime, dura, ftime2;
  get_reftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
  iparm = get_parameter(gsp);
  ftime = get_ftime(gsp);
  vlev = get_vlevel(gsp);
  dura = get_duration(gsp);
  ftime2 = ftime + dura;
  // 要素と面の複合フィルタ
  switch (iparm) {
  case IPARM_Z:
  case IPARM_RAIN:
  case IPARM_Pmsl:
    // 最重要面(500, 地上)の最重要要素は予報時間にかかわらず保存
    if ((vlev == 500.e2 || vlev > 1000.e2)
    && (ftime2 > 360 && (ftime2 % 720 == 0))) goto SAVE;
    break;
  case IPARM_U:
  case IPARM_V:
  case IPARM_RH:
    break;
  case IPARM_VVPa:
    if (!(vlev == 700.e2 || vlev == 300.e2)) goto END_SKIP;
    break;
  case IPARM_rDIV:
    if (!(vlev == 850.e2 || vlev == 250.e2)) goto END_SKIP;
    break;
  case IPARM_rVOR:
    if (!(vlev == 500.e2)) goto END_SKIP;
    break;
  default:
    goto END_SKIP;
  }
  // 基本 ft6h まで保存
  if (ftime2 > 360) goto END_SKIP;

SAVE:
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
