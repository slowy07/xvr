/**
MIT License

Copyright (c) 2025 arfy slowy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "xvr_bucket.h"
#include "xvr_console_colors.h"

#include <stdio.h>
#include <stdlib.h>

// buckets of fun
Xvr_Bucket *Xvr_allocateBucket(unsigned int capacity) {
  if (capacity == 0) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Cannot allocate a 'Xvr_Bucket' with "
                                 "zero capacity\n" XVR_CC_RESET);
    exit(1);
  }

  Xvr_Bucket *bucket = malloc(sizeof(Xvr_Bucket) + capacity);

  if (bucket == NULL) {
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Failed to allocate a 'Xvr_Bucket' of %d "
                         "capacity\n" XVR_CC_RESET,
            (int)capacity);
    exit(1);
  }

  // initialize the bucket
  bucket->next = NULL;
  bucket->capacity = capacity;
  bucket->count = 0;

  return bucket;
}

void *Xvr_partitionBucket(Xvr_Bucket **bucketHandle, unsigned int amount) {
  if ((*bucketHandle) == NULL) {
    fprintf(stderr, XVR_CC_ERROR
            "ERROR: Expected a 'Xvr_Bucket', received NULL\n" XVR_CC_RESET);
    exit(1);
  }

  if (amount % 4 != 0) {
    amount += 4 - (amount % 4);
  }

  // if you try to allocate too much space
  if ((*bucketHandle)->capacity < amount) {
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Failed to partition a 'Xvr_Bucket': requested "
                         "%d from a bucket of %d capacity\n" XVR_CC_RESET,
            (int)amount, (int)((*bucketHandle)->capacity));
    exit(1);
  }

  if ((*bucketHandle)->capacity < (*bucketHandle)->count + amount) {
    Xvr_Bucket *tmp = Xvr_allocateBucket((*bucketHandle)->capacity);
    tmp->next = (*bucketHandle);
    (*bucketHandle) = tmp;
  }

  (*bucketHandle)->count += amount;
  return ((*bucketHandle)->data + (*bucketHandle)->count - amount);
}

void Xvr_freeBucket(Xvr_Bucket **bucketHandle) {
  Xvr_Bucket *iter = (*bucketHandle);

  while (iter != NULL) {
    Xvr_Bucket *last = iter;
    iter = iter->next;

    free(last);
  }
  (*bucketHandle) = NULL;
}
