#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include <stdio.h>

int test_buckets() {
  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(sizeof(int) * 32);
    if (bucket == NULL || bucket->capacity != 32 * sizeof(int)) {
      fprintf(stderr, XVR_CC_ERROR
              "Error: failed to initializate `xvr_bucket`\n" XVR_CC_RESET);
      return -1;
    }
    Xvr_freeBucket(&bucket);
  }

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(sizeof(int) * 32);

    int *a = Xvr_partitionBucket(&bucket, sizeof(int));
    int *b = Xvr_partitionBucket(&bucket, sizeof(int));
    int *c = Xvr_partitionBucket(&bucket, sizeof(int));
    int *d = Xvr_partitionBucket(&bucket, sizeof(int));

    if (bucket == NULL || bucket->count != 4 * sizeof(int)) {
      fprintf(stderr,
              XVR_CC_ERROR "Error: failed to partition `Xvr_Bucket` corretly: "
                           "count is %d, expected %d\n" XVR_CC_RESET,
              (int)(bucket->count), (int)(4 * sizeof(int)));
      return -1;
    }
    Xvr_freeBucket(&bucket);
  }

  return 0;
}

int main() {
  printf(XVR_CC_WARN "testing: xvr bucket\n" XVR_CC_RESET);

  int total = 0, res = 0;

  {
    res = test_buckets();
    total += res;

    if (res == 0) {
      printf(XVR_CC_NOTICE
             "test_buckets(): nice one cik aman loh ya\n" XVR_CC_RESET);
    }
  }

  return total;
}
