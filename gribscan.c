#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits.h>

#include "gribscan.h"

  unsigned long
ui4(const unsigned char *buf)
{
  return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

  int
si2(const unsigned char *buf)
{
  long r;
  r = ((buf[0] & 0x7Fu) << 8) | buf[1];
  return (buf[0] & 0x80u) ? -r : r;
}

  unsigned
ui2(const unsigned char *buf)
{
  return (buf[0] << 8) | buf[1];
}

#if 0
  size_t
ui3(const unsigned char *buf)
{
  return (buf[0] << 16) | (buf[1] << 8) | buf[2];
}

  long
si3(const unsigned char *buf)
{
  long r;
  r = ((buf[0] & 0x7Fu) << 16) | (buf[1] << 8) | buf[2];
  return (buf[0] & 0x80u) ? -r : r;
}

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

  unsigned
unpackbits(const unsigned char *buf, size_t nbits, size_t pos)
{
  size_t byteofs = (nbits * pos) / 8u;
  size_t bitofs = (nbits * pos) % 8u;
  return getbits(buf + byteofs, bitofs, nbits);
}

#define MYASSERT1(test, _plusfmt, val) \
  if (!(test)) { \
    fprintf(stderr, "assert(%s) " _plusfmt "\n", #test, val); \
    return ERR_BADGRIB; \
  }

#define MYASSERT3(test, _plusfmt, v1, v2, v3) \
  if (!(test)) { \
    fprintf(stderr, "assert(%s) " _plusfmt "\n", #test, v1, v2, v3); \
    return ERR_BADGRIB; \
  }

#define WEAK_ASSERT1(test, _plusfmt, val) \
  if (!(test)) { \
    fprintf(stderr, "assert(%s) " _plusfmt "\n", #test, val); \
    return GSE_JUSTWARN; \
  }
#endif

/*
 * 4 オクテットでパラメタを次の構造で表現する
 * 構造:
 * 0xFF000000: 予約 (暫定ゼロ、ローカルの処理に使用)
 * 0x00FF0000: discipline
 * 0x0000FF00: category
 * 0x000000FF: parameter
 * ~0: エラー
 * 用例:
 * 0x00000000: temperature
 * 0x00000101: relative humidity
 * 0x00000108: total precipitation
 * 0x00000202: u component of wind
 * 0x00000203: v component of wind
 * 0x0000020c: relative vorticity
 * 0x0000020d: relative divergence
 * 0x00000301: sea level pressure
 * 0x00000305: geopotential height
 */
  unsigned long
get_parameter(const struct grib2secs *gsp)
{
  unsigned pdst;
  unsigned long r;
  r = gsp->discipline << 16;
  if (gsp->pdslen == 0)
    return ~0;
  pdst = ui2(gsp->pds + 7);
  switch (pdst) {
  case 0:
  case 8:
    r |= ui2(gsp->pds + 9);
    break;
  default:
    fprintf(stderr, "unsupported PDS template 4.%u\n", pdst);
    return ~0;
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
    strftime(buf, size, "%Y-%m-%dT%H:%MZ", tp);
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

/*
 * GRIB報buf（長さbuflenバイト）を解読する。
 */
  enum gribscan_err_t
scanmsg(unsigned char *buf, size_t buflen, const char *locator)
{
  enum gribscan_err_t r;
  unsigned rectype;
  size_t recl, pos;
  struct grib2secs gs;
  gs.ids = gs.gds = gs.pds = gs.drs = gs.bms = gs.ds = NULL;
  gs.idslen = gs.gdslen = gs.pdslen = gs.drslen = gs.bmslen = gs.dslen = 0;
  gs.discipline = buf[6];
  for (pos = 16u; pos <= buflen - 4; pos += recl) {
    recl = ui4(buf + pos);
    if (recl == 0x37373737uL) {
      /* 第8節を検出 */
      break;
    }
    rectype = buf[pos+4];
    switch (rectype) {
case 1:
      gs.ids = buf + pos;
      gs.idslen = recl;
      break;
case 3:
      gs.gds = buf + pos;
      gs.gdslen = recl;
      break;
case 4:
      gs.pds = buf + pos;
      gs.pdslen = recl;
      break;
case 5:
      gs.drs = buf + pos;
      gs.drslen = recl;
      break;
case 6:
      gs.bms = buf + pos;
      gs.bmslen = recl;
      break;
case 7:
      gs.ds = buf + pos;
      gs.dslen = recl;
      r = checksec7(&gs);
      if (r != GSE_OKAY)
        return r;
      break;
default:
      fprintf(stderr, "%s:%lu %u\n", locator, (unsigned long)pos, rectype);
      break;
    }
  }
  return GSE_OKAY;
}

/*
 * IDS(GRIB第0節, "GRIB"に続く12バイト)を読み込んで、GRIB第2版であれば
 * 電文長だけ読み込んで解読する。
 */
  enum gribscan_err_t
gdecode(FILE *fp, const char *locator)
{
  enum gribscan_err_t r = GSE_OKAY;
  unsigned char ids[12];
  size_t zr, msglen;
  unsigned char *msgbuf;
  /* section 0 IDS */
  zr = fread(ids, 1, 12, fp);
  if (zr < 12) {
    fputs("EOF in IDS", stderr);
    return ERR_OVERRUN;
  }
  if (ids[3] != 2u) {
    fputs("Not GRIB Edition 2\n", stderr);
    return GSE_JUSTWARN;
  }
  msglen = ui4(ids + 4);
  if (msglen != 0u) {
    fputs("GRIB message size 2GiB or more not supported\n", stderr);
    return GSE_JUSTWARN;
  }
  msglen = ui4(ids + 8);
  msgbuf = malloc(msglen);
  if (msgbuf == NULL) { return ERR_NOMEM; }
  /* --- begin ensure malloc-free --- */
  memcpy(msgbuf+0, "GRIB", 4);
  memcpy(msgbuf+4, ids, 12);
  zr = fread(msgbuf + 16, 1, msglen - 16, fp);
  if (zr < msglen - 16) {
    fputs("EOF in GRIB\n", stderr);
    r = ERR_OVERRUN;
    goto free_and_return;
  }
  r = scanmsg(msgbuf, msglen, locator);
free_and_return:
  free(msgbuf);
  /* --- end ensure malloc-free --- */
  return r;
}

/*
 * ファイル名 fnam を開き、バイト列 "GRIB" を探し、そこからGRIBとして解読する。
 */
  enum gribscan_err_t
scandata(const char *fnam)
{
  enum gribscan_err_t r = GSE_OKAY;
  FILE *fp;
  long lpos = 0;
  int c;
  int state = 0;
  fp = fopen(fnam, "rb");
  if (fp == NULL) { goto error; }
  fprintf(stderr, "=== file %s ===\n", fnam);
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
        r = gdecode(fp, locator);
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
