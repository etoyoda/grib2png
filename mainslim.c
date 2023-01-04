#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include "gribscan.h"
#include "visual.h"
#include "mymalloc.h" // only for mymemstat();

static struct ofile_t {
  const char *fnam;
  FILE *ofp;
  size_t pos;
  unsigned char *ids;
  unsigned char *gds;
} ofile = {
  NULL,
  NULL, 0,
  NULL, NULL
};

static const char *sfilter = "p[VVPa]=v70000=v30000=|&,p[rDIV]=v85000=v25000=|&,p[rVOR]=v50000=&,p[U]=p[V]=p[T]=p[RH]=p[Z]=||||,|||,g361<&,p[Z]=v50000=&p[RAIN]=p[Pmsl]=||,g720%0=g360=|&,|";

  gribscan_err_t
save_open(const char *ofnam)
{
  unsigned char sec0[16] = { 'G','R','I','B',
    0,0,0,2,
    0,0,0,0, 0,0,0,0  // size of msg
  };
  if (ofnam) {
    ofile.fnam = ofnam;
  } else {
    if (ofile.fnam) {
      return GSE_OKAY;
    }
    ofile.fnam = "distilled.bin";
  }
  if (NULL == (ofile.ofp = fopen(ofile.fnam, "wb"))) {
    return ERR_IO;
  }
  if (1 != fwrite(sec0, sizeof sec0, 1, ofile.ofp)) {
    return ERR_IO;
  }
  ofile.pos = sizeof sec0;
  return GSE_OKAY;
}

  gribscan_err_t
save_data(const struct grib2secs *gsp)
{
  //--- IDS
  if (ofile.ids == NULL) {
    if (1 != fwrite(gsp->ids, gsp->idslen, 1, ofile.ofp)) {
      return ERR_IO;
    }
    ofile.ids = mydup(gsp->ids, gsp->idslen);
    ofile.pos += gsp->idslen;
  } else {
    if (memcmp(gsp->ids, ofile.ids, gsp->idslen)) {
      fprintf(stderr, "inconsistent ids\n");
      fprintf(stderr, "\t%p\t%p\n", gsp->ids, ofile.ids);
      for (int i = 0; i < 21; i++) {
        fprintf(stderr, "\t%02x\t%02x\n", gsp->ids[i], ofile.ids[i]);
      }
      return ERR_BADGRIB;
    }
  }
  //--- GDS
  if (ofile.gds == NULL) {
    if (1 != fwrite(gsp->gds, gsp->gdslen, 1, ofile.ofp)) {
      return ERR_IO;
    }
    ofile.gds = mydup(gsp->gds, gsp->gdslen);
    ofile.pos += gsp->gdslen;
  } else {
    if (memcmp(gsp->gds, ofile.gds, gsp->gdslen)) {
      fprintf(stderr, "inconsistent gds\n");
      return ERR_BADGRIB;
    }
  }
  //--- PDS
  if (1 != fwrite(gsp->pds, gsp->pdslen, 1, ofile.ofp)) {
    return ERR_IO;
  }
  ofile.pos += gsp->pdslen;
  //--- DRS
  if (1 != fwrite(gsp->drs, gsp->drslen, 1, ofile.ofp)) {
    return ERR_IO;
  }
  ofile.pos += gsp->drslen;
  //--- BMS
  if (1 != fwrite(gsp->bms, gsp->bmslen, 1, ofile.ofp)) {
    return ERR_IO;
  }
  ofile.pos += gsp->bmslen;
  //--- DS
  if (1 != fwrite(gsp->ds, gsp->dslen, 1, ofile.ofp)) {
    return ERR_IO;
  }
  ofile.pos += gsp->dslen;
  return GSE_OKAY;
}

  gribscan_err_t
save_close(void)
{
  unsigned char sz[8];
  fwrite("7777", 4, 1, ofile.ofp);
  size_t msgsize = ftell(ofile.ofp);
  sz[0] = (msgsize >> 56) & 0xFF;
  sz[1] = (msgsize >> 48) & 0xFF;
  sz[2] = (msgsize >> 40) & 0xFF;
  sz[3] = (msgsize >> 32) & 0xFF;
  sz[4] = (msgsize >> 24) & 0xFF;
  sz[5] = (msgsize >> 16) & 0xFF;
  sz[6] = (msgsize >>  8) & 0xFF;
  sz[7] =  msgsize        & 0xFF;
  fseek(ofile.ofp, 8, SEEK_SET);
  fwrite(sz, 8, 1, ofile.ofp);
  fclose(ofile.ofp);
  return 0;
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
  double vlev;
  long ftime, dura, imemb;
  get_reftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
  iparm = get_parameter(gsp);
  ftime = get_ftime(gsp);
  vlev = get_vlevel(gsp);
  dura = get_duration(gsp);
  imemb = get_perturb(gsp);
  // 要素と面の複合フィルタ
  switch (gribscan_filter(sfilter, iparm, ftime, dura, vlev, imemb)) {
    case ERR_FSTACK: 
    case GSE_SKIP: goto END_SKIP; break;
    default:
    case GSE_OKAY: goto SAVE; break;
  }

SAVE:
  printf("b%s %6s f%-+5ld d%-+5ld v%-8.1lf m%-+3ld\n",
    sreftime, param_name(iparm), ftime, dura, vlev, imemb);
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
      if (argv[i][1] == 'o') {
        // explicit open
        r = save_open(argv[i] + 2);
        if (r != GSE_OKAY) goto BARF;
      } else if (argv[i][1] == 'f') {
        sfilter = argv[i] + 2;
      } else if (argv[i][1] == 'a') {
        sfilter = "";
      } else {
        fprintf(stderr, "%s: unknown option\n", argv[i]);
        r = GSE_JUSTWARN;
        goto BARF;
      }
    } else {
      // implicit open
      r = save_open(NULL);
      if (r != GSE_OKAY) goto BARF;
      r = grib2scan_by_filename(argv[i]);
      if (r != GSE_OKAY) goto BARF;
    }
  }
BARF:
  if (r != GSE_OKAY) return r;
  r = save_close();
  return r;
}

  int
main(int argc, const char **argv)
{
  gribscan_err_t r;
  r = argscan(argc, argv);
  if (r == ERR_NOINPUT) {
    fprintf(stderr, "usage: %s [-a][-fFILTER][-oFILENAME] input ...\n", argv[0]);
  } else if (r != GSE_OKAY) {
    fprintf(stderr, "%s: exit(%u)\n", argv[0], r);
  }
  mymemstat();
  return r;
}
