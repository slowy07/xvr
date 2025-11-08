#include "xvr_memory.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "xvr_console_colors.h"
#include "xvr_refstring.h"

void* Xvr_private_defaultMemoryAllocator(void* pointer, size_t oldSize,
                                         size_t newSize) {
    if (newSize == 0 && oldSize == 0) {
        return NULL;
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* mem = realloc(pointer, newSize);

    if (mem == NULL) {
        fprintf(stderr,
                XVR_CC_ERROR
                "[internal] Memory allocation error (requested %d, replacing "
                "%d)\n" XVR_CC_RESET,
                (int)newSize, (int)oldSize);
        exit(-1);
    }
    return mem;
}

static Xvr_MemoryAllocatorFn allocator = Xvr_private_defaultMemoryAllocator;

void* Xvr_reallocate(void* pointer, size_t oldSize, size_t newSize) {
    return allocator(pointer, oldSize, newSize);
}

void Xvr_setMemoryAllocator(Xvr_MemoryAllocatorFn fn) {
    if (fn == NULL) {
        fprintf(
            stderr, XVR_CC_ERROR
            "[internal] memory allocator error (can't be null)\n" XVR_CC_RESET);
        exit(-1);
    }

    if (fn == Xvr_reallocate) {
        fprintf(stderr, XVR_CC_ERROR
                "[internal] memory allocator error (can't loop the "
                "Xvr_reallocate function)\n" XVR_CC_RESET);
        exit(-1);
    }

    allocator = fn;
    Xvr_setRefStringAllocatorFn(fn);
}
