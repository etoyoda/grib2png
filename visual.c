// visual.c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <png.h>
#include "visual.h"


// libpng に渡せる形の RGBA バッファを作る。 
  png_bytep *
new_pngimg(size_t owidth, size_t oheight)
{
  png_bytep *r;
  r = malloc(sizeof(png_bytep) * oheight + 1);
  if (r == NULL) { return NULL; }
  for (int j = 0; j < oheight; j++) {
    r[j] = calloc(owidth, sizeof(png_byte) * 4);
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
setpixel_pmsl(png_bytep pixel, double val)
{
  // val は 0.1 hPa 単位、4 hPa 単位で縞々透過をつける
  long istep = (long)(val / 40.0);
  unsigned frac = (unsigned)((val - istep * 40.0) / 40.0) * 0x100u;
  // 1013 hPa => istep=253 を中心に
  int red = (1013/4 - istep) * 4 + 0x80;
  int blue = (istep - 1013/4) * 4 + 0x80;
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

