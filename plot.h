extern int openpl();
extern int closepl();
extern int setrgb(unsigned char r, unsigned char g, unsigned char b);
extern int setlinewidth(float x);
extern int setfontsize(float x);
extern int moveto(float x, float y);
extern int lineto(float x, float y);
extern int symbol(const char *text);
extern int box(float x0, float y0, float x1, float y1);
