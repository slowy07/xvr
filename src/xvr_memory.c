#include "xvr_memory.h"
#include "xvr_console_color.h"
#include <stdio.h>
#include <stdlib.h>

void *Xvr_reallocate(void *pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, newSize);

  if (result == NULL) {
    fprintf(stderr,
            XVR_CC_ERROR "[internal] error: memory allocation error (requested "
                         "%d, replacing %d)\n" XVR_CC_RESET,
            (int)newSize, (int)oldSize);
    exit(1);
  }

  return result;
}

void Xvr_initBucket(Xvr_Bucket **bucketHandle, size_t capacity) {
  if (capacity == 0) {
    fprintf(stderr, XVR_CC_ERROR "[internal] Error: cannot init a bucket with "
                                 "zero capacity\n" XVR_CC_RESET);
    exit(1);
  }

  (*bucketHandle) = malloc(sizeof(Xvr_Bucket));

  if ((*bucketHandle) == NULL) {
    fprintf(stderr, XVR_CC_ERROR "[internal] Error: failed to allocate space "
                                 "for a bucket\n" XVR_CC_RESET);
    exit(1);
  }

  (*bucketHandle)->next = NULL;
  (*bucketHandle)->contents = NULL;
  (*bucketHandle)->capacity = capacity;
  (*bucketHandle)->count = 0;
}

void *Xvr_partBucket(Xvr_Bucket **bucketHandle, size_t space) {
  if ((*bucketHandle) == NULL) {
    fprintf(stderr, XVR_CC_ERROR
            "[internal] Error: expected bucket, received NULL\n" XVR_CC_RESET);
    exit(1);
  }

  if ((*bucketHandle)->capacity < (*bucketHandle)->count + space) {
    Xvr_Bucket *temp = NULL;
    Xvr_initBucket(&temp, (*bucketHandle)->capacity);
    temp->next = (*bucketHandle);
    (*bucketHandle) = temp;
  }

  if ((*bucketHandle)->contents == NULL) {
    (*bucketHandle)->contents = malloc((*bucketHandle)->capacity);

    if ((*bucketHandle)->contents == NULL) {
      fprintf(stderr, XVR_CC_ERROR "[internal] Error: failed to allocating "
                                   "space for bucket content\n" XVR_CC_RESET);
      exit(1);
    }
  }

  (*bucketHandle)->count += space;
  return ((*bucketHandle)->contents + (*bucketHandle)->count - space);
}

void Xvr_freeBucket(Xvr_Bucket **bucketHandle) {
  while ((*bucketHandle) != NULL) {
    Xvr_Bucket *ptr = (*bucketHandle);
    (*bucketHandle) = (*bucketHandle)->next;
    free(ptr->contents);
    free(ptr);
  }
}
