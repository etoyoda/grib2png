#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include "gribscan.h"
#include "mymalloc.h"

// output file, name can be changed by -o option
static const char *oFnam = "merged.grib";
static FILE *oFp = NULL;

typedef enum bitmapid_t {
  BMI_FULL = 'F',
  BMI_NORTH = 'N',
  BMI_SOUTH = 'S',
  BMI_Error = '#'
} bitmapid_t;

#define UK_NI 171u
#define UK_NJ 192u
#define FULL_NI (UK_NI*2u-1u)
#define FULL_NJ ((UK_NJ-1u)*3u)
#define FULL_NIJ (FULL_NI*FULL_NJ)

  bitmapid_t
ukwmc_bitmap_id(const grib2secs_t *gsp, const bounding_t *bp)
{
  size_t ni = bp->ni;
  size_t nj = bp->nj;
  bitmapid_t r = BMI_FULL;
  if (gsp->bms==NULL) {
    return r;
  }
  for (size_t j=0u; j<nj; j++) {
    size_t rowmiss = 0u;
    for (size_t i=0u; i<ni; i++) {
      if (unpackbits(gsp->bms+6, 1, i+j*ni) == 0) {
        rowmiss++;
      }
    }
    if (j==0u) {
      if (rowmiss==0u) {
        r = BMI_FULL;
      } else if (rowmiss==ni) {
        r = BMI_NORTH;
      } else {
        r = BMI_Error;
      }
    } else if (j < nj-1) {
      if (rowmiss != 0) { r = BMI_Error; }
    } else if (j == nj-1) {
      if (rowmiss==0u) {
        if (!((r==BMI_FULL)||(r==BMI_NORTH))) {
          r = BMI_Error;
        }
      } else if (rowmiss==ni) {
        if (r == BMI_FULL) {
          r = BMI_SOUTH;
        } else {
          r = BMI_Error;
        }
      } else {
        r = BMI_Error;
      }
    }
  }
  return r;
}

typedef enum tileid_t {
  TI_NW = 0,
  TI_NC,
  TI_NE,
  TI_SW,
  TI_SC,
  TI_SE,
  TI_Error
} tileid_t;

#define ALL_TILES ((1u << (int)TI_Error)-1u)

typedef struct sorter_t {
  iparm_t iparm;
  long ftime;
  long dura;
  double vlev;
  double memb;
  // fields to be added to retain data contents
  unsigned have_tiles;
  float *fbuf[TI_Error];
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
  sp->have_tiles = 0u;
  for (int i=0; i<TI_Error; i++) { sp->fbuf[i] = NULL; }
  sp->next = NULL;
  return sp;
}

  enum tileid_t
gds_tileid(const bounding_t *bp)
{
  if (bp->ni != 171) return TI_Error;
  if (bp->nj != 192) return TI_Error;
  if (fabs(bp->di - 705882.e-6) > 1.0e-9) return TI_Error;
  if (fabs(bp->dj - 471204.e-6) > 1.0e-9) return TI_Error;
  if ((bp->n==90.0)&&(bp->s==0.0)&&(bp->w==70.0)&&(bp->e==190.0)) { return TI_NE; }
  if ((bp->n==0.0)&&(bp->s==-90.0)&&(bp->w==70.0)&&(bp->e==190.0)) { return TI_SE; }
  if ((bp->n==90.0)&&(bp->s==0.0)&&(bp->w==190.0)&&(bp->e==310.0)) { return TI_NW; }
  if ((bp->n==0.0)&&(bp->s==-90.0)&&(bp->w==190.0)&&(bp->e==310.0)) { return TI_SW; }
  if ((bp->n==90.0)&&(bp->s==0.0)&&(bp->w==310.0)&&(bp->e==430.0)) { return TI_NC; }
  if ((bp->n==0.0)&&(bp->s==-90.0)&&(bp->w==310.0)&&(bp->e==430.0)) { return TI_SC; }
  fprintf(stderr, "Unknown tile n%g s%g w%g e%g\n", bp->n, bp->s, bp->w, bp->e);
  return TI_Error;
}

  enum gribscan_err_t
store_plane(sorter_t *sp, const grib2secs_t *gsp)
{
  if (oFp==NULL) {
    oFp=fopen(oFnam, "wb");
    if (oFp==NULL) {
      perror(oFnam);
      return ERR_IO;
    }
    // write IS/IDS/GDS at the beginning of file
    unsigned char is[16] = {'G','R','I','B', 0,0,0,2, 0,0,0,0, 0,0,0,0};
    if (1u!=fwrite(is,16u,1u,oFp)) { return ERR_IO; }
    if (1u!=fwrite(gsp->ids,gsp->idslen,1u,oFp)) { return ERR_IO; }
    unsigned char *mygds=mymalloc(gsp->gdslen);
    if (mygds==NULL) { return ERR_NOMEM; }
    memcpy(mygds,gsp->gds,gsp->gdslen);
    // currently decode_gds() aborts w/GDS other than GDT3.0
    // following offsets are for GDT3.0
    set_si4(mygds+6, FULL_NIJ); //Ni*Nj
    set_si4(mygds+30, FULL_NI); //Ni
    set_si4(mygds+34, FULL_NJ); //Nj
    set_si4(mygds+46, 90000000L); //La1
    set_si4(mygds+50, -170000000L); //Lo1
    set_si4(mygds+55, -90000000L); //La2
    set_si4(mygds+59, 190000000L); //Lo2
    if (1u!=fwrite(mygds,gsp->gdslen,1u,oFp)) { return ERR_IO; }
    myfree(mygds);
  }
  if (1u!=fwrite(gsp->pds,gsp->pdslen,1u,oFp)) { return ERR_IO; }
  //=== DRS - DRT5.4
  unsigned char drs[12];
  set_ui4(drs+0,12uL);
  drs[4]=5u;
  set_ui4(drs+5,FULL_NIJ);
  set_ui2(drs+9,4u); // DRT5.4
  drs[11]=1u; // IEEE float32
  if (1u!=fwrite(drs,12u,1u,oFp)) { return ERR_IO; }
  //=== BMS - not using bitmap
  unsigned char bms[6];
  set_ui4(bms+0,6uL);
  bms[4]=6u;
  bms[5]=255u; // bitmap does not apply
  if (1u!=fwrite(bms,6u,1u,oFp)) { return ERR_IO; }
  //=== DS - DRT5.4
  size_t dslen=5u+sizeof(float)*FULL_NIJ;
  unsigned char *ds=mymalloc(dslen);
  if (ds==NULL) { return ERR_NOMEM; }
  set_ui4(ds+0,dslen);
  ds[4]=7u;
  unsigned char *bbuf=ds+5;
  //j=0
  memset(bbuf,4*FULL_NI,0xFF);
  //Northern hemisphere
  for(int ti=TI_NW; ti<=TI_NE; ti++){
    for(int j=0; j<(UK_NJ-1); j++){
      for(int i=0; i<(UK_NI-1); i++) {
        int bofs=(j+1)*FULL_NI+i;
        if (ti==TI_NC) bofs+=(UK_NI-1);
        if (ti==TI_NE) bofs+=(UK_NI-1)*2;
        bofs *= 4;
        int tofs=j*UK_NI+i;
        set_float32(bbuf+bofs,sp->fbuf[ti][tofs]);
      }
    }
  }
  //Southern hemisphere
  for(int ti=TI_SW; ti<=TI_SE; ti++){
    for(int j=0; j<(UK_NJ-1); j++){
      for(int i=0; i<(UK_NI-1); i++) {
        int bofs=(j+UK_NJ-1)*FULL_NI+i;
        if (ti==TI_SC) bofs+=(UK_NI-1);
        if (ti==TI_SE) bofs+=(UK_NI-1)*2;
        bofs *= 4;
        int tofs=j*UK_NI+i;
        set_float32(bbuf+bofs,sp->fbuf[ti][tofs]);
      }
    }
  }
  //j=last
  memset(bbuf+4*FULL_NI*(FULL_NJ-1),4*FULL_NI,0xFF);
  if (1u!=fwrite(ds,dslen,1u,oFp)) { return ERR_IO; }
  return GSE_OKAY;
}

  void
close_outgrib(void)
{
  if (oFp==NULL) return;
  fwrite("7777", 4u, 1u, oFp);
  long pos=ftell(oFp);
  unsigned char is8[8];
  set_ui4(is8, (unsigned long)pos >> 32);
  set_ui4(is8+4, 0xFFFFFFFFuL & (unsigned long)pos);
  fseek(oFp, 8u, SEEK_SET);
  fwrite(is8, 8u, 1u, oFp);
  fclose(oFp);
}

  enum gribscan_err_t
sorter_add(sorter_t *sp, const grib2secs_t *gsp,
  const bounding_t *bp)
{
  // タイル位置を判定。既に取得されていれば @dup と表示して処理中止
  enum tileid_t ti;
  ti = gds_tileid(bp);
  if (ti == TI_Error) return ERR_UNSUPPORTED;
  if (sp->have_tiles & (1u << (int)ti)) {
    puts("@dup");
    return GSE_OKAY;
  }
  // ビットマップの分類
  enum bitmapid_t bi;
  bi = ukwmc_bitmap_id(gsp, bp);
  printf("@bm %c %d %s\n", (int)bi, (int)ti, param_name(sp->iparm));
  if ((ti==TI_NW)&&(bi==BMI_NORTH)) goto tibi_okay;
  if ((ti==TI_NC)&&(bi==BMI_NORTH)) goto tibi_okay;
  if ((ti==TI_NE)&&(bi==BMI_NORTH)) goto tibi_okay;
  if ((ti==TI_SW)&&(bi==BMI_SOUTH)) goto tibi_okay;
  if ((ti==TI_SC)&&(bi==BMI_SOUTH)) goto tibi_okay;
  if ((ti==TI_SE)&&(bi==BMI_SOUTH)) goto tibi_okay;
  puts("unexpected bitmap and tile combination");
  return ERR_BADGRIB;
tibi_okay:
  ;
  size_t npixels=get_npixels(gsp);
  float *fbuf=mymalloc(npixels*sizeof(float));
  gribscan_err_t r;
  r=decode_ds_float(gsp, fbuf, NULL);
  if (r!=GSE_OKAY) {
    myfree(fbuf);
    return r;
  }
  float fmin=FLT_MAX, fmax=-FLT_MAX;
  double fsum=0.0;
  for (size_t i=0u; i<npixels; i++) {
    if (fbuf[i]>fmax) fmax=fbuf[i];
    if (fbuf[i]<fmin) fmin=fbuf[i];
    fsum+=fbuf[i];
  }
  sp->have_tiles |= (1u << (int)ti);
  sp->fbuf[(int)ti]=fbuf;
  printf("max %10.3f min %10.3f avg %10.3f\n",
    fmax, fmin, fsum/npixels);
  // 全タイルの取得ができていれば @full といって報告
  r=GSE_OKAY;
  if (sp->have_tiles == ALL_TILES) {
    printf("@full %7s f=%ld d=%ld v=%s m=%g\n",
      param_name(sp->iparm), sp->ftime, sp->dura, level_name(sp->vlev),
      sp->memb);
    r=store_plane(sp, gsp);
    // dispose data values; the sorter element is kept so that 
    // duplicated tiles arriving later can be recognised by have_tiles;
    for (int i=0; i<TI_Error; i++) {
      myfree(sp->fbuf[i]);
      sp->fbuf[i]=NULL;
    }
  }
  return r;
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
sort_data(const struct grib2secs *gsp, const bounding_t *bp)
{
  gribscan_err_t r;
  if (Sorter == NULL) {
    puts("@new");
    Sorter = new_sorter(gsp); 
    r = sorter_add(Sorter, gsp, bp);
    return r;
  }
  sorter_t *cur = Sorter;
  while (1) {
    if (sorter_gsp_compat(cur, gsp)) {
      r = sorter_add(cur, gsp, bp);
      break;
    }
    if (cur->next == NULL) {
      puts("@new");
      cur->next = new_sorter(gsp);
      r = sorter_add(cur->next, gsp, bp);
      break;
    }
    cur = cur->next;
  }
  return r;
}

  void
sorter_remains_check()
{
  sorter_t *cur = Sorter;
  if (!cur) { return; }
  while (1) {
    if (cur->have_tiles != ALL_TILES) {
      printf("@partial %04x %7s f=%ld d=%ld v=%s m=%g\n",
        cur->have_tiles,
        param_name(cur->iparm), cur->ftime, cur->dura, level_name(cur->vlev),
        cur->memb);
    }
    if (!cur->next) { break; }
    cur = cur->next;
  }
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
  }

  printf("b%s %6s f%-+5ld d%-+5ld v%-8s m%-+4.3g\n",
    sreftime, param_name(iparm), ftime, dura, level_name(vlev), memb);
  sort_data(gsp, &bnd);
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
      case 'o':
        oFnam = argv[i]+2;
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
  sorter_remains_check();
  return r; 
}

  int
main(int argc, const char **argv)
{
  gribscan_err_t r;
  // mymalloc_verbose = 1;
  gribscan_quiet_on_bitmaps = 1;
  r = argscan(argc, argv);
  if (r == ERR_NOINPUT) {
    fprintf(stderr, "usage: %s input ...\n", argv[0]);
  } else if (r != GSE_OKAY) {
    fprintf(stderr, "%s: exit(%u)\n", argv[0], r);
  }
  close_outgrib();
  mymemstat();
  return r;
}
