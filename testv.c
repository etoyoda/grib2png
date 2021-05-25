#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "visual.h"

int
main(int argc, const char **argv)
{
  const int SIZE = 256;
  double buf[SIZE][SIZE];
  for (int j = 0; j < SIZE; j++) {
    for (int i = 0; i < SIZE; i++) {
      buf[j][i] = 8192.0 / (1.0 + hypot(i - SIZE/2, j - SIZE/2) / 4);
    }
  }
  return gridsave((double *)buf, SIZE, SIZE, "z.png");
}
