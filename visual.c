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

// コンター用なので pixel[3] を上書きしない
  void
setpixel_z(png_bytep pixel, double val)
{
  int blue;
  double cycle = (val > 6360.0) ? 120.0 : 60.0;
  // val は 1 m 単位、縞々透過は 60 m 単位 (6000 m 以上では 120 m 単位)
  long istep = floor(val / cycle); 
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
  } else if (val < 10320.0) { // 300hPa用
    blue = (istep - 9120/120) * 24 + 0x80;
  } else { // 200hPa用
    blue = (istep - 11760/120) * 24 + 0x80;
  }
  int red = 0xFF - blue;
  pixel[0] = (red < 0) ? 0 : (red > 0xFF) ? 0xFF : red;
  pixel[1] = 0x80;
  pixel[2] = (blue < 0) ? 0 : (blue > 0xFF) ? 0xFF : blue;
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
  pixel[3] = (ival > 80) ? 0xFF
    : (ival < 30) ? 0x80
    : 0x40;
}

  void
setpixel_papt(png_bytep pixel, double val)
{
  // val は 0.1 K 単位、6 K 単位で縞々透過をつける
  long istep = floor(val / 60.0);
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
  pixel[3] = 0x80;
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
  case -12:
  default:
    frac /= 2;
    pixel[0] = 0; pixel[1] = 65; pixel[2] = 255; break;
  case -15:
  case -18:
  case -21:
  case -24:
    frac /= 2;
    pixel[0] = 64; pixel[1] = 32; pixel[2] = 128; break;
  case -27:
  case -30:
    pixel[0] = 64; pixel[1] = 32; pixel[2] = 128; break;
  }
  pixel[3] = frac;
}

  void
setpixel_rain6(png_bytep pixel, double val)
{
  // 通報値 (0.1mm/6h) を降水強度 (mm/h) に換算するならば
  // 素朴には 1/60 倍する。かなり弱すぎる表現になるが
  const double factor = 1.0/60.0;
  int mmh = floor(val * factor);
  if (mmh < 1) {
    pixel[0] = 80; pixel[1] = 105; pixel[2] = 255;
    pixel[3] = floor(val * factor * 255);
    if (pixel[3] < 0xC0) { pixel[0] = pixel[1] = 0; }
  } else if (mmh < 2) {
    pixel[0] = 242; pixel[1] = 242; pixel[2] = 255; pixel[3] = 255;
  } else if (mmh < 5) {
    pixel[0] = 160; pixel[1] = 210; pixel[2] = 255; pixel[3] = 255;
  } else if (mmh < 10) {
    pixel[0] = 33; pixel[1] = 140; pixel[2] = 255; pixel[3] = 255;
  } else if (mmh < 20) {
    pixel[0] = 0; pixel[1] = 65; pixel[2] = 255; pixel[3] = 255;
  } else if (mmh < 30) {
    pixel[0] = 250; pixel[1] = 245; pixel[2] = 0; pixel[3] = 255;
  } else if (mmh < 50) {
    pixel[0] = 255; pixel[1] = 153; pixel[2] = 0; pixel[3] = 255;
  } else if (mmh < 80) {
    pixel[0] = 255; pixel[1] = 40; pixel[2] = 0; pixel[3] = 255;
  } else {
    pixel[0] = 180; pixel[1] = 0; pixel[2] = 104; pixel[3] = 255;
  }
}

  void
setpixel_windsfc(png_bytep pixel, double val)
{
  // 通報値 (0.1 m/s)
  int mps = floor(val * 0.1);
  if (mps < 5) {
    pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
  } else if (mps < 10) {
    pixel[0] = 160; pixel[1] = 210; pixel[2] = 255; pixel[3] = 0x80;
  } else if (mps < 15) {
    pixel[0] = 250; pixel[1] = 245; pixel[2] = 0; pixel[3] = 0x80;
  } else if (mps < 20) {
    pixel[0] = 255; pixel[1] = 153; pixel[2] = 0; pixel[3] = 0x80;
  } else if (mps < 25) {
    pixel[0] = 255; pixel[1] = 40; pixel[2] = 0; pixel[3] = 0x80;
  } else if (mps < 30) {
    pixel[0] = 180; pixel[1] = 0; pixel[2] = 104; pixel[3] = 0x80;
  } else {
    pixel[0] = 100; pixel[1] = 0; pixel[2] = 80; pixel[3] = 0xC0;
  }
}

  void
setpixel_winds(png_bytep pixel, double val)
{
  // 通報値 (0.1 m/s)
  int mps = floor(val * 0.1);
  if (mps < 1) {
    pixel[0] = 64; pixel[1] = 242; pixel[2] = 255; pixel[3] = 0x80;
  } else if (mps < 25) {
    pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
  } else if (mps < 30) {
    pixel[0] = 160; pixel[1] = 210; pixel[2] = 255; pixel[3] = 0x80;
  } else if (mps < 40) {
    pixel[0] = 250; pixel[1] = 245; pixel[2] = 0; pixel[3] = 0x80;
  } else if (mps < 50) {
    pixel[0] = 255; pixel[1] = 153; pixel[2] = 0; pixel[3] = 0x80;
  } else if (mps < 70) {
    pixel[0] = 255; pixel[1] = 40; pixel[2] = 0; pixel[3] = 0x80;
  } else if (mps < 100) {
    pixel[0] = 180; pixel[1] = 0; pixel[2] = 104; pixel[3] = 0x80;
  } else {
    pixel[0] = 100; pixel[1] = 0; pixel[2] = 80; pixel[3] = 0xC0;
  }
}

// コンター用なので pixel[3] を上書きしない
  void
setpixel_pmsl(png_bytep pixel, double val)
{
  // val は 0.1 hPa 単位、4 hPa 単位で縞々透過をつける
  long ival = (long)floor(val / 40.0) * 4;
  if (ival >= 1040) {
    pixel[0] = 0;
    pixel[1] = 0;
    pixel[2] = 0xFF;
  } else if (ival > 1012) {
    pixel[0] = 0;
    pixel[1] = 0x80;
    pixel[2] = 0xFF;
  } else if (ival > 980) {
    pixel[0] = 0xFF;
    pixel[1] = 0x80;
    pixel[2] = 0;
  } else {
    pixel[0] = 0xFF;
    pixel[1] = 0;
    pixel[2] = 0;
  }
}

  void
setpixel_rvor(png_bytep pixel, double val)
{
  int ival = floor(val);
  // val は 1e-6/s 単位
  if (ival < -128) {
    pixel[0] = 0;
    pixel[1] = 0x60;
    pixel[2] = 0x80;
    pixel[3] = 0x80;
  } else if (ival < 0) {
    pixel[0] = 0;
    pixel[1] = 0x80 + ival/4;
    pixel[2] = 0xFF + ival;
    pixel[3] = (ival > -16) ? -ival * 8 : 0x80;
  } else if (ival < 128) {
    pixel[0] = 0xFF - ival;
    pixel[1] = 0x80 - ival/4;
    pixel[2] = 0;
    pixel[3] = (ival < 16) ? ival * 8 : 0x80;
  } else {
    pixel[0] = 0x80;
    pixel[1] = 0x60;
    pixel[2] = 0;
    pixel[3] = 0x80;
  }
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
contour_pmsl(png_bytep *ovector, const double *gbuf,
  size_t owidth, size_t oheight)
{
  // 段階1: 全格子 pixel[0] に段彩番号を打つ
  for (size_t j = 0; j < oheight; j++) {
    for (size_t i = 0; i < owidth; i++) {
      png_bytep pixel = ovector[j] + i * 4;
      double val = gbuf[i + j * owidth];
      // Pmsl は 0.1 hPa 単位、4 hPa 単位で等値線を引く
      // 500...1500 hPa -> istep 0...250
      unsigned istep = ((unsigned)floor(val / 40.0) - 125u) % 250u;
      pixel[0] = pixel[1] = pixel[2] = istep;
      pixel[3] = 0xFF;
    }
  }
  // 段階2: 隣接格子が同値な場合透明化する
  for (size_t j = 1; j < (oheight-1); j++) {
    for (size_t i = 1; i < (owidth-1); i++) {
      png_bytep pixel = ovector[j] + i * 4;
      unsigned istep = pixel[0];
      // 隣接4格子が同値な場合（等圧線の間）を透明に抜く
      if ( ovector[j-1][(i  )*4] == istep
        && ovector[j  ][(i-1)*4] == istep
        && ovector[j  ][(i+1)*4] == istep
        && ovector[j+1][ i   *4] == istep) {
        pixel[3] = 0;
      }
      // istep 値を 5 で割った余りが 4 以外の場合
      // 隣接4格子が同値または1増な場合（等圧線の間）を透明に抜く
      if (4 != istep % 5) {
        if ((ovector[j-1][(i  )*4] == istep
          || ovector[j-1][(i  )*4] == istep+1)
          &&(ovector[j  ][(i-1)*4] == istep
          || ovector[j  ][(i-1)*4] == istep+1)
          &&(ovector[j  ][(i+1)*4] == istep
          || ovector[j  ][(i+1)*4] == istep+1)
          &&(ovector[j+1][ i   *4] == istep
          || ovector[j+1][ i   *4] == istep+1)){
          pixel[3] = 0;
        }
      }
    }
  }
  for (size_t j = 0; j < oheight; j++) {
    for (size_t i = 0; i < owidth; i++) {
      png_bytep pixel = ovector[j] + i * 4;
      if (pixel[3] == 0) {
        pixel[0] = pixel[1] = pixel[2] = 0;
      } else {
        setpixel_pmsl(pixel, gbuf[i + j * owidth]);
      }
    }
  }
  return 0;
}

  int
contour_z(png_bytep *ovector, const double *gbuf,
  size_t owidth, size_t oheight)
{
  // 最初の格子で決めてしまう
  double cycle = (gbuf[0] > 6360.0) ? 120.0 : 60.0;
  // 段階1: 全格子 pixel[0] に段彩番号を打つ
  for (size_t j = 0; j < oheight; j++) {
    for (size_t i = 0; i < owidth; i++) {
      png_bytep pixel = ovector[j] + i * 4;
      double val = gbuf[i + j * owidth];
      // Z は 1 m 単位、縞々透過は 60 m 単位 (6000 m 以上では 120 m 単位)
      // 念の為 250 の剰余にしているが、50 hPa 程度までなら、250 未満
      unsigned istep = (unsigned)floor(val / cycle) % 250; 
      pixel[0] = pixel[1] = pixel[2] = istep;
      pixel[3] = 0xFF;
    }
  }
  // 段階2: 隣接格子が同値な場合透明化する
  for (size_t j = 1; j < (oheight-1); j++) {
    for (size_t i = 1; i < (owidth-1); i++) {
      png_bytep pixel = ovector[j] + i * 4;
      unsigned istep = pixel[0];
      // 隣接4格子が同値な場合（等圧線の間）を透明に抜く
      if ( ovector[j-1][(i  )*4] == istep
        && ovector[j  ][(i-1)*4] == istep
        && ovector[j  ][(i+1)*4] == istep
        && ovector[j+1][ i   *4] == istep) {
        pixel[3] = 0;
      }
      // istep 値を 5 で割った余りが 4 以外の場合
      // 隣接4格子が同値または1増な場合（等圧線の間）を透明に抜く
      if (4 != istep % 5) {
        if ((ovector[j-1][(i  )*4] == istep
          || ovector[j-1][(i  )*4] == istep+1)
          &&(ovector[j  ][(i-1)*4] == istep
          || ovector[j  ][(i-1)*4] == istep+1)
          &&(ovector[j  ][(i+1)*4] == istep
          || ovector[j  ][(i+1)*4] == istep+1)
          &&(ovector[j+1][ i   *4] == istep
          || ovector[j+1][ i   *4] == istep+1)){
          pixel[3] = 0;
        }
      }
    }
  }
  for (size_t j = 0; j < oheight; j++) {
    for (size_t i = 0; i < owidth; i++) {
      png_bytep pixel = ovector[j] + i * 4;
      if (pixel[3] == 0) {
        pixel[0] = pixel[1] = pixel[2] = 0;
      } else {
        setpixel_z(pixel, gbuf[i + j * owidth]);
      }
    }
  }
  return 0;
}

  int
drawfront(png_bytep *ovector, const double *gbuf,
  size_t owidth, size_t oheight, palette_t pal)
{
  double *dgbuf; // |nabla_n gbuf|
  double *ddgbuf; // nabla_n |nabla_n gbuf|
  dgbuf = malloc(sizeof(double) * owidth * oheight * 2);
  ddgbuf = dgbuf + owidth * oheight;
  for (size_t j = 1; j < oheight - 1; j++) {
    for (size_t i = 1; i < owidth - 1; i++) {
      dgbuf[i + j*owidth] = 
        hypot(gbuf[i+1 + j*owidth] - gbuf[i-1 + j*owidth],
        gbuf[i + (j+1)*owidth] - gbuf[i + (j-1)*owidth]);
    }
  }
  for (size_t j = 1; j < oheight - 1; j++) {
    for (size_t i = 1; i < owidth - 1; i++) {
      double nx, ny;
      ddgbuf[i + j*owidth] = 
        gbuf[i+1+j*owidth] + gbuf[i-1+j*owidth] - gbuf[i+j*owidth]*2 +
        gbuf[i+(j+1)*owidth] + gbuf[i+(j-1)*owidth] - gbuf[i+j*owidth]*2;
    }
  }
  for (size_t j = 2; j < oheight - 2; j++) {
    for (size_t i = 2; i < owidth - 2; i++) {
      png_bytep pixel = ovector[j] + i * 4;
      if (
        (dgbuf[i+j*owidth] > 0.1) && (
        (ddgbuf[(i-1)+j*owidth] * ddgbuf[i+j*owidth] < 0.0) ||
        (ddgbuf[(i+1)+j*owidth] * ddgbuf[i+j*owidth] < 0.0) ||
        (ddgbuf[i+(j-1)*owidth] * ddgbuf[i+j*owidth] < 0.0) ||
        (ddgbuf[i+(j+1)*owidth] * ddgbuf[i+j*owidth] < 0.0)
        )
      ){
        pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
      }
    }
  }
  free(dgbuf);
  return 0;
}

  int
render(png_bytep *ovector, const double *gbuf,
  size_t owidth, size_t oheight, palette_t pal)
{
  int r = 0;
  switch (pal) {
  case PALETTE_Z:
    r = contour_z(ovector, gbuf, owidth, oheight);
    break;
  case PALETTE_Pmsl:
    r = contour_pmsl(ovector, gbuf, owidth, oheight);
    break;
  case PALETTE_RH:
    for (size_t j = 0; j < oheight; j++) {
      for (size_t i = 0; i < owidth; i++) {
        png_bytep pixel = ovector[j] + i * 4;
        setpixel_rh(pixel, gbuf[i + j * owidth]);
      }
    }
    break;
  case PALETTE_papT:
    for (size_t j = 0; j < oheight; j++) {
      for (size_t i = 0; i < owidth; i++) {
        png_bytep pixel = ovector[j] + i * 4;
        setpixel_papt(pixel, gbuf[i + j * owidth]);
      }
    }
    drawfront(ovector, gbuf, owidth, oheight, pal);
    break;
  case PALETTE_T:
    for (size_t j = 0; j < oheight; j++) {
      for (size_t i = 0; i < owidth; i++) {
        png_bytep pixel = ovector[j] + i * 4;
        setpixel_t(pixel, gbuf[i + j * owidth]);
      }
    }
    break;
  case PALETTE_WINDS_SFC:
    for (size_t j = 0; j < oheight; j++) {
      for (size_t i = 0; i < owidth; i++) {
        png_bytep pixel = ovector[j] + i * 4;
        setpixel_windsfc(pixel, gbuf[i + j * owidth]);
      }
    }
    break;
  case PALETTE_WINDS:
    for (size_t j = 0; j < oheight; j++) {
      for (size_t i = 0; i < owidth; i++) {
        png_bytep pixel = ovector[j] + i * 4;
        setpixel_winds(pixel, gbuf[i + j * owidth]);
      }
    }
    break;
  case PALETTE_RAIN6:
    for (size_t j = 0; j < oheight; j++) {
      for (size_t i = 0; i < owidth; i++) {
        png_bytep pixel = ovector[j] + i * 4;
        setpixel_rain6(pixel, gbuf[i + j * owidth]);
      }
    }
    break;
  case PALETTE_rVOR:
    for (size_t j = 0; j < oheight; j++) {
      for (size_t i = 0; i < owidth; i++) {
        png_bytep pixel = ovector[j] + i * 4;
        setpixel_rvor(pixel, gbuf[i + j * owidth]);
      }
    }
    break;
  case PALETTE_VVPa:
    for (size_t j = 0; j < oheight; j++) {
      for (size_t i = 0; i < owidth; i++) {
        png_bytep pixel = ovector[j] + i * 4;
        setpixel_rvor(pixel, gbuf[i + j * owidth] * -50.0);
      }
    }
    break;
  default:
    for (size_t j = 0; j < oheight; j++) {
      for (size_t i = 0; i < owidth; i++) {
        png_bytep pixel = ovector[j] + i * 4;
        setpixel_gsi(pixel, gbuf[i + j * owidth]);
      }
    }
    break;
  }
  return r;
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

