#include "../../src/xvr_array.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  unsigned int iterations = atoi(argv[1]);

  Xvr_Array *array = XVR_ARRAY_ALLOCATE();

  printf("found %d iteration\n", iterations);

  for (int i = 0; i < iterations; i++) {
    XVR_ARRAY_PUSHBACK(array, XVR_VALUE_FROM_INTEGER(i));
  }

  XVR_ARRAY_FREE(array);
  return 0;
}
