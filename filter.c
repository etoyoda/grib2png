#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gribscan.h"

#define streq(a, b) (strcmp((a),(b))==0)

#define GSF_STACKSIZE 1024

  extern gribscan_err_t
gribscan_filter(const char *sfilter,
  iparm_t param,
  long ftime,
  long dura,
  double vlev)
{
  const char *sptr = sfilter;
  char c;
  double dbuf;
  double stack[GSF_STACKSIZE];
  // のりしろ: 置数しないで二項演算子を発行しても死なないように2つ積む
  double *tos = stack + 1;
  tos[0] = 0.0;
  tos[1] = 0.0;
#define PUSH { if (tos >= stack + GSF_STACKSIZE) { goto OVERFLOW; } }
  while (!!(c = *(sptr++))) {
    switch (c) {
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      PUSH
      *tos = strtod(sptr, (char **)&sptr);
      break;
    case 'p': // p for parameter
      PUSH
      *tos = param;
      break;
    case 'f': // f for forecast time
      PUSH
      *tos = ftime;
      break;
    case 'd': // d for duration
      PUSH
      *tos = dura;
      break;
    case 'F': // f for (end of) forecast time
      PUSH
      *tos = ftime + dura;
      break;
    case '&':
    case 'A':
      dbuf = (tos[0] != 0.0) && (tos[-1] != 0.0);
#define POP {if (tos > (stack + 1)) { tos--; }}
      POP
      *tos = dbuf;
      break;
    case '|':
    case 'V':
      dbuf = (tos[0] != 0.0) || (tos[-1] != 0.0);
      POP
      *tos = dbuf;
      break;
    case '=':
    case 'E':
      dbuf = (tos[0] == tos[-1]);
      POP
      *tos = dbuf;
      break;
    case '<':
    case 'L':
      dbuf = (tos[0] < tos[-1]);
      POP
      *tos = dbuf;
      break;
    case '>':
    case 'G':
      dbuf = (tos[0] > tos[-1]);
      POP
      *tos = dbuf;
      break;
    case ':':
      PUSH
      tos[0] = tos[-1];
      break;
#if 0
    case '/':
      break;
    case '_':
      break;
#endif
    case ',': // NO-OP
      break;
    default:
      fprintf(stderr, "<%c>", c);
      break;
    }
  }
  if (*tos != 0.0) {
    return GSE_OKAY;
  } else {
    return GSE_SKIP;
  }
OVERFLOW:
  return ERR_FSTACK;
}
