#include <stdio.h>
#include <time.h>
#include <math.h>

#include "mymalloc.h"
#include "gribscan.h"

#ifndef DBG53
# define DBG53 0
#endif

  gribscan_err_t
decode_ds(const grib2secs_t *gsp, double *dbuf,
  void (*adjust_scales)(iparm_t param, int *scale_e, int *scale_d)
  )
{
  size_t npixels;
  unsigned drstempl;
  int scale_e;
  int scale_d;
  unsigned depth;
  float refv;
  if ((gsp->drslen == 0) || (gsp->dslen == 0)) {
    fprintf(stderr, "missing DRS %zu DS %zu\n", gsp->drslen, gsp->dslen);
    return ERR_BADGRIB;
  }
  // DRS#6 - データ点数（ビットマップがある場合は第7節に残る点数）
  npixels = ui4(gsp->drs + 5);
  // DRS#10 - データ表現テンプレート番号
  drstempl = ui2(gsp->drs + 9);

  // DRT5.0 と DRT5.3 の共通部
  refv = float32(gsp->drs + 11);
  scale_e = si2(gsp->drs + 15);
  scale_d = si2(gsp->drs + 17);
  // DRS#20 - ビットパックのビット数
  depth = gsp->drs[19];

  // テンプレートの分岐
  if (drstempl == 0) goto DRT5_0;
  if (drstempl == 3) goto DRT5_3;
  fprintf(stderr, "unsupported DRS template 5.%u\n", drstempl);
  return ERR_BADGRIB;

  /* DRT5.0 - 単純圧縮 */
DRT5_0: ;
  if (gsp->drslen < 21) {
    fprintf(stderr, "DRS size %zu < 21 for DRT5.0\n", gsp->drslen);
    return ERR_BADGRIB;
  }
  adjust_scales(get_parameter(gsp), &scale_e, &scale_d);
  for (unsigned i = 0; i < npixels; i++) {
    dbuf[i] = (refv + ldexp(unpackbits(gsp->ds + 5, depth, i), scale_e))
      * pow(10.0, -scale_d);
  }
  return GSE_OKAY;

  /* DRT5.0 - 複合差分圧縮 */
DRT5_3:
  if (gsp->drslen < 49) {
    fprintf(stderr, "DRS size %zu < 49 for DRT5.3\n", gsp->drslen);
    return ERR_BADGRIB;
  }
  // DRS#22 - 資料群分割法: 1は一般的な分割
  if (gsp->drs[21] != 1) { fprintf(stderr, "DRS#22 = %u (1 only)\n", gsp->drs[21]);
    return ERR_BADGRIB;
  }
  // DRS#23 - 欠損値: 0は欠損値なし (すると DRS#24,28 は無視できる
  if (gsp->drs[22] != 0) { fprintf(stderr, "DRS#23 = %u (0 only)\n", gsp->drs[21]);
    return ERR_BADGRIB;
  }
  // DRS#32 - 資料群の数
  unsigned ng = ui4(gsp->drs + 31);
  // DRS#36 - 資料群幅の参照値
  unsigned g_width_ref = gsp->drs[35];
  // DRS#37 - 資料群幅を表わすためのビット数
  unsigned g_width_nbits = gsp->drs[36];
  // DRS#38 - 資料群長の参照値
  unsigned g_len_ref = ui4(gsp->drs + 37);
  // DRS#42 - 資料群長に対する長さ増分
  unsigned g_len_inc = gsp->drs[41];
  // DRS#43 - 最後の資料群の真の資料群長
  unsigned last_g_len = ui4(gsp->drs + 42);
  // DRS#47 - 尺度付き資料群長を表わすためのビット数
  unsigned g_len_nbits = gsp->drs[46];
  // DRS#48 - 空間差分の次数: 現状では2だけに対応
  if (gsp->drs[47] != 2) { fprintf(stderr, "DRS#22 = %u (2 only)\n", gsp->drs[47]);
    return ERR_BADGRIB;
  }
  // DRS#49 - DRT7.3#6以後の数値のオクテット数; 気象庁では2に固定.
  if (gsp->drs[48] != 2) { fprintf(stderr, "DRS#22 = %u (2 only)\n", gsp->drs[48]);
    return ERR_BADGRIB;
  }
  // DS#6-11 - Z(1), Z(2), Zmin (各2オクテット)
  unsigned z1 = ui2(gsp->ds + 5);
  unsigned z2 = ui2(gsp->ds + 7);
  signed zmin = si2(gsp->ds + 9);
#if DBG53
printf("ng=%u z1=%u z2=%u zmin=%d\n", ng, z1, z2, zmin);
#endif
  // 三個配列の確保
  unsigned *group_ref = mymalloc(sizeof(unsigned) * ng);
  if (group_ref == NULL) { return ERR_NOMEM; }
  unsigned *g_width = mymalloc(sizeof(unsigned) * ng);
  if (g_width == NULL) { return ERR_NOMEM; }
  unsigned *group_length = mymalloc(sizeof(unsigned) * ng);
  if (group_length == NULL) { return ERR_NOMEM; }

  // 第1配列 group_ref の読み取り
  unsigned char *ptr = gsp->ds + 11u;
#if DBG53
printf("ds#%04lu %u group_ref\n", ptr - gsp->ds + 1, depth);
printx(gsp->ds, (ptr-gsp->ds)/16*16);
#endif
  for (size_t j = 0; j < ng; j++) {
    group_ref[j] = unpackbits(ptr, depth, j);
  }
  unsigned blocksize = (ng * depth + 7u) / 8u;
  ptr += blocksize;

  // 第2配列 g_width の読み取り
#if DBG53
printf("ds#%04lu %u g_width\n", ptr - gsp->ds + 1, g_width_nbits);
printx(gsp->ds, (ptr-gsp->ds)/16*16);
#endif
  for (size_t j = 0; j < ng; j++) {
    g_width[j] = unpackbits(ptr, g_width_nbits, j) + g_width_ref;
  }
  blocksize = (ng * g_width_nbits + 7u) / 8u;
  ptr += blocksize;

  // 第3配列 g_len の読み取り (その場で group_length に換算)
#if DBG53
printf("ds#%04lu %u g_len\n", ptr - gsp->ds + 1, g_len_nbits);
printx(gsp->ds, (ptr-gsp->ds)/16*16);
#endif
  for (size_t j = 0; j < ng; j++) {
    // g_len[j] とすべき値
    unsigned g_len_j = unpackbits(ptr, g_len_nbits, j);
    group_length[j] = g_len_ref + g_len_inc * g_len_j;
  }
  // group_length の最終要素は別途指定される
  group_length[ng-1] = last_g_len;
  blocksize = (ng * g_len_nbits + 7u) / 8u;
  ptr += blocksize;

#if DBG53
printf("ds#%04lu z\n", ptr - gsp->ds + 1);
printx(gsp->ds, (ptr-gsp->ds)/16*16);
#endif
  // 保安確認
  size_t npx = 0;
  size_t nbits = (ptr - gsp->ds) * 8u;
  for (size_t j = 0; j < ng; j++) {
    npx += group_length[j];
    nbits += group_length[j] * g_width[j];
  }
  if (npx != npixels) {
    fprintf(stderr, "npixels %zu != %zu\n", npx, npixels);
    return ERR_BADGRIB;
  }
#if DBG53
printf("nbytes %zu %zu\n", (nbits + 7u) / 8u, gsp->dslen);
#endif
  if (nbits > gsp->dslen * 8u) {
    fprintf(stderr, "nbits %zu overrun %zu\n", nbits, gsp->dslen * 8u);
    return ERR_BADGRIB;
  } else if ((nbits+7u)/8u < gsp->dslen) {
    fprintf(stderr, "padding %zu octets\n", gsp->dslen-(((size_t)nbits+7u)/8u));
  }

  dbuf[0] = (refv + ldexp(z1, scale_e)) * pow(10.0, -scale_d);
  dbuf[1] = (refv + ldexp(z2, scale_e)) * pow(10.0, -scale_d);

  signed x_prev, x_prev2;
  npx = 2;
  nbits = g_width[0] * 2;
  x_prev = (int)z2;
  x_prev2 = (int)z1;
  for (size_t j = 0; j < ng; j++) {
    size_t grplen = (j == 0) ? (group_length[j] - 2) : group_length[j];
#if DBG53
printf("ds#%04lu grp %5zu ref %5u w %5u len %5u\n", ptr-gsp->ds+1+(nbits/8u),
j, group_ref[j], g_width[j], group_length[j]);
#endif
    for (size_t k = 0; k < grplen; k++) {
      int x, y;
      unsigned z;
      size_t byteofs = nbits / 8u;
      size_t bitofs = nbits % 8u;
      z = getbits(ptr + byteofs, bitofs, g_width[j]);
      y = z + (int)group_ref[j] + zmin;
      x = y + 2u * x_prev - x_prev2;
      dbuf[npx] = (refv + ldexp(x, scale_e)) * pow(10.0, -scale_d);
#if DBG53
printf("%5zu z=%5d gref=%5d y=%5d x=%5d %7.2f\n", npx, z, group_ref[j], y, x,
dbuf[npx]);
#endif
      // shift to next pixel
      nbits += g_width[j];
      npx++;
      x_prev2 = x_prev;
      x_prev = x;
    }
  }

  myfree(group_ref);
  myfree(g_width);
  myfree(group_length);
  return GSE_OKAY;
}

