#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "gribscan.h"

  double
syntaxsugar(const char *sptr, const char **sptrptr)
{
  const char *eptr;
  eptr = strchr(sptr, ']');
  if (eptr == NULL) { goto BARF; }
  *sptrptr = eptr + 1;
  if (strncmp(sptr, "T", eptr-sptr)==0) { return IPARM_T; }
  if (strncmp(sptr, "papT", eptr-sptr)==0) { return IPARM_papT; }
  if (strncmp(sptr, "dT", eptr-sptr)==0) { return IPARM_dT; }
  if (strncmp(sptr, "RH", eptr-sptr)==0) { return IPARM_RH; }
  if (strncmp(sptr, "RR1H", eptr-sptr)==0) { return IPARM_RR1H; }
  if (strncmp(sptr, "RAIN", eptr-sptr)==0) { return IPARM_RAIN; }
  if (strncmp(sptr, "WINDS", eptr-sptr)==0) { return IPARM_WINDS; }
  if (strncmp(sptr, "U", eptr-sptr)==0) { return IPARM_U; }
  if (strncmp(sptr, "V", eptr-sptr)==0) { return IPARM_V; }
  if (strncmp(sptr, "PSI", eptr-sptr)==0) { return IPARM_PSI; }
  if (strncmp(sptr, "CHI", eptr-sptr)==0) { return IPARM_CHI; }
  if (strncmp(sptr, "VVPa", eptr-sptr)==0) { return IPARM_VVPa; }
  if (strncmp(sptr, "rVOR", eptr-sptr)==0) { return IPARM_rVOR; }
  if (strncmp(sptr, "rDIV", eptr-sptr)==0) { return IPARM_rDIV; }
  if (strncmp(sptr, "Pres", eptr-sptr)==0) { return IPARM_Pres; }
  if (strncmp(sptr, "Pmsl", eptr-sptr)==0) { return IPARM_Pmsl; }
  if (strncmp(sptr, "Z", eptr-sptr)==0) { return IPARM_Z; }
BARF:
  return strtod("NaN", NULL);
}

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
  tos[0] = 1.0;
  tos[1] = 1.0;
#define PUSH { if (tos >= stack + GSF_STACKSIZE) { goto OVERFLOW; } else { tos++; } }
  while (!!(c = *(sptr++))) {
    switch (c) {
    // === CONSTANTS AND VARIABLES ===
    // double precision
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      PUSH
      *tos = strtod(sptr-1, (char **)&sptr);
      break;
    // hexadecimal integer
    case 'x':
      PUSH
      *tos = (double)strtoul(sptr, (char **)&sptr, 16);
      break;
    // syntax sugar
    case '[':
      PUSH
      *tos = syntaxsugar(sptr, &sptr);
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
    case 'g': // f for (end of) forecast time
      PUSH
      *tos = ftime + dura;
      break;
    case 'v': // vertical level
      PUSH
      *tos = vlev;
      break;
    // === BINARY OPERATORS ===
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
      dbuf = (tos[-1] < tos[0]);
      POP
      *tos = dbuf;
      break;
    case '>':
    case 'G':
      dbuf = (tos[-1] > tos[0]);
      POP
      *tos = dbuf;
      break;
    // === OTHER OPERATOR ===
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
