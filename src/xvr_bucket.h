#ifndef XVR_BUCKET_H
#define XVR_BUCKET_H

#include "xvr_common.h"

#define XVR_BUCKET_SMALL 256
#define XVR_BUCKET_MEDIUM 512
#define XVR_BUCKET_LARGE 1024

#define XVR_BUCKET_IDEAL 1024

typedef struct Xvr_Bucket { // 32 | 64 BITNESS
  struct Xvr_Bucket *next;  // 4  | 8
  unsigned int capacity;    // 4  | 4
  unsigned int count;       // 4  | 4
  char data[];              //-  | -
} Xvr_Bucket;               // 12 | 16

XVR_API Xvr_Bucket *Xvr_allocateBucket(unsigned int capacity);
XVR_API void *Xvr_partitionBucket(Xvr_Bucket **bucketHandle,
                                  unsigned int amount);
XVR_API void Xvr_freeBucket(Xvr_Bucket **bucketHandle);

#endif // !XVR_BUCKET_H
