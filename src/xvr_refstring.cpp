#include "xvr_refstring.h"

#include <cstring>
#include <cstdio>

#include "xvr_memory.h"
#include "xvr_string_utils.h"

#if defined(XVR_DEBUG) || defined(DEBUG)
static int g_refstring_count = 0;
#endif

static Xvr_RefStringAllocatorFn allocate = [](void* pointer, size_t oldSize, size_t newSize) -> void* {
    return Xvr_reallocate(pointer, oldSize, newSize);
};

extern "C" {

void Xvr_setRefStringAllocatorFn(Xvr_RefStringAllocatorFn allocator) {
    allocate = allocator;
}

Xvr_RefString* Xvr_createRefString(const char* cstring) {
    size_t length = xvr_safe_strlen(cstring, 4096);
    return Xvr_createRefStringLength(cstring, length);
}

Xvr_RefString* Xvr_createRefStringLength(const char* cstring, size_t length) {
    size_t totalSize = sizeof(size_t) + sizeof(int) + sizeof(char) * (length + 1);
    Xvr_RefString* refString = static_cast<Xvr_RefString*>(allocate(NULL, 0, totalSize));

    if (refString == NULL) {
        return NULL;
    }

    refString->refCount = 1;
    refString->length = length;
    
    if (cstring && length > 0) {
        std::memcpy(refString->data, cstring, length);
    }
    refString->data[length] = '\0';

#if defined(XVR_DEBUG) || defined(DEBUG)
    g_refstring_count++;
#endif

    return refString;
}

void Xvr_deleteRefString(Xvr_RefString* refString) {
    if (!refString) return;
    
    refString->refCount--;
    if (refString->refCount <= 0) {
#if defined(XVR_DEBUG) || defined(DEBUG)
        g_refstring_count--;
#endif
        size_t totalSize = sizeof(size_t) + sizeof(int) + sizeof(char) * (refString->length + 1);
        allocate(refString, totalSize, 0);
    }
}

int Xvr_countRefString(Xvr_RefString* refString) { 
    if (!refString) return 0;
    return refString->refCount; 
}

size_t Xvr_lengthRefString(Xvr_RefString* refString) {
    if (!refString) return 0;
    return refString->length;
}

Xvr_RefString* Xvr_copyRefString(Xvr_RefString* refString) {
    if (!refString) return NULL;
    refString->refCount++;
    return refString;
}

Xvr_RefString* Xvr_deepCopyRefString(Xvr_RefString* refString) {
    if (!refString) return NULL;
    return Xvr_createRefStringLength(refString->data, refString->length);
}

const char* Xvr_toCString(Xvr_RefString* refString) { 
    if (!refString) return "";
    return refString->data; 
}

bool Xvr_equalsRefString(Xvr_RefString* lhs, Xvr_RefString* rhs) {
    if (lhs == rhs) {
        return true;
    }
    if (!lhs || !rhs) {
        return false;
    }
    if (lhs->length != rhs->length) {
        return false;
    }
    return std::strncmp(lhs->data, rhs->data, lhs->length) == 0;
}

bool Xvr_equalsRefStringCString(Xvr_RefString* lhs, char* cstring) {
    if (!lhs || !cstring) return false;
    
    size_t length = xvr_safe_strlen(cstring, 4096);
    if (lhs->length != length) {
        return false;
    }
    return std::strncmp(lhs->data, cstring, lhs->length) == 0;
}

#if defined(XVR_DEBUG) || defined(DEBUG)
void Xvr_debugPrintRefStringStats(void) {
    fprintf(stderr, "[RefString] Active strings: %d\n", g_refstring_count);
}

int Xvr_debugGetRefStringCount(void) { return g_refstring_count; }

void Xvr_debugResetRefStringStats(void) { g_refstring_count = 0; }
#endif

}
