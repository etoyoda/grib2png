#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#include "gribscan.h"

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
  if (nbits == 7u) {
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
  c0 = (buf[0] << (nbits + bitofs - 8u)) & ((1u << nbits) - 1u);
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
get_parameter(const struct grib2secs *gsp)
{
  unsigned pdst;
  unsigned long r;
  r = gsp->discipline << 16;
  if (gsp->pdslen == 0)
    return ~0ul;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 8:
    r |= ui2(gsp->pds + 9);
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return ~0ul;
  }
  return r;
}

  long
get_ftime(const struct grib2secs *gsp)
{
  long r;
  unsigned tunits;
  unsigned pdst;
  if (gsp->pdslen == 0)
    return LONG_MAX;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 8:
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
get_duration(const struct grib2secs *gsp)
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
    return 0;
  case 8:
    nranges = gsp->pds[41];
    tunits = gsp->pds[48];
    r = ui4(gsp->pds + 49);
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
get_vlevel(const struct grib2secs *gsp)
{
  double r;
  unsigned pdst, vtype;
  const unsigned char *vlevptr;
  if (gsp->pdslen == 0)
    return LONG_MAX;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 8:
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

// 引数を iparm_t にするためには次のケースを全部列挙しなければならない
  const char *
param_name(unsigned long iparm)
{
  static char buf[32];
  switch (iparm) {
  case 0x000000: return "T";
  case 0x000006: return "dT";
  case 0x000101: return "RH";
  case 0x000107: return "RR1H";
  case 0x000108: return "RAIN";
  case 0x000202: return "U";
  case 0x000203: return "V";
  case 0x000204: return "PSI";
  case 0x000205: return "CHI";
  case 0x000208: return "VVPa";
  case 0x00020c: return "rVOR";
  case 0x00020d: return "rDIV";
  case 0x000300: return "Pres";
  case 0x000301: return "Pmsl";
  case 0x000305: return "Z";
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
get_reftime(struct tm *tp, const struct grib2secs *gsp)
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
get_npixels(const struct grib2secs *gsp)
{
  if (gsp->drslen == 0)
    return 0;
  return ui4(gsp->drs + 5);
}

  gribscan_err_t
decode_ds(const struct grib2secs *gsp, double *dbuf)
{
  size_t npixels;
  unsigned drstempl;
  int scale_e;
  int scale_d;
  unsigned width;
  iparm_t param;
  if ((gsp->drslen == 0) || (gsp->dslen == 0)) {
    fprintf(stderr, "missing DRS %zu DS %zu\n", gsp->drslen, gsp->dslen);
    return ERR_BADGRIB;
  }
  npixels = ui4(gsp->drs + 5);
  drstempl = ui2(gsp->drs + 9);
  if (drstempl != 0) {
    fprintf(stderr, "unsupported DRS template 5.%u\n", drstempl);
    return ERR_BADGRIB;
  }
  float refv = float32(gsp->drs + 11);
  scale_e = si2(gsp->drs + 15);
  scale_d = si2(gsp->drs + 17);
  width = gsp->drs[19];
  // 特定パラメタの場合十進尺度 scale_d を補正する
  // 海面気圧: hPa 単位に変換
  // 渦度または発散: 1e-6/s 単位に変換
  // 気温または露点: 0.1 K 単位に変換
  // 積算降水量: 0.1 mm 単位に変換
  switch (param = get_parameter(gsp)) {
case IPARM_Pmsl:
    scale_d += 2;
    break;
case IPARM_rDIV:
case IPARM_rVOR:
    scale_d -= 6;
    break;
case IPARM_T:
case IPARM_dT:
case IPARM_RAIN:
    scale_d -= 1;
    break;
default:
    break;
  }
  for (unsigned i = 0; i < npixels; i++) {
    dbuf[i] = (refv + ldexp(unpackbits(gsp->ds + 5, width, i), scale_e))
      * pow(10.0, -scale_d);
  }
  return GSE_OKAY;
}

// GRIB2 の各節を fp から読み込んで、節の修飾関係に従って gsp に積み込んで、
// 第7節（データ節）が読めるたびに checksec7() を呼び出す。
  gribscan_err_t
grib2loopsecs(struct grib2secs *gsp, FILE *fp, const char *locator)
{
  // 気象庁1.25度格子GSM予報値の場合 recl = 62645 であり、多少大きく設定
  const size_t RECLZERO = 64 * 1024;
  gribscan_err_t r;
  size_t recl, zr;
  unsigned rectype;
  unsigned char *secbuf;
  for (;;) {
    secbuf = malloc(RECLZERO);
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
      if (gsp->ids) { free(gsp->ids); }
      gsp->ids = secbuf;
      gsp->idslen = recl;
      break;
case 3:
      if (gsp->gds) { free(gsp->gds); }
      gsp->gds = secbuf;
      gsp->gdslen = recl;
      break;
case 4:
      if (gsp->pds) { free(gsp->pds); }
      gsp->pds = secbuf;
      gsp->pdslen = recl;
      break;
case 5:
      if (gsp->drs) { free(gsp->drs); }
      gsp->drs = secbuf;
      gsp->drslen = recl;
      break;
case 6:
      if (gsp->bms) { free(gsp->bms); }
      gsp->bms = secbuf;
      gsp->bmslen = recl;
      break;
case 7:
      if (gsp->ds) { free(gsp->ds); }
      gsp->ds = secbuf;
      gsp->dslen = recl;
      r = checksec7(gsp);
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
  struct grib2secs *
new_grib2secs(const unsigned char ids[12])
{
  struct grib2secs *r;
  r = malloc(sizeof(struct grib2secs));
  if (r == NULL) { return NULL; }
  r->discipline = ids[2];
  r->msglen = ui4(ids + 4);
  r->msglen <<= 32;
  r->msglen |= ui4(ids + 8);
  r->ids = r->gds = r->pds = r->drs = r->bms = r->ds = NULL;
  r->idslen = r->gdslen = r->pdslen = r->drslen = r->bmslen = r->dslen = 0;
  return r;
}

/* 解読構造体 grib2secs を破棄、要すれば各節のメモリを破棄してから。
 */
  void
del_grib2secs(struct grib2secs *gsp)
{
  if (gsp->ids) { free(gsp->ids); }
  if (gsp->gds) { free(gsp->gds); }
  if (gsp->pds) { free(gsp->pds); }
  if (gsp->drs) { free(gsp->drs); }
  if (gsp->bms) { free(gsp->bms); }
  if (gsp->ds) { free(gsp->ds); }
  free(gsp);
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
  struct grib2secs *gsp;
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
  return r;
}
