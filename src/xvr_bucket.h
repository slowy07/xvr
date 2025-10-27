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

#ifndef XVR_BUCKET_H
#define XVR_BUCKET_H

#include "xvr_common.h"

typedef struct Xvr_Bucket {   // 32 | 64 BITNESS
    struct Xvr_Bucket* next;  // 4  | 8
    unsigned int capacity;    // 4  | 4
    unsigned int count;       // 4  | 4
    unsigned char data[];     //-  | -
} Xvr_Bucket;                 // 12 | 16

XVR_API Xvr_Bucket* Xvr_allocateBucket(unsigned int capacity);
XVR_API unsigned char* Xvr_partitionBucket(Xvr_Bucket** bucketHandle,
                                  unsigned int amount);
XVR_API void Xvr_freeBucket(Xvr_Bucket** bucketHandle);

#ifndef XVR_BUCKET_1KB
#    define XVR_BUCKET_1KB (1 << 10)
#endif  // !XVR_BUCKET_1KB

#ifndef XVR_BUCKET_2KB
#    define XVR_BUCKET_2KB (1 << 11)
#endif  // !XVR_BUCKET_2KB

#ifndef XVR_BUCKET_4KB
#    define XVR_BUCKET_4KB (1 << 12)
#endif  // !XVR_BUCKET_4KB

#ifndef XVR_BUCKET_8KB
#    define XVR_BUCKET_8KB (1 << 13)
#endif  // !XVR_BUCKET_8KB

#ifndef XVR_BUCKET_16KB
#    define XVR_BUCKET_16KB (1 << 14)
#endif  // !XVR_BUCKET_16KB

#ifndef XVR_BUCKET_32KB
#    define XVR_BUCKET_32KB (1 << 15)
#endif  // !XVR_BUCKET_32KB

#ifndef XVR_BUCKET_64KB
#    define XVR_BUCKET_64KB (1 << 16)
#endif  // !XVR_BUCKET_64KB

#ifndef XVR_BUCKET_IDEAL
#    define XVR_BUCKET_IDEAL (XVR_BUCKET_64KB - sizeof(Xvr_Bucket))
#endif  // !XVR_BUCKET_IDEAL

#endif  // !XVR_BUCKET_H
