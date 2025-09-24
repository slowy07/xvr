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
**/

#ifndef XVR_MEMORY_H
#define XVR_MEMORY_H

#include "xvr_common.h"
#include <stddef.h>

#define XVR_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define XVR_ALLOCATE(type, count)                                              \
  (type *)Xvr_reallocate(NULL, 0, sizeof(type) * (count))

#define XVR_FREE(type, pointer) (type *)Xvr_reallocate(pointer, sizeof(type), 0)

#define XVR_GROW_ARRAY(type, pointer, oldSize, newSize)                        \
  (type *)Xvr_reallocate(pointer, sizeof(type) * oldSize,                      \
                         sizeof(type) * newSize)

#define XVR_SHRINK_ARRAY(type, pointer, oldCount, count)                       \
  (type *)Xvr_reallocate((type *)pointer, sizeof(type) * (oldCount),           \
                         sizeof(type) * (count))

#define XVR_FREE_ARRAY(type, pointer, oldSize)                                 \
  (type *)Xvr_reallocate(pointer, sizeof(type) * oldSize, 0)

XVR_API void *Xvr_reallocate(void *pointer, size_t oldSize, size_t newSize);

#define XVR_BUCKET_INIT(type, bucket, count) Xvr_initBucket(&(bucket), sizeof(type)*(count))
#define XVR_BUCKET_PART(type, bucket) (type*)Xvr_partBucket(&(bucket), sizeof(type))
#define XVR_BUCKET_FREE(bucket) Xvr_freeBucket(&(bucket))

typedef struct Xvr_Bucket {
  struct Xvr_Bucket *next;
  void *contents;
  int capacity;
  int count;
} Xvr_Bucket;

XVR_API void Xvr_initBucket(Xvr_Bucket **bucketHandle, size_t capacity);
XVR_API void *Xvr_partBucket(Xvr_Bucket **bucketHandle, size_t space);
XVR_API void Xvr_freeBucket(Xvr_Bucket **bucketHandle);

#endif // !XVR_MEMORY_H
