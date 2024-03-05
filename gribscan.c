#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#include "mymalloc.h"
#include "gribscan.h"

#ifndef DBG53
# define DBG53 0
#endif

//=== レプレゼンテーション層 ===

// 4オクテット符号付き整数の解読
  long
si4(const unsigned char *buf)
{
  unsigned long r;
  r = ((buf[0] & 0x7Fu) << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
  return (buf[0] & 0x80u) ? -r : r;
}

// 4オクテット符号なし整数の解読
  unsigned long
ui4(const unsigned char *buf)
{
  return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

// 2オクテット符号付き整数の解読
  int
si2(const unsigned char *buf)
{
  long r;
  r = ((buf[0] & 0x7Fu) << 8) | buf[1];
  return (buf[0] & 0x80u) ? -r : r;
}

// 2オクテット符号なし整数の解読
  unsigned
ui2(const unsigned char *buf)
{
  return (buf[0] << 8) | buf[1];
}

// 任意ビット数整数の抽出
// 先頭オクテット buf の bitofs ビット飛ばした場所から nbits 幅
// 制約: bitofs は 0..7
// 長大ビット配列からの抽出は unpackbits() を使う
  unsigned
getbits(const unsigned char *buf, size_t bitofs, size_t nbits)
{
  unsigned c0, c1, c2, c3;
  if (nbits == 0u) {
    return 0u;
  } else if (nbits == 7u) {
    switch (bitofs) {
    case 0: return buf[0] >> 1;
    case 1: return buf[0] & 0x7Fu;
    case 2: return ((buf[0] << 1) & 0x7Fu) | buf[1] >> 7;
    case 3: return ((buf[0] << 2) & 0x7Fu) | buf[1] >> 6;
    case 4: return ((buf[0] << 3) & 0x7Fu) | buf[1] >> 5;
    case 5: return ((buf[0] << 4) & 0x7Fu) | buf[1] >> 4;
    case 6: return ((buf[0] << 5) & 0x7Fu) | buf[1] >> 3;
    case 7: return ((buf[0] << 6) & 0x7Fu) | buf[1] >> 2;
    }
  } else if (nbits == 8u) {
    switch (bitofs) {
    case 0: return buf[0];
    case 1: return ((buf[0] << 1) & 0xFFu) | buf[1] >> 7;
    case 2: return ((buf[0] << 2) & 0xFFu) | buf[1] >> 6;
    case 3: return ((buf[0] << 3) & 0xFFu) | buf[1] >> 5;
    case 4: return ((buf[0] << 4) & 0xFFu) | buf[1] >> 4;
    case 5: return ((buf[0] << 5) & 0xFFu) | buf[1] >> 3;
    case 6: return ((buf[0] << 6) & 0xFFu) | buf[1] >> 2;
    case 7: return ((buf[0] << 7) & 0xFFu) | buf[1] >> 1;
    }
  } else if (nbits == 10u) {
    switch (bitofs) {
    case 0: return ((buf[0] << 2) & 0x3FFu) | buf[1] >> 6;
    case 1: return ((buf[0] << 3) & 0x3FFu) | buf[1] >> 5;
    case 2: return ((buf[0] << 4) & 0x3FFu) | buf[1] >> 4;
    case 3: return ((buf[0] << 5) & 0x3FFu) | buf[1] >> 3;
    case 4: return ((buf[0] << 6) & 0x3FFu) | buf[1] >> 2;
    case 5: return ((buf[0] << 7) & 0x3FFu) | buf[1] >> 1;
    case 6: return ((buf[0] << 8) & 0x3FFu) | buf[1];
    case 7: return ((buf[0] << 9) & 0x3FFu) | buf[1] << 1 | buf[2] >> 7;
    }
  } else if (nbits == 12u) {
    switch (bitofs) {
    case 0: return ((buf[0] <<  4) & 0xFFFu) | buf[1] >> 4;
    case 1: return ((buf[0] <<  5) & 0xFFFu) | buf[1] >> 3;
    case 2: return ((buf[0] <<  6) & 0xFFFu) | buf[1] >> 2;
    case 3: return ((buf[0] <<  7) & 0xFFFu) | buf[1] >> 1;
    case 4: return ((buf[0] <<  8) & 0xFFFu) | buf[1];
    case 5: return ((buf[0] <<  9) & 0xFFFu) | buf[1] << 1 | buf[2] >> 7;
    case 6: return ((buf[0] << 10) & 0xFFFu) | buf[1] << 2 | buf[2] >> 6;
    case 7: return ((buf[0] << 11) & 0xFFFu) | buf[1] << 3 | buf[2] >> 5;
    }
  }
  c0 = (nbits + bitofs < 8u
    ? buf[0] >> (8u - nbits - bitofs)
    : buf[0] << (nbits + bitofs - 8u)
  ) & ((1u << nbits) - 1u);
  c1 = nbits + bitofs < 16u
    ? buf[1] >> (16u - nbits - bitofs)
    : buf[1] << (nbits + bitofs - 16u)
  ;
  c2 = nbits + bitofs < 24u
    ? buf[2] >> (24u - nbits - bitofs)
    : buf[2] << (nbits + bitofs - 24u)
  ;
  c3 = buf[3] >> (32u - nbits - bitofs);
  return c0 | c1 | c2 | c3;
}

// 任意ビット数整数の抽出
// 長大ビット配列 buf の bitofs ビット目から nbits 幅を抽出
  unsigned
unpackbits(const unsigned char *buf, size_t nbits, size_t pos)
{
  size_t byteofs = (nbits * pos) / 8u;
  size_t bitofs = (nbits * pos) % 8u;
  return getbits(buf + byteofs, bitofs, nbits);
}

// GRIB 固有の 40 ビット浮動小数点数の解読
  double
float40(const unsigned char *ptr)
{
  if (ptr[0] == 255) {
    return nan("");
  } else if ((0x80 & ptr[0]) != 0) {
    return pow(10.0, ptr[0] & 0x7F) * ui4(ptr + 1);
  } else {
    return pow(10.0, -ptr[0]) * ui4(ptr + 1);
  }
}

// ビッグエンディアン32ビット浮動小数点数の解読
  double
float32(const unsigned char *buf)
{
  int sign = (buf[0] & 0x80) ? -1 : 1;
  unsigned e = ((buf[0] & 0x7Fu) << 1) | ((buf[1] >> 7) & 0x1u);
  unsigned long mant = (buf[1] << 16) | (buf[2] << 8) | buf[3];
  if (e == 0 && mant == 0.0) {
    return sign * 0.0;
  } else if (e == 0) {
    return sign * ldexp((double)mant, e - 127 - 23);
  } else {
    return sign * ldexp((double)(mant | 0x800000uL), e - 127 - 23);
  }
}

//=== GRIB の構造解析 ===

// gsp の指示する PDS からパラメタを抽出
// 4 オクテットでパラメタを次の構造で表現する
// 構造:
// 0xFF000000: (暫定ゼロ、ローカル定義の処理のために予約)
// 0x00FF0000: discipline
// 0x0000FF00: category
// 0x000000FF: parameter
// ~0: エラー
  unsigned long
get_parameter(const grib2secs_t *gsp)
{
  unsigned pdst;
  unsigned long r;
  r = gsp->discipline << 16;
  if (gsp->pdslen == 0)
    return ~0ul;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 1:
  case 8:
  case 11:
    r |= ui2(gsp->pds + 9);
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return ~0ul;
  }
  return r;
}

  gribscan_err_t
set_parameter(grib2secs_t *gsp, iparm_t iparm)
{
  gsp->discipline = (iparm & 0xFF0000) >> 24;
  if (gsp->pdslen == 0) return ERR_BADGRIB;
  unsigned pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 1:
  case 8:
  case 11:
    gsp->pds[9] = (iparm & 0xFF00) >> 8;
    gsp->pds[10] = iparm & 0xFF;
    break;
  default:
    return ERR_BADGRIB;
  }
  return GSE_OKAY;
}

  long
get_ftime(const grib2secs_t *gsp)
{
  long r;
  unsigned tunits;
  unsigned pdst;
  if (gsp->pdslen == 0)
    return LONG_MAX;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 1:
  case 8:
  case 11:
    tunits = gsp->pds[17];
    r = ui4(gsp->pds + 18);
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return LONG_MAX;
  }
  switch (tunits) {
  case 0: break;
  case 1: r *= 60; break;
  case 2: r *= (60 * 24); break;
  case 10: r *= (3 * 60); break;
  case 11: r *= (6 * 60); break;
  case 12: r *= (12 * 60); break;
  case 13: r /= 60; break;
  default:
    return LONG_MAX;
  }
  return r;
}

  long
get_duration(const grib2secs_t *gsp)
{
  long r;
  unsigned tunits;
  unsigned pdst;
  unsigned nranges;
  if (gsp->pdslen == 0)
    return LONG_MAX;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 1:
    return 0;
  case 8:
    nranges = gsp->pds[41];
    tunits = gsp->pds[48];
    r = ui4(gsp->pds + 49);
    break;
  case 11:
    nranges = gsp->pds[44];
    tunits = gsp->pds[51];
    r = ui4(gsp->pds + 52);
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return LONG_MAX;
  }
  if (nranges != 1) {
    fprintf(stderr, "unsupported n time ranges %u > 1\n", nranges);
    return LONG_MAX;
  }
  switch (tunits) {
  case 0: break;
  case 1: r *= 60; break;
  case 2: r *= (60 * 24); break;
  case 10: r *= (3 * 60); break;
  case 11: r *= (6 * 60); break;
  case 12: r *= (12 * 60); break;
  case 13: r /= 60; break;
  default:
    return LONG_MAX;
  }
  return r;
}

  double
get_vlevel(const grib2secs_t *gsp)
{
  double r;
  unsigned pdst, vtype;
  const unsigned char *vlevptr;
  if (gsp->pdslen == 0)
    return LONG_MAX;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 1:
  case 8:
  case 11:
    vtype = gsp->pds[22];
    vlevptr = gsp->pds + 23;
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return nan("");
    break;
  }
  switch (vtype) {
  case 1:
    r = 101325.0;
    break;
  case 100:
    r = float40(vlevptr);
    break;
  case 101:
    r = 101324.0;
    break;
  case 103:
    r = 101324.5 - float40(vlevptr) * 11.0;
    break;
  default:
    fprintf(stderr, "unsupported vertical level type %u\n", vtype);
    return nan("");
    break;
  }
  return r;
}

  double
get_perturb(const grib2secs_t *gsp)
{
  unsigned pdst;
  unsigned ptype;
  unsigned pnum;
  if (gsp->pdslen == 0)
    return LONG_MAX;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 8:
    return -0.0;
    break;
  case 1:
  case 11:
    ptype = gsp->pds[34];
    pnum = gsp->pds[35];
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return nan("");
  }
  switch (ptype) {
  case 0:
    return 0.0;
    break;
  case 1:
    return -0.0;
    break;
  case 2:
    return -(double)pnum;
    break;
  case 3:
    return +(double)pnum;
    break;
  case 4:
    return 0.5;
    break;
  default:
    fprintf(stderr, "unsupported PDS 4.%u perturbation type %u\n",
      pdst, ptype);
    return nan("");
    break;
  }
}

// 引数を iparm_t にするためには次のケースを全部列挙しなければならない
  const char *
param_name(unsigned long iparm)
{
  static char buf[32];
  switch (iparm) {
  case 0x000000: return "T";
  case IPARM_papT: return "papT";
  case 0x000006: return "dT";
  case 0x000101: return "RH";
  case 0x000107: return "RR1H";
  case 0x000108: return "RAIN";
  case IPARM_WD: return "WD";
  case 0x000202: return "U";
  case 0x000203: return "V";
  case IPARM_WINDS: return "WINDS";
  case 0x000204: return "PSI";
  case 0x000205: return "CHI";
  case 0x000208: return "VVPa";
  case 0x00020c: return "rVOR";
  case 0x00020d: return "rDIV";
  case 0x000300: return "Pres";
  case 0x000301: return "Pmsl";
  case 0x000305: return "Z";
  case IPARM_CLA: return "CLA";
  case IPARM_CLL: return "CLL";
  case IPARM_CLM: return "CLM";
  case IPARM_CLH: return "CLH";
  default:
    sprintf(buf, "p%02lu-%03lu-%03lu-%03lu",
      iparm >> 24, (iparm >> 16) & 0xFF,
      (iparm >> 8) & 0xFF, iparm & 0xFF);
    return buf;
  }
}

  const char *
showtime(char *buf, size_t size, const struct tm *tp)
{
    strftime(buf, size, "%Y%m%dT%H%MZ", tp);
    return buf;
}

  void
get_reftime(struct tm *tp, const grib2secs_t *gsp)
{
  tp->tm_year = si2(gsp->ids + 12) - 1900;
  tp->tm_mon = gsp->ids[14] - 1;
  tp->tm_mday = gsp->ids[15];
  tp->tm_hour = gsp->ids[16];
  tp->tm_min = gsp->ids[17];
  tp->tm_sec = gsp->ids[18];
}

// エラーは 0
  size_t
get_npixels(const grib2secs_t *gsp)
{
  if (gsp->drslen == 0)
    return 0;
  return ui4(gsp->drs + 5);
}

  void
printx(unsigned char *ptr, size_t base)
{
  ptr += base;
  for (unsigned j=0u; j<4u; j++) {
    printf("%04lu:", base+j*16u);
    for (unsigned i=0u; i<16u; i++) {
      if (i == 8u) printf(" .");
      printf(" %02X", ptr[j*16u+i]);
    }
    printf("\n    ");
    for (unsigned i=0u; i<16u; i++) {
      if (i == 8u) printf("\n    ");
      putchar(' ');
      for (unsigned mask=0x80u; mask; mask >>= 1) {
        putchar((ptr[j*16u+i] & mask) ? '1' : '0');
      }
    }
    putchar('\n');
  }
}

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
      signed x, y, z;
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

// GRIB2 GDSからデータの投影法パラメタを bp に抽出する。
  gribscan_err_t
decode_gds(const grib2secs_t *gsp, bounding_t *bp)
{
  size_t gpixels;
  unsigned gsysno, gdt, unit;
  size_t npixels = get_npixels(gsp);
  // === 未サポートの状況の検知 ===
  // GDS 欠損
  if (gsp->gdslen == 0) {
    fprintf(stderr, "GDS missing\n");
    return ERR_BADGRIB;
  }
  // GDS に中身がなく既登録格子系番号で指示する場合 (obsolete)
  if ((gsysno = gsp->gds[5]) != 0) {
    fprintf(stderr, "Unsupported GDS#5 %u\n", gsysno);
    return ERR_UNSUPPORTED;
  }
  // GDS 格子数が DRS 格子数と不一致の場合（= ビットマップ使用時）
  if ((gpixels = ui4(gsp->gds + 6)) != npixels) {
    fprintf(stderr, "Pixels unmatch DRS %zu != GDS %zu\n", npixels, gpixels);
    return ERR_UNSUPPORTED;
  }
  // GDT が 5.0 (正距円筒図法) ではない場合
  if ((gdt = ui2(gsp->gds + 12)) != 0) {
    fprintf(stderr, "Unsupported GDT 5.%u\n", gdt);
    return ERR_UNSUPPORTED;
  }
  // 経緯度の単位が 1e-6 deg ではない場合
  if ((unit = ui4(gsp->gds + 38)) != 0) {
    fprintf(stderr, "Unsupported unit dividend %u\n", unit);
    return ERR_UNSUPPORTED;
  }
  if ((unit = ui4(gsp->gds + 42)) != 0xFFFFFFFF) {
    fprintf(stderr, "Unsupported unit divisor %u\n", unit);
    return ERR_UNSUPPORTED;
  }
  // GDS 格子数が ni*nj と不一致の場合 (thinned grid)
  bp->ni = si4(gsp->gds + 30);
  bp->nj = si4(gsp->gds + 34);
  if (npixels != bp->ni * bp->nj) {
    fprintf(stderr, "Unsupported npixels %zu != Ni %zu * Nj %zu\n", 
      npixels, bp->ni, bp->nj);
    return ERR_UNSUPPORTED;
  }
  // 主要要素デコード
  bp->n = si4(gsp->gds + 46) / 1.0e6;
  bp->w = si4(gsp->gds + 50) / 1.0e6;
  bp->s = si4(gsp->gds + 55) / 1.0e6;
  bp->e = si4(gsp->gds + 59) / 1.0e6;
  // 東端 bp->e が西経表示で大小関係が不正常な場合補正
  if (bp->e < bp->w) { bp->e += 360.0; }
  // 格子長 di との整合性チェック
  bp->di = si4(gsp->gds + 63) / 1.0e6;
  if (fabs(fabs((bp->e - bp->w) / (bp->ni - 1)) - fabs(bp->di)) > 1.0e-6) {
    fprintf(stderr, "GDS E %g - W %g != Ni %zu * Di %g\n",
      bp->e, bp->w, bp->ni, bp->di);
    return ERR_UNSUPPORTED;
  }
  // 格子長 dj との整合性チェック
  bp->dj = si4(gsp->gds + 67) / 1.0e6;
  if (fabs(fabs((bp->n - bp->s) / (bp->nj - 1)) - fabs(bp->dj)) > 1.0e-6) {
    fprintf(stderr, "GDS N %g - S %g != Nj %zu * Dj %g\n",
      bp->e, bp->w, bp->ni, bp->di);
    return ERR_UNSUPPORTED;
  }
  // 緯度円全円周あるかチェック
  if (fabs((bp->e - bp->w) * bp->ni / (bp->ni - 1) - 360.0) < 1.0e-6) {
    bp->wraplon = 1;
  } else {
    bp->wraplon = 0;
  }
  
  return GSE_OKAY;
}

// GRIB格子の緯度
  double
bp_lat(bounding_t *bp, double j)
{
  return bp->n + j * (bp->s - bp->n) / (bp->nj - 1.0);
}

// GRIB格子の経度
  double
bp_lon(bounding_t *bp, double i)
{
  double r = bp->w + i * (bp->e - bp->w) / (bp->ni - 1.0);
  if (r>180.0) r-=360.0;
  return r;
}

  const char *
level_name(double vlev)
{
  static char lvbuf[32];
  if (vlev == 101325.0) {
    return "sfc";
  } else if (vlev == 101324.0) {
    return "msl";
  } else if (vlev == VLEVEL_Z2M) {
    return "z2";
  } else if (vlev == VLEVEL_Z10M) {
    return "z10";
  } else {
    snprintf(lvbuf, sizeof lvbuf, "p%g", vlev / 100.0);
    return lvbuf;
  }
}

// GRIB2 の各節を fp から読み込んで、節の修飾関係に従って gsp に積み込んで、
// 第7節（データ節）が読めるたびに checksec7() を呼び出す。
  gribscan_err_t
grib2loopsecs(grib2secs_t *gsp, FILE *fp, const char *locator)
{
  // 気象庁1.25度格子GSM予報値の場合 recl = 62645 であり、多少大きく設定
  const size_t RECLZERO = 64 * 1024;
  gribscan_err_t r;
  size_t recl, zr;
  unsigned rectype;
  unsigned char *secbuf;
  for (;;) {
    secbuf = mymalloc(RECLZERO);
    if (secbuf == NULL) {
      return ERR_NOMEM;
    }
    zr = fread(secbuf, 1, 4, fp);
    if (zr != 4) {
      return ERR_IO;
    }
    recl = ui4(secbuf);
    /* 第8節を検出したら抜ける */
    if (recl == 0x37373737uL) {
      break;
    }
    if (recl > RECLZERO) {
      secbuf = realloc(secbuf, recl);
      if (secbuf == NULL) {
        return ERR_NOMEM;
      }
    }
    zr = fread(secbuf + 4, 1, recl - 4, fp);
    if (zr != recl - 4) {
      return ERR_IO;
    }
    rectype = secbuf[4];
    switch (rectype) {
case 1:
      if (gsp->ids) { myfree(gsp->ids); }
      gsp->ids = secbuf;
      gsp->idslen = recl;
      break;
case 3:
      if (gsp->gds) { myfree(gsp->gds); }
      gsp->gds = secbuf;
      gsp->gdslen = recl;
      break;
case 4:
      if (gsp->pds) { myfree(gsp->pds); }
      gsp->pds = secbuf;
      gsp->pdslen = recl;
      break;
case 5:
      if (gsp->drs) { myfree(gsp->drs); }
      gsp->drs = secbuf;
      gsp->drslen = recl;
      break;
case 6:
      // BMS オクテット6の値が 254 なら読み飛ばす
      // gsp->bms は直近の bms のままになるので、それが 254 の意味である
      if (secbuf[5] == 254) { break; }
      if (gsp->bms) { myfree(gsp->bms); }
      gsp->bms = secbuf;
      gsp->bmslen = recl;
      break;
case 7:
      // gsp->ds は checksec7() によって破棄されるから myfree しない
      gsp->ds = secbuf;
      gsp->dslen = recl;
      r = checksec7(gsp);
      gsp->ds = NULL;
      if (!(r == GSE_OKAY || r == GSE_SKIP)) { return r; }
      break;
default:
      fprintf(stderr, "%s: Unsupported section type %u\n", locator, rectype);
      break;
    }
  }
  return GSE_OKAY;
}

/* 解読構造体 grib2secs を malloc() して初期化して返す。
 */
  grib2secs_t *
new_grib2secs(const unsigned char ids[12])
{
  grib2secs_t *r;
  r = mymalloc(sizeof(grib2secs_t));
  if (r == NULL) { return NULL; }
  r->discipline = ids[2];
  r->msglen = ui4(ids + 4);
  r->msglen <<= 32;
  r->msglen |= ui4(ids + 8);
  r->ids = r->gds = r->pds = r->drs = r->bms = r->ds = NULL;
  r->idslen = r->gdslen = r->pdslen = r->drslen = r->bmslen = r->dslen = 0;
  return r;
}

/* 解読構造体を保存用に複写。
 */
  grib2secs_t *
dup_grib2secs(const grib2secs_t *gsp)
{
  grib2secs_t *r = mymalloc(sizeof(grib2secs_t));
  if (r == NULL) { return NULL; }
  r->discipline = gsp->discipline;
  r->msglen = 0;
  r->ids = r->gds = r->pds = r->drs = r->bms = r->ds = NULL;
  if (gsp->ids) {
    if (!(r->ids = mydup(gsp->ids, gsp->idslen))) return NULL;
    r->idslen = gsp->idslen;
  }
  if (gsp->gds) {
    if (!(r->gds = mydup(gsp->gds, gsp->gdslen))) return NULL;
    r->gdslen = gsp->gdslen;
  }
  if (gsp->pds) {
    if (!(r->pds = mydup(gsp->pds, gsp->pdslen))) return NULL;
    r->pdslen = gsp->pdslen;
  }
  if (gsp->drs) {
    if (!(r->drs = mydup(gsp->drs, gsp->drslen))) return NULL;
    r->drslen = gsp->drslen;
  }
  if (gsp->bms) {
    if (!(r->bms = mydup(gsp->bms, gsp->bmslen))) return NULL;
    r->pdslen = gsp->bmslen;
  }
  if (gsp->ds) {
    if (!(r->ds = mydup(gsp->ds, gsp->dslen))) return NULL;
    r->pdslen = gsp->dslen;
  }
  return r;
}

/* 解読構造体 grib2secs を破棄、要すれば各節のメモリを破棄してから。
 */
  void
del_grib2secs(grib2secs_t *gsp)
{
  if (gsp->ids) { myfree(gsp->ids); }
  if (gsp->gds) { myfree(gsp->gds); }
  if (gsp->pds) { myfree(gsp->pds); }
  if (gsp->drs) { myfree(gsp->drs); }
  if (gsp->bms) { myfree(gsp->bms); }
  if (gsp->ds) { myfree(gsp->ds); }
  myfree(gsp);
}

/* GRIB2 電文の解読。
 * 前提としてマジックナンバー "GRIB" が読み込まれた状態で呼ばれる。
 * この関数は IDS の残り12オクテットを読み込み、残りの節は
 * grib2loopsecs() で解読する。
 */
  gribscan_err_t
grib2decode(FILE *fp, const char *locator)
{
  gribscan_err_t r = GSE_OKAY;
  unsigned char ids[12];
  size_t zr;
  /* 解読結果を保持する構造体 */
  grib2secs_t *gsp;
  /* section 0 IDS */
  zr = fread(ids, 1, 12, fp);
  if (zr < 12) {
    fputs("EOF in IDS", stderr);
    return ERR_OVERRUN;
  }
  if (ids[3] != 2u) {
    fprintf(stderr, "Unsupported GRIB Edition %u\n", ids[3]);
    return GSE_JUSTWARN;
  }
  gsp = new_grib2secs(ids);
  if (gsp == NULL) {
    return ERR_NOMEM;
  }
  r = grib2loopsecs(gsp, fp, locator);
  del_grib2secs(gsp);
  return r;
}

/*
 * ファイル名 fnam を開き、バイト列 "GRIB" を探し、そこからGRIBとして解読する。
 */
  gribscan_err_t
grib2scan_by_filename(const char *fnam)
{
  gribscan_err_t r = GSE_OKAY;
  FILE *fp;
  long lpos = 0;
  int c;
  int state = 0;
  fp = fopen(fnam, "rb");
  if (fp == NULL) { goto error; }
#if 0
  fprintf(stderr, "=== file %s ===\n", fnam);
#endif
  /* automaton to find the magic number "GRIB */
  while ((c = getc(fp)) != EOF) {
    switch (state) {
    default:
    case 0:
      if (c == 'G') { state = c; lpos = ftell(fp); } else { state = 0; }
      break;
    case 'G':
      if (c == 'R') { state = c; } else { state = 0; }
      break;
    case 'R':
      if (c == 'I') { state = c; } else { state = 0; }
      break;
    case 'I':
      if (c == 'B') {
        char locator[32];
        snprintf(locator, sizeof locator, "%-.24s:%lu",
          fnam + ((strlen(fnam) > 24) ? (strlen(fnam) - 24) : 0), lpos);
        r = grib2decode(fp, locator);
        if (r != GSE_OKAY) {
          fprintf(stderr, "%s: GRIB decode %u\n", locator, r);
          if (r != GSE_JUSTWARN) {
            goto klose;
          }
        }
      }
      state = 0;
      break;
    }
  }
klose:
  if (fclose(fp) != 0) { goto error; }
  return r;
error:
  perror(fnam);
  return ERR_IO;
}
