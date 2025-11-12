#include "xvr_refstring.h"

#include <string.h>

extern void* Xvr_private_defaultMemoryAllocator(void* pointer, size_t oldSize,
                                                size_t newSize);
static Xvr_RefStringAllocatorFn allocate = Xvr_private_defaultMemoryAllocator;

void Xvr_setRefStringAllocatorFn(Xvr_RefStringAllocatorFn allocator) {
    allocate = allocator;
}

Xvr_RefString* Xvr_createRefString(const char* cstring) {
    size_t length = strlen(cstring);

    return Xvr_createRefStringLength(cstring, length);
}

Xvr_RefString* Xvr_createRefStringLength(const char* cstring, size_t length) {
    Xvr_RefString* refString = allocate(
        NULL, 0, sizeof(size_t) + sizeof(int) + sizeof(char) * (length + 1));

    if (refString == NULL) {
        return NULL;
    }

    refString->refCount = 1;
    refString->length = length;
    strncpy(refString->data, cstring, refString->length);

    refString->data[refString->length] = '\0';

    return refString;
}

void Xvr_deleteRefString(Xvr_RefString* refString) {
    refString->refCount--;
    if (refString->refCount <= 0) {
        allocate(refString,
                 sizeof(size_t) + sizeof(int) +
                     sizeof(char) * (refString->length + 1),
                 0);
    }
}

int Xvr_countRefString(Xvr_RefString* refString) { return refString->refCount; }

size_t Xvr_lengthRefString(Xvr_RefString* refString) {
    return refString->length;
}

Xvr_RefString* Xvr_copyRefString(Xvr_RefString* refString) {
    refString->refCount++;
    return refString;
}

Xvr_RefString* Xvr_deepCopyRefString(Xvr_RefString* refString) {
    return Xvr_createRefStringLength(refString->data, refString->length);
}

const char* Xvr_toCString(Xvr_RefString* refString) { return refString->data; }

bool Xvr_equalsRefString(Xvr_RefString* lhs, Xvr_RefString* rhs) {
    if (lhs == rhs) {
        return true;
    }

    if (lhs->length != rhs->length) {
        return false;
    }

    return strncmp(lhs->data, rhs->data, lhs->length) == 0;
}

bool Xvr_equalsRefStringCString(Xvr_RefString* lhs, char* cstring) {
    size_t length = strlen(cstring);

    if (lhs->length != length) {
        return false;
    }

    return strncmp(lhs->data, cstring, lhs->length) == 0;
}
