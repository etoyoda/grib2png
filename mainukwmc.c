#include <stdio.h>
#include <time.h>
#include "gribscan.h"
#include "mymalloc.h"

typedef struct sorter_t {
  iparm_t iparm;
  long ftime;
  long dura;
  double vlev;
  double memb;
  // fields to be added to retain data contents
  struct sorter_t *next;
} sorter_t;

  struct sorter_t *
new_sorter(const grib2secs_t *gsp)
{
  struct sorter_t *sp;
  sp = mymalloc(sizeof(sorter_t));
  if (sp == NULL) return NULL;
  sp->iparm = get_parameter(gsp);
  sp->ftime = get_ftime(gsp);
  sp->vlev = get_vlevel(gsp);
  sp->dura = get_duration(gsp);
  sp->memb = get_perturb(gsp);
  sp->next = NULL;
  return sp;
}

  int
sorter_gsp_compat(const sorter_t *sp, const grib2secs_t *gsp)
{
  if (sp->iparm != get_parameter(gsp)) return 0;
  if (sp->ftime != get_ftime(gsp)) return 0;
  if (sp->vlev != get_vlevel(gsp)) return 0;
  if (sp->dura != get_duration(gsp)) return 0;
  if (sp->memb != get_perturb(gsp)) return 0;
  return 1;
}

static sorter_t *Sorter = NULL;

  gribscan_err_t
sort_data(const struct grib2secs *gsp)
{
  if (Sorter == NULL) {
    puts("new");
    Sorter = new_sorter(gsp); 
    return GSE_OKAY;
  }
  sorter_t *cur = Sorter;
  while (1) {
    if (sorter_gsp_compat(cur, gsp)) {
      // register
      puts("hit");
      break;
    }
    if (cur->next == NULL) {
      puts("new");
      cur->next = new_sorter(gsp);
      break;
    }
    cur = cur->next;
  }
  return GSE_OKAY;
}

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

// empty filter string means "accept all"
static const char *sfilter = "";

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
      goto END_SKIP;
      break;
    case GSE_OKAY:
    default:
      /* do nothing */;
  }
  // 位置情報の印字。
  // 同一GRIB報内で複数GDSが存在する場合ポインタ値が異なることを利用。
  // gribscan.c grib2loopsecs() のコメント参照。
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

  printf("b%s %6s f%-+5ld d%-+5ld v%-8s m%-+4.3g\n",
    sreftime, param_name(iparm), ftime, dura, level_name(vlev), memb);
  sort_data(gsp);
  goto END_NORMAL;

END_SKIP:
  r = GSE_SKIP;
END_NORMAL:
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
