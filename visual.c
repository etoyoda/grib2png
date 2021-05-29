// visual.c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <png.h>
#include <math.h>
#include "visual.h"


// libpng に渡せる形の RGBA バッファを作る。 
  png_bytep *
new_pngimg(size_t owidth, size_t oheight)
{
  png_bytep *r;
  r = malloc(sizeof(png_bytep) * (oheight + 1));
  if (r == NULL) { return NULL; }
  for (int j = 0; j < oheight; j++) {
    r[j] = malloc(owidth * sizeof(png_byte) * 4);
    if (r[j] == NULL) { return NULL; }
  }
  r[oheight] = NULL;
  return r;
}

  void
png_set_textv(png_structp png, png_infop info, char **textv)
{
  const int ntext = 2;
  png_text textbuf[ntext];
  textbuf[0].key = "Software";
  textbuf[1].key = "Source";
  for (int i = 0; i < ntext; i++) {
    textbuf[i].compression = PNG_TEXT_COMPRESSION_NONE;
    textbuf[i].text = textv[i];
    textbuf[i].text_length = strlen(textbuf[i].text);
  }
  png_set_text(png, info, textbuf, ntext);
}

// RGBA バッファ ovector (寸法 owidth * oheight) をファイル filename に出力
  int
write_pngimg(png_bytep *ovector, size_t owidth, size_t oheight,
  const char *filename, char **textv)
{
  int r = 0;
  // ファイルを開く
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) { return 'I'; }
  // PNGヘッダ構造体の作成初期化
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL);
  if (png == NULL) { r = 'I'; errno = EDOM; goto err_fclose; }
  png_infop info = png_create_info_struct(png);
  if (info == NULL) { r = 'I'; errno = EDOM; goto err_png; }
  if (setjmp(png_jmpbuf(png))) { errno = EDOM; goto err_png; }
  // テキストヘッダ記入
  png_set_textv(png, info, textv);
  // ファイル書き出し
  png_init_io(png, fp);
  png_set_IHDR(png, info, owidth, oheight, 8, PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);
  png_write_image(png, ovector);
  png_write_end(png, NULL);
  
err_png:
  png_destroy_write_struct(&png, &info);
err_fclose:
  if (fclose(fp) != 0) { return 'I'; }
  return r;
}

// new_pngimg() で確保したメモリ領域を開放する。
  void
del_pngimg(png_bytep *ovector)
{
  for (unsigned j = 0; ovector[j]; j++) {
    free(ovector[j]);
  }
  free(ovector);
}

  void
setpixel_z(png_bytep pixel, double val)
{
  int blue;
  double cycle = (val > 6360.0) ? 120.0 : 60.0;
  // val は 1 m 単位、縞々透過は 60 m 単位 (6000 m 以上では 120 m 単位)
  long istep = floor(val / cycle); 
  unsigned frac = (unsigned)((val - istep * cycle) * 0x100u / cycle);
  if (val < 1000.0) { // 925hPa用
    blue = (istep - 720/60) * 24 + 0x80;
  } else if (val < 2160.0) { // 850hPa用
    blue = (istep - 1440/60) * 16 + 0x80;
  } else if (val < 4200.0) { // 700hPa用
    blue = (istep - 3000/60) * 16 + 0x80;
  } else if (val < 6360.0) { // 500hPa用
    blue = (istep - 5520/60) * 16 + 0x80;
  } else if (val < 8160.0) { // 400hPa用
    blue = (istep - 7200/120) * 24 + 0x80;
  } else if (val < 9720.0) { // 300hPa用
    blue = (istep - 9120/120) * 24 + 0x80;
  } else if (val < 11040.0) { // 250hPa用
    blue = (istep - 10320/120) * 24 + 0x80;
  } else { // 200hPa用
    blue = (istep - 11760/120) * 24 + 0x80;
  }
  int red = 0xFF - blue;
  pixel[0] = (red < 0) ? 0 : (red > 0xFF) ? 0xFF : red;
  pixel[1] = 0x80;
  pixel[2] = (blue < 0) ? 0 : (blue > 0xFF) ? 0xFF : blue;
  pixel[3] = frac;
}

  void
setpixel_rh(png_bytep pixel, double val)
{
  // val は 1 % 単位、10 % 単位で色を変えて、透過率は80%を境に大きく変える
  long istep = floor(val / 10.0);
  long ival = floor(val);
  switch (istep * 10) {
  default:
  case 100: pixel[0] = 0; pixel[1] = 77; pixel[2] = 64; break;
  case  90: pixel[0] = 0; pixel[1] = 153; pixel[2] = 128; break;
  case  80: pixel[0] = 31; pixel[1] = 204; pixel[2] = 175; break;
  case  70: pixel[0] = 73; pixel[1] = 243; pixel[2] = 214; break;
  case  60: pixel[0] = 255; pixel[1] = 255; pixel[2] = 240; break;
  case  50: pixel[0] = 255; pixel[1] = 229; pixel[2] = 191; break;
  case  40: pixel[0] = 255; pixel[1] = 200; pixel[2] = 70; break;
  case  30: pixel[0] = 245; pixel[1] = 120; pixel[2] = 15; break;
  case  20: pixel[0] = 120; pixel[1] = 55; pixel[2] = 5; break;
  case  10: pixel[0] = 60; pixel[1] = 30; pixel[2] = 5; break;
  case   0: pixel[0] = 0; pixel[1] = 38; pixel[2] = 38; break;
  }
  pixel[3] = (ival > 100) ? 0xFF
    : (ival >= 80) ? (255 - (100 - ival) * 2)
    : (ival >= 0) ? ival * 2
    : 0;
}

  void
setpixel_papt(png_bytep pixel, double val)
{
  // val は 0.1 K 単位、6 K 単位で縞々透過をつける
  long istep = floor(val / 60.0);
  unsigned frac = (unsigned)((val - istep * 60.0) * 0x100u / 60.0);
  switch (istep * 6) {
  case 354:
  case 348:
    pixel[0] = 145; pixel[1] = 0; pixel[2] = 83; break;
  case 342:
  case 336:
    pixel[0] = 255; pixel[1] = 26; pixel[2] = 26; break;
  case 330:
  case 324:
    pixel[0] = 255; pixel[1] = 153; pixel[2] = 0; break;
  case 318:
  case 312:
    pixel[0] = 255; pixel[1] = 240; pixel[2] = 0; break;
  case 306:
  case 300:
    pixel[0] = 255; pixel[1] = 240; pixel[2] = 180; break;
  case 294:
  case 288:
    pixel[0] = 153; pixel[1] = 238; pixel[2] = 255; break;
  case 282:
  case 276:
    pixel[0] = 0; pixel[1] = 191; pixel[2] = 255; break;
  case 270:
  case 264:
    pixel[0] = 0; pixel[1] = 126; pixel[2] = 255; break;
  case 258:
  case 252:
    pixel[0] = 33; pixel[1] = 33; pixel[2] = 255; break;
  default:
    pixel[0] = 0; pixel[1] = 0; pixel[2] = 112; break;
  }
  pixel[3] = frac;
}

  void
setpixel_t(png_bytep pixel, double val)
{
  // val は 0.1 K 単位、3 K 単位で縞々透過をつける
  long istep = floor((val - 2731.5) / 30.0);
  unsigned frac = (unsigned)(((val - 2731.5) - istep * 30.0) * 0x100u / 30.0);
  switch (istep * 3) {
  case 36:
  case 35:
  case 33:
  case -33:
  case -36:
    pixel[0] = 180; pixel[1] = 0; pixel[2] = 104; break;
  case 30:
  case -39:
  case -42:
  case -45:
  case -48:
  case -51:
  case -54:
  case -57:
  case -60:
    pixel[0] = 255; pixel[1] = 40; pixel[2] = 0; break;
  case 27:
  case 25:
  case 24:
    pixel[0] = 255; pixel[1] = 153; pixel[2] = 0; break;
  case 21:
  case 20:
  case 18:
    pixel[0] = 250; pixel[1] = 245; pixel[2] = 0; break;
  case 15:
    pixel[0] = 255; pixel[1] = 255; pixel[2] = 150; break;
  case 12:
  case 10:
  case 9:
    pixel[0] = 255; pixel[1] = 255; pixel[2] = 240; break;
  case 6:
  case 5:
  case 3:
    pixel[0] = 185; pixel[1] = 235; pixel[2] = 255; break;
  case 0:
    pixel[0] = 0; pixel[1] = 150; pixel[2] = 255; break;
  case -3:
  case -5:
  case -6:
    pixel[0] = 0; pixel[1] = 65; pixel[2] = 255; break;
  case -9:
  case -10:
  default:
    pixel[0] = 0; pixel[1] = 32; pixel[2] = 128; break;
  }
  pixel[3] = frac;
}

  void
setpixel_pmsl(png_bytep pixel, double val)
{
  // val は 0.1 hPa 単位、4 hPa 単位で縞々透過をつける
  long istep = floor(val / 40.0);
  unsigned frac = (unsigned)((val - istep * 40.0) * 0x100u / 40.0);
  // 1013 hPa => istep=253 を中心に
  int red = (1013/4 - istep) * 8 + 0x80;
  int blue = (istep - 1013/4) * 8 + 0x80;
  pixel[0] = (red < 0) ? 0 : (red > 0xFF) ? 0xFF : red;
  pixel[1] = 0x80;
  pixel[2] = (blue < 0) ? 0 : (blue > 0xFF) ? 0xFF : blue;
  pixel[3] = frac;
}

  void
setpixel_gsi(png_bytep pixel, double val)
{
  unsigned long ival;
  if (val >= 0.0) {
    ival = val + 0.5;
    pixel[0] = (ival << 16) & 0x7F;
    pixel[1] = (ival <<  8) & 0xFF;
    pixel[2] =  ival        & 0xFF;
    pixel[3] = 0xFF;
  } else {
    ival = -val + 0.5;
    pixel[0] = ((ival << 16) & 0x7F) | 0x80;
    pixel[1] = (ival <<  8) & 0xFF;
    pixel[2] =  ival        & 0xFF;
    pixel[3] = 0xFF;
  }
}

  int
render(png_bytep *ovector, const double *gbuf,
  size_t owidth, size_t oheight, palette_t pal)
{
  for (size_t j = 0; j < oheight; j++) {
    for (size_t i = 0; i < owidth; i++) {
      png_bytep pixel = ovector[j] + i * 4;
      switch (pal) {
      case PALETTE_Z:
        setpixel_z(pixel, gbuf[i + j * owidth]);
        break;
      case PALETTE_RH:
        setpixel_rh(pixel, gbuf[i + j * owidth]);
        break;
      case PALETTE_papT:
        setpixel_papt(pixel, gbuf[i + j * owidth]);
        break;
      case PALETTE_T:
        setpixel_t(pixel, gbuf[i + j * owidth]);
        break;
      case PALETTE_Pmsl:
        setpixel_pmsl(pixel, gbuf[i + j * owidth]);
        break;
      default:
        setpixel_gsi(pixel, gbuf[i + j * owidth]);
        break;
      }
    }
  }
  return 0;
}


  int
gridsave(double *gbuf, size_t owidth, size_t oheight, palette_t pal,
  const char *filename, char **textv)
{
  png_bytep *ovector;
  int r = 0;
  // 1. RGBイメージメモリ確保
  ovector = new_pngimg(owidth, oheight);
  if (ovector == NULL) return 'M';
  // 2. データ変換（double値→RGBA）
  r = render(ovector, gbuf, owidth, oheight, pal);
  if (r != 0) goto badend;
  // 3. PNG書き出し
  r = write_pngimg(ovector, owidth, oheight, filename, textv);
badend:
  // 4. メモリ開放
  del_pngimg(ovector);
  return r;
}

