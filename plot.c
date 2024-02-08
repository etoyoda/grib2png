// plot.c
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "plot.h"

static int is_pen_up = 1;
static float cx = 0.0;
static float cy = 0.0;
static FILE *psout = NULL;
static float clinewidth = 1;
static float cred = 0.0;
static float cgreen = 0.0;
static float cblue = 0.0;
static char cname[256] = "/tmp/plotXXXXXX";

// openpl() - API to initialize the plot library;
//  can be called multiple times safely;
//  CAUTION: cannot be called after closepl();
  int
openpl(void)
{
  int fd;
  if (psout) return 1;
  fd = mkstemp(cname);
  psout = fdopen(fd, "w+t");
  return 0;
}

// newpath() - internal function to issue "newpath"
  int
newpath(void)
{
  openpl();
  fprintf(psout, "newpath\n");
  is_pen_up = 0;
  return 0;
}

  int
closepath(void)
{
  openpl();
  fprintf(psout, "%g setlinewidth %.3f %.3f %.3f setrgbcolor stroke\n",
    clinewidth, cred, cgreen, cblue);
  is_pen_up = 1;
  return 0;
}

static float cfontsize = 28.0;
static int cfontset = 0;

  int
setfont(void)
{
  if (cfontset) return 1;
  fprintf(psout, "/Courier findfont %g scalefont setfont\n", cfontsize);
  cfontset = 1;
  return 0;
}

  int
setfontsize(float sz)
{
  if (cfontset && (cfontsize != sz)) cfontset = 0;
  cfontsize = sz;
  return setfont();
}

#define CLOSEPATH_IF_NEEDED (is_pen_up ? -1 : closepath())
#define NEWPATH_IF_NEEDED (is_pen_up ? newpath() : -1)

  int
setlinewidth(float x)
{
  CLOSEPATH_IF_NEEDED;
  clinewidth = x;
  return x;
}

  int
setrgb(unsigned char r, unsigned char g, unsigned char b)
{
  CLOSEPATH_IF_NEEDED;
  cred = (float)r / 0xFF;
  cgreen = (float)g / 0xFF;
  cblue = (float)b / 0xFF;
  return 0;
}

  int
closepl(void)
{
  char cmd[512];
  int r;
  CLOSEPATH_IF_NEEDED;
  fprintf(psout, "showpage\n");
  fclose(psout);
  sprintf(cmd, "gs -q -sDEVICE=pngalpha -r72 -g1024x1024 -dBATCH -dNOPAUSE -sOutputFile=plot.png %s\n", cname);
  r = system(cmd);
  if ((r != 0) || (getenv("PLOT"))) {
    fputs(cmd, stderr);
    fprintf(stderr, "PostScript %s remains\n", cname);
    return -1;
  }
  remove(cname);
  return r;
}

  int
moveto(float x, float y)
{
  CLOSEPATH_IF_NEEDED;
  NEWPATH_IF_NEEDED;
  fprintf(psout, "%.1f %.1f moveto\n", x, y);
  cx = x;
  cy = y;
  return 0;
}

  int
lineto(float x, float y)
{
  openpl();
  fprintf(psout, "%.1f %.1f lineto\n", x, y);
  cx = x;
  cy = y;
  return 0;
}

  int
box(float x0, float y0, float x1, float y1)
{
  CLOSEPATH_IF_NEEDED;
  NEWPATH_IF_NEEDED;
  fprintf(psout, "%.1f %.1f moveto\n", x0, y0);
  fprintf(psout, "%.1f %.1f lineto\n", x0, y1);
  fprintf(psout, "%.1f %.1f lineto\n", x1, y1);
  fprintf(psout, "%.1f %.1f lineto\n", x1, y0);
  fprintf(psout, "closepath %g setlinewidth %.3f %.3f %.3f setrgbcolor fill\n",
    clinewidth, cred, cgreen, cblue);
  is_pen_up = 1;
  return 0;
}

  int
symbol(const char *text)
{
  openpl();
  setfont();
  fputs("(", psout);
  for (const char *p = text; *p; p++) {
    if (isspace(*p)) {
      fputc(' ', psout);
    } else if (*p == '(') {
      fputs("\\(", psout);
    } else if (*p == ')') {
      fputs("\\)", psout);
    } else if (isgraph(*p)) {
      fputc(*p, psout);
    } else {
      fputc('#', psout);
    }
  }
  fputs(") show\n", psout);
  return 0;
}

#ifdef TESTMAIN
  int
main()
{
  openpl();
  moveto(1, 1);
  lineto(1, 1023);
  lineto(1023, 1023);
  lineto(1023, 1);
  lineto(1, 1);
  setlinewidth(2.0f);
  moveto(100, 200);
  lineto(200, 250);
  lineto(100, 300);
  moveto(200, 250);
  symbol("Edge 1");
  moveto(100, 300);
  symbol("(100,300)");
  closepl();
}
#endif
