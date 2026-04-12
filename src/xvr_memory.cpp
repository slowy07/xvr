#include "xvr_memory.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_console_colors.h"
#include "xvr_refstring.h"

#if XVR_DEBUG_ALLOCATIONS
static size_t g_alloc_count = 0;
static size_t g_free_count = 0;
static size_t g_current_memory = 0;
static size_t g_peak_memory = 0;
static int g_initialized = 0;

void Xvr_debugPrintMemoryStats(void) {
    fprintf(stderr,
            "%s[Memory] Allocations: %zu, Frees: %zu, "
            "Current: %zu bytes, Peak: %zu bytes\n%s",
            XVR_CC_NOTICE, g_alloc_count, g_free_count, g_current_memory,
            g_peak_memory, XVR_CC_RESET);
}

size_t Xvr_debugGetAllocCount(void) { return g_alloc_count; }
size_t Xvr_debugGetFreeCount(void) { return g_free_count; }
size_t Xvr_debugGetCurrentMemory(void) { return g_current_memory; }

void Xvr_debugResetMemoryStats(void) {
    g_alloc_count = 0;
    g_free_count = 0;
    g_current_memory = 0;
    g_peak_memory = 0;
    g_initialized = 1;
}
#endif

extern "C" {

void* Xvr_private_defaultMemoryAllocator(void* pointer, size_t oldSize,
                                         size_t newSize) {
    /* NOTE: This allocator uses standard malloc/free - consider arena
       allocation for performance-critical code paths to reduce allocation
       overhead */
#if XVR_DEBUG_ALLOCATIONS
    if (!g_initialized) {
        g_initialized = 1;
    }
#endif

    if (newSize == 0 && oldSize == 0) {
        return NULL;
    }

    if (newSize == 0) {
#if XVR_DEBUG_ALLOCATIONS
        g_free_count++;
        g_current_memory -= oldSize;
#endif
        free(pointer);

        return NULL;
    }

    void* mem = realloc(pointer, newSize);

    if (mem == NULL) {
        fprintf(stderr,
                "%s[internal] Memory allocation error (requested %d, replacing "
                "%d)\n%s",
                XVR_CC_ERROR, (int)newSize, (int)oldSize, XVR_CC_RESET);
        exit(-1);
    }

#if XVR_DEBUG_ALLOCATIONS
    if (pointer == NULL) {
        g_alloc_count++;
        g_current_memory += newSize;
    } else {
        g_current_memory += newSize - oldSize;
    }
    if (g_current_memory > g_peak_memory) {
        g_peak_memory = g_current_memory;
    }
#endif

    return mem;
}

static Xvr_MemoryAllocatorFn allocator = Xvr_private_defaultMemoryAllocator;

void* Xvr_reallocate(void* pointer, size_t oldSize, size_t newSize) {
    return allocator(pointer, oldSize, newSize);
}

void Xvr_setMemoryAllocator(Xvr_MemoryAllocatorFn fn) {
    if (fn == NULL) {
        fprintf(stderr,
                "%s[internal] Memory allocator error (can't be null)\n%s",
                XVR_CC_ERROR, XVR_CC_RESET);
        exit(-1);
    }

    if (fn == Xvr_reallocate) {
        fprintf(stderr,
                "%s[internal] Memory allocator error (can't loop the "
                "Xvr_reallocate function)\n%s",
                XVR_CC_ERROR, XVR_CC_RESET);
        exit(-1);
    }

    allocator = fn;
    Xvr_setRefStringAllocatorFn(fn);
}

}  // extern "C"
