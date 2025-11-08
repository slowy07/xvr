#include <stdio.h>
#include <stdlib.h>

#include "xvr_console_colors.h"
#include "xvr_memory.h"

static int callCount = 0;

void* allocator(void* pointer, size_t oldSize, size_t newSize) {
    callCount++;

    if (newSize == 0 && oldSize == 0) {
        return NULL;
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* mem = realloc(pointer, newSize);
    if (mem == NULL) {
        exit(-1);
    }

    return mem;
}

void testMemoryAllocation(void) {
    {
        int* integer = XVR_ALLOCATE(int, 1);
        XVR_FREE(int, integer);
    }

    {
        int* array = XVR_ALLOCATE(int, 10);
        array[1] = 30;
        XVR_FREE_ARRAY(int, array, 10);
    }
    {
        int* array1 = XVR_ALLOCATE(int, 10);
        int* array2 = XVR_ALLOCATE(int, 10);

        array1[1] = 42;
        array2[1] = 42;

        XVR_FREE_ARRAY(int, array1, 10);
        XVR_FREE_ARRAY(int, array2, 10);
    }
}

int main(void) {
    testMemoryAllocation();
    Xvr_setMemoryAllocator(allocator);
    testMemoryAllocation();

    if (callCount != 8) {
        fprintf(stderr,
                XVR_CC_ERROR
                "woilah cik, Unexpected call count for custom allocator, Was "
                "called %d times\n" XVR_CC_RESET,
                callCount);
        return -1;
    }

    printf(XVR_CC_NOTICE "TEST MEMORY: jalan loh ya cik\n" XVR_CC_RESET);
}
