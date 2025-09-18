#include "../../src/xvr_console_color.h"
#include "../../src/xvr_memory.h"
#include <stdio.h>

int test_reallocate() {
  {
    int *integer = XVR_ALLOCATE(int, 1);
    XVR_FREE(int, integer);
  }

  {
    int *array = XVR_ALLOCATE(int, 10);
    array[1] = 42;
    XVR_FREE_ARRAY(int, array, 10);
  }

  {
    int *array1 = XVR_ALLOCATE(int, 10);
    int *array2 = XVR_ALLOCATE(int, 10);

    array1[1] = 42;
    array2[1] = 42;

    XVR_FREE_ARRAY(int, array1, 10);
    XVR_FREE_ARRAY(int, array2, 10);
  }

  return 0;
}

int test_bucket() {
  {
    Xvr_Bucket *bucket = NULL;
    XVR_BUCKET_INIT(int, bucket, 32);

    if (bucket == NULL || bucket->capacity != 32 * sizeof(int)) {
      fprintf(stderr, XVR_CC_ERROR
              "ERROR: failed to initialize `Xvr_Bucket`\n" XVR_CC_RESET);
      return -1;
    }

    XVR_BUCKET_FREE(bucket);
  }

  return 0;
}

int main() {
  int total = 0, res = 0;

  res = test_reallocate();
  total += res;

  if (res == 0) {
    printf(XVR_CC_NOTICE "everything nice one gass\n" XVR_CC_RESET);
  }

  return total;
}
