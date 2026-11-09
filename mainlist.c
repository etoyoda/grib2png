#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include "gribscan.h"
#include "mymalloc.h"

  gribscan_err_t
check_bms(const struct grib2secs *gsp, bounding_t *bnd)
{
  size_t ni = bnd->ni;
  size_t nj = bnd->nj;
  for (size_t j=0; j<nj; j++) {
    size_t rowmiss = 0;
    for (size_t i=0; i<ni; i++) {
      if (unpackbits(gsp->bms+6, 1, i+j*ni) == 0) {
        rowmiss++;
      }
    }
    if (rowmiss) {
      printf("bitmap row=%zu miss=%zu\n", j, rowmiss);
    }
  }
  return GSE_OKAY;
}

  gribscan_err_t
check_datacontent(const struct grib2secs *gsp)
{
  size_t npixels = get_npixels(gsp);
  float *fbuf = mymalloc(npixels*sizeof(float));
  size_t cnan=0, cbig=0;
  decode_ds_float(gsp, fbuf, NULL);
  for (size_t i=0; i<npixels; i++) {
    if (isnanf(fbuf[i])) cnan++;
    if (fabsf(fbuf[i])>1.0e9f) cbig++;
  }
  if (cnan||cbig) {
    printf("cnan=%zu cbig=%zu fbuf[0]=%08X\n", cnan, cbig,
    *(uint32_t*)fbuf);
    bounding_t bnd;
    if (GSE_OKAY == decode_gds(gsp, &bnd)) {
      for (size_t j=0; j<bnd.nj; j++) {
        size_t cjbig=0;
        size_t cjnan=0;
        for (size_t i=0; i<bnd.ni; i++) {
          if (fabsf(fbuf[i+j*bnd.ni])>1.0e9f) cjbig++;
          if (isnanf(fbuf[i+j*bnd.ni])) cjnan++;
        }
        if (cjbig || cjnan) {
          printf(" row %zu: nan %zu big %zu\n", j, cjnan, cjbig);
        }
      }
    }
  }
  myfree(fbuf);
  return GSE_OKAY;
}

// empty filter string means "accept all"
static const char *sfilter = "";
static const void *prev_gds = NULL;

// gribscan ライブラリから呼び返される関数。
  gribscan_err_t
checksec7(const struct grib2secs *gsp)
{
  gribscan_err_t r;
  // dimensions
  struct tm t;
  char sreftime[24];
  unsigned long iparm;
  double vlev, memb;
  long ftime, dura;
  r = GSE_OKAY;
  // retrieve PDT metadata
  get_reftime(&t, gsp);
  showtime(sreftime, sizeof sreftime, &t);
  iparm = get_parameter(gsp);
  ftime = get_ftime(gsp);
  vlev = get_vlevel(gsp);
  dura = get_duration(gsp);
  memb = get_perturb(gsp);
  // filter
  switch (gribscan_filter(sfilter, iparm, ftime, dura, vlev, memb)) {
    case ERR_FSTACK:
    case GSE_SKIP:
      r = GSE_SKIP;
      goto END_SKIP;
      break;
    case GSE_OKAY:
    default:
      /* do nothing */;
  }
  // 位置情報の印字。
  // 同一GRIB報内で複数GDSが存在する場合ポインタ値が異なることを利用。
  // gribscan.c grib2loopsecs() のコメント参照。
  if (prev_gds != gsp->gds) {
    bounding_t bnd;
    gribscan_err_t r2 = decode_gds(gsp, &bnd);
    if (r2 != GSE_OKAY) {
      fprintf(stderr, "GDS decoding error %u\n", r2);
    } else {
      printf("lat%+07.3f:%+07.3f lon%+08.3f:%+08.3f %c %8.6gx%-8.6g %4zux%-4zu\n",
        bnd.s, bnd.n, bnd.w, bnd.e, (bnd.wraplon ? 'C' : 'R'),
        bnd.di, bnd.dj, bnd.ni, bnd.nj);
      if (bnd.has_bitmap) {
        check_bms(gsp, &bnd);
      }
    }
    prev_gds = gsp->gds;
  }
  printf("b%s %6s f%-+5ld d%-+5ld v%-8s m%-+4.3g\n",
    sreftime, param_name(iparm), ftime, dura, level_name(vlev), memb);
  check_datacontent(gsp);
END_SKIP:
  myfree(gsp->ds);
  return r;
}

  gribscan_err_t
argscan(int argc, const char **argv)
{
  gribscan_err_t r = ERR_NOINPUT;
  for (int i=1; i<argc; i++) {
    if (argv[i][0]=='-') {
      switch (argv[i][1]) {
      case 'f':
        sfilter = argv[i]+2;
	break;
      default:
        fprintf(stderr, "%s: unknown option\n", argv[i]);
	r = GSE_JUSTWARN;
	goto BARF;
      }
    } else {
      // 複数ファイルを処理する場合に、前のファイルを忘れる
      prev_gds = NULL;
      r = grib2scan_by_filename(argv[i]);
      if (r != GSE_OKAY) goto BARF;
    }
  }
BARF:
  return r; 
}

  int
main(int argc, const char **argv)
{
  gribscan_err_t r;
  r = argscan(argc, argv);
  if (r == ERR_NOINPUT) {
    fprintf(stderr, "usage: %s input ...\n", argv[0]);
  } else if (r != GSE_OKAY) {
    fprintf(stderr, "%s: exit(%u)\n", argv[0], r);
  }
  mymemstat();
  return r;
}
