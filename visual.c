// visual.c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <png.h>
#include "visual.h"


// libpng に渡せる形の RGBA バッファを作る。 
  png_bytep *
new_pngimg(size_t owidth, size_t oheight)
{
  png_bytep *r;
  r = malloc(sizeof(png_bytep) * oheight);
  if (r == NULL) { return NULL; }
  r[0] = calloc(owidth * oheight, sizeof(png_byte) * 4);
  if (r[0] == NULL) {
    free(r);
    return NULL;
  }
  for (int j = 1; j < oheight; j++) {
    r[j] = r[0] + owidth * 4;
  }
  return r;
}

// RGBA バッファ ovector (寸法 owidth * oheight) をファイル filename に出力
  int
write_pngimg(png_bytep *ovector, size_t owidth, size_t oheight,
  const char *filename)
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
  // テキストヘッダ記入 (TBD)
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
  free(ovector[0]);
  free(ovector);
}

  int
render(png_bytep *ovector, const double *gbuf,
  size_t owidth, size_t oheight)
{
  for (size_t j = 0; j < oheight; j++) {
    for (size_t i = 0; i < owidth; i++) {
      size_t ij = i + j * owidth;
      unsigned long ival = gbuf[ij] * 256;
      png_bytep pixel = ovector[j] + i * 4;
      pixel[0] = (ival << 16) & 0xFF;
      pixel[1] = (ival <<  8) & 0xFF;
      pixel[2] =  ival        & 0xFF;
      pixel[3] = 0xFF;
    }
  }
  return 0;
}


  int
gridsave(double *gbuf, size_t owidth, size_t oheight,
  const char *filename)
{
  png_bytep *ovector;
  int r = 0;
  // 1. RGBイメージメモリ確保
  ovector = new_pngimg(owidth, oheight);
  if (ovector == NULL) return 'M';
  // 2. データ変換（double値→RGBA）
  r = render(ovector, gbuf, owidth, oheight);
  if (r != 0) goto badend;
  // 3. PNG書き出し
  r = write_pngimg(ovector, owidth, oheight, filename);
badend:
  // 4. メモリ開放
  del_pngimg(ovector);
  return r;
}

