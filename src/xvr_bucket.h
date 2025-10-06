#ifndef XVR_BUCKET_H
#define XVR_BUCKET_H

#include "xvr_common.h"

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

#ifndef XVR_BUCKET_TINY
#define XVR_BUCKET_TINY (1024 * 2)
#endif // !XVR_BUCKET_TINY

#ifndef XVR_BUCKET_SMALL
#define XVR_BUCKET_SMALL (1024 * 4)
#endif // !XVR_BUCKET_SMALL

#ifndef XVR_BUCKET_MEDIUM
#define XVR_BUCKET_MEDIUM (1024 * 8)
#endif // !XVR_BUCKET_MEDIUM

#ifndef XVR_BUCKET_LARGE
#define XVR_BUCKET_LARGE (1024 * 16)
#endif // !XVR_BUCKET_LARGE

#ifndef XVR_BUCKET_HUGE
#define XVR_BUCKET_HUGE (1024 * 32)
#endif // !XVR_BUCKET_HUGE

#ifndef XVR_BUCKET_IDEAL
#define XVR_BUCKET_IDEAL (XVR_BUCKET_HUGE - sizeof(Xvr_Bucket))
#endif // !XVR_BUCKET_IDEAL

#endif // !XVR_BUCKET_H
