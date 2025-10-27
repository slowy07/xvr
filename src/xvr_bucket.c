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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "xvr_console_colors.h"

// buckets of fun
Xvr_Bucket* Xvr_allocateBucket(unsigned int capacity) {
    assert(capacity != 0 &&
           "Cannot allocate a 'Xvr_Bucket' with zero capacity");

    Xvr_Bucket* bucket = malloc(sizeof(Xvr_Bucket) + capacity);

    if (bucket == NULL) {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Failed to allocate a 'Xvr_Bucket' of %d "
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

void* Xvr_partitionBucket(Xvr_Bucket** bucketHandle, unsigned int amount) {
    amount = (amount + 3) & ~3;

    assert((*bucketHandle) != NULL && "Expected a 'Xvr_Bucket', received NULL");
    assert((*bucketHandle)->capacity >= amount &&
           "ERROR: Failed to partition a 'Xvr_Bucket', requested amount is too "
           "high");

    if ((*bucketHandle)->capacity < (*bucketHandle)->count + amount) {
        Xvr_Bucket* tmp = Xvr_allocateBucket((*bucketHandle)->capacity);
        tmp->next = (*bucketHandle);
        (*bucketHandle) = tmp;
    }

    (*bucketHandle)->count += amount;
    return ((*bucketHandle)->data + (*bucketHandle)->count - amount);
}

void Xvr_freeBucket(Xvr_Bucket** bucketHandle) {
    Xvr_Bucket* iter = (*bucketHandle);

    while (iter != NULL) {
        // run down the chain
        Xvr_Bucket* last = iter;
        iter = iter->next;

        // clear the previous bucket from memory
        free(last);
    }

    // for safety
    (*bucketHandle) = NULL;
}
