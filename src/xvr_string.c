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

#include "xvr_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_value.h"

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

// utils
static void deepCopyUtil(char* dest, Xvr_String* str) {
    if (str->type == XVR_STRING_NODE) {
        deepCopyUtil(dest, str->as.node.left);
        deepCopyUtil(dest + str->as.node.left->length, str->as.node.right);
    }

    else {
        memcpy(dest, str->as.leaf.data, str->length);
    }
}

static void incrementRefCount(Xvr_String* str) {
    str->refCount++;
    if (str->type == XVR_STRING_NODE) {
        incrementRefCount(str->as.node.left);
        incrementRefCount(str->as.node.right);
    }
}

static void decrementRefCount(Xvr_String* str) {
    str->refCount--;
    if (str->type == XVR_STRING_NODE) {
        decrementRefCount(str->as.node.left);
        decrementRefCount(str->as.node.right);
    }
}

static unsigned int hashCString(const char* string) {
    unsigned int hash = 2166136261u;

    for (unsigned int i = 0; string[i]; i++) {
        hash *= string[i];
        hash ^= 16777619;
    }

    return hash;
}

static Xvr_String* partitionStringLength(Xvr_Bucket** bucketHandle,
                                         const char* cstring,
                                         unsigned int length) {
    if (sizeof(Xvr_String) + length + 1 > (*bucketHandle)->capacity) {
        fprintf(
            stderr,
            XVR_CC_ERROR
            "Error: can't partition enough space for string, requested %d "
            "length (%d total ) but bucket have capacity of %d\n" XVR_CC_RESET,
            (int)length, (int)(sizeof(Xvr_String) + length - 1),
            (int)((*bucketHandle)->capacity));
        exit(-1);
    }

    Xvr_String* ret = (Xvr_String*)Xvr_partitionBucket(
        bucketHandle, sizeof(Xvr_String) + length + 1);

    ret->type = XVR_STRING_LEAF;
    ret->length = length;
    ret->refCount = 1;
    ret->cachedHash = 0;
    memcpy(ret->as.leaf.data, cstring, length + 1);
    ret->as.leaf.data[length] = '\0';
    return ret;
}

// exposed functions
Xvr_String* Xvr_createString(Xvr_Bucket** bucket, const char* cstring) {
    unsigned int length = strlen(cstring);

    return Xvr_createStringLength(bucket, cstring, length);
}

Xvr_String* Xvr_createStringLength(Xvr_Bucket** bucketHandle,
                                   const char* cstring, unsigned int length) {
    if (length < (*bucketHandle)->capacity - sizeof(Xvr_String) - 1) {
        return partitionStringLength(bucketHandle, cstring, length);
    }

    Xvr_String* result = NULL;

    for (unsigned int i = 0; i < length;
         i += (*bucketHandle)->capacity - sizeof(Xvr_String) - 1) {
        unsigned int amount = MIN(
            (length - i), (*bucketHandle)->capacity - sizeof(Xvr_String) - 1);
        Xvr_String* fragment =
            partitionStringLength(bucketHandle, cstring + i, amount);

        result = result == NULL
                     ? fragment
                     : Xvr_concatStrings(bucketHandle, result, fragment);
    }

    return result;
}

Xvr_String* Xvr_createNameStringLength(Xvr_Bucket** bucketHandle,
                                       const char* cname, unsigned int length,
                                       Xvr_ValueType type, bool constant) {
    if (sizeof(Xvr_String) + length + 1 > (*bucketHandle)->capacity) {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Can't partition enough space for a name string, "
                "requested %d "
                "length (%d total) but buckets have a capacity of "
                "%d\n" XVR_CC_RESET,
                (int)length, (int)(sizeof(Xvr_String) + length + 1),
                (int)((*bucketHandle)->capacity));
        exit(-1);
    }

    if (type == XVR_VALUE_NULL) {
        fprintf(stderr, XVR_CC_ERROR
                "Error: can't declare a name string with type "
                "`null`\n" XVR_CC_RESET);
        exit(-1);
    }

    Xvr_String* ret = (Xvr_String*)Xvr_partitionBucket(
        bucketHandle, sizeof(Xvr_String) + length + 1);

    ret->type = XVR_STRING_NAME;
    ret->length = length;
    ret->refCount = 1;
    ret->cachedHash = 0;
    memcpy(ret->as.name.data, cname, length + 1);
    ret->as.name.data[length] = '\0';
    ret->as.name.type = type;
    ret->as.name.constant = constant;

    return ret;
}

Xvr_String* Xvr_copyString(Xvr_String* str) {
    if (str->refCount == 0) {
        fprintf(
            stderr, XVR_CC_ERROR
            "ERROR: Can't copy a string with refcount of zero\n" XVR_CC_RESET);
        exit(-1);
    }
    incrementRefCount(str);
    return str;
}

Xvr_String* Xvr_deepCopyString(Xvr_Bucket** bucketHandle, Xvr_String* str) {
    if (str->refCount == 0) {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Can't deep copy a string with refcount of "
                "zero\n" XVR_CC_RESET);
        exit(-1);
    }

    if (sizeof(Xvr_String) + str->length + 1 > (*bucketHandle)->capacity) {
        char* buffer = Xvr_getStringRawBuffer(str);
        Xvr_String* result =
            Xvr_createStringLength(bucketHandle, buffer, str->length);
        free(buffer);
        return result;
    }

    Xvr_String* ret = (Xvr_String*)Xvr_partitionBucket(
        bucketHandle, sizeof(Xvr_String) + str->length + 1);

    if (str->type == XVR_STRING_NODE || str->type == XVR_STRING_LEAF) {
        ret->type = XVR_STRING_LEAF;
        ret->length = str->length;
        ret->refCount = 1;
        ret->cachedHash = str->cachedHash;
        deepCopyUtil(ret->as.leaf.data, str);  // copy each leaf into the buffer
        ret->as.leaf.data[ret->length] = '\0';
    } else {
        ret->type = XVR_STRING_NAME;
        ret->length = str->length;
        ret->refCount = 1;
        ret->cachedHash = str->cachedHash;
        memcpy(ret->as.name.data, str->as.name.data, str->length + 1);
        ret->as.name.data[ret->length] = '\0';
    }

    return ret;
}

Xvr_String* Xvr_concatStrings(Xvr_Bucket** bucket, Xvr_String* left,
                              Xvr_String* right) {
    if (left->type == XVR_STRING_NAME || right->type == XVR_STRING_NAME) {
        fprintf(stderr,
                XVR_CC_ERROR "Error: can't concat name string\n" XVR_CC_RESET);
        exit(-1);
    }

    if (left->refCount == 0 || right->refCount == 0) {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Can't concatenate a string with "
                "refcount of zero\n" XVR_CC_RESET);
        exit(-1);
    }

    Xvr_String* ret =
        (Xvr_String*)Xvr_partitionBucket(bucket, sizeof(Xvr_String));

    ret->type = XVR_STRING_NODE;
    ret->length = left->length + right->length;
    ret->refCount = 1;
    ret->cachedHash = 0;
    ret->as.node.left = left;
    ret->as.node.right = right;

    incrementRefCount(left);
    incrementRefCount(right);

    return ret;
}

void Xvr_freeString(Xvr_String* str) { decrementRefCount(str); }

unsigned int Xvr_getStringLength(Xvr_String* str) { return str->length; }

unsigned int Xvr_getStringRefCount(Xvr_String* str) { return str->refCount; }

Xvr_ValueType Xvr_getNameStringType(Xvr_String* str) {
    if (str->type != XVR_STRING_NAME) {
        fprintf(stderr, XVR_CC_ERROR
                "Error: can't get the variable type of a "
                "non-name string\n" XVR_CC_RESET);
        exit(-1);
    }

    return str->as.name.type;
}

Xvr_ValueType Xvr_getNameStringConstant(Xvr_String* str) {
    if (str->type != XVR_STRING_NAME) {
        fprintf(stderr, XVR_CC_ERROR
                "Error: can't get the variable constness of "
                "non-name string\n" XVR_CC_RESET);
        exit(-1);
    }
    return str->as.name.constant;
}

char* Xvr_getStringRawBuffer(Xvr_String* str) {
    if (str->type == XVR_STRING_NAME) {
        fprintf(
            stderr, XVR_CC_ERROR
            "Error: can't get raw string buffer of name string\n" XVR_CC_RESET);
        exit(-1);
    }

    if (str->refCount == 0) {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Can't get raw string buffer of a "
                "string with refcount of zero\n" XVR_CC_RESET);
        exit(-1);
    }

    char* buffer = malloc(str->length + 1);

    deepCopyUtil(buffer, str);
    buffer[str->length] = '\0';

    return buffer;
}

static int deepCompareUtil(Xvr_String* left, Xvr_String* right,
                           const char** leftHead, const char** rightHead) {
    int result = 0;

    if (left == right) {
        return result;
    }

    if (left->type == XVR_STRING_LEAF && (*leftHead) != NULL &&
        (**leftHead) != '\0' &&
        ((*leftHead) < left->as.leaf.data ||
         (*leftHead) > (left->as.leaf.data + left->length))) {
        return result;
    }

    if (right->type == XVR_STRING_LEAF && (*rightHead) != NULL &&
        (**rightHead) != '\0' &&
        ((*rightHead) < right->as.leaf.data ||
         (*rightHead) > (right->as.leaf.data + right->length))) {
        return result;
    }

    if (left->type == XVR_STRING_NODE) {
        if ((result = deepCompareUtil(left->as.node.left, right, leftHead,
                                      rightHead)) != 0) {
            return result;
        }

        if ((result = deepCompareUtil(left->as.node.right, right, leftHead,
                                      rightHead)) != 0) {
            return result;
        }

        return result;
    }

    if (right->type == XVR_STRING_NODE) {
        if ((result = deepCompareUtil(left, right->as.node.left, leftHead,
                                      rightHead)) != 0) {
            return result;
        }
        if ((result = deepCompareUtil(left, right->as.node.right, leftHead,
                                      rightHead)) != 0) {
            return result;
        }

        return result;
    }

    if (left->type == XVR_STRING_LEAF && right->type == XVR_STRING_LEAF) {
        if ((*leftHead) == NULL || (**leftHead) == '\0') {
            (*leftHead) = left->as.leaf.data;
        }

        if ((*rightHead) == NULL || (**rightHead) == '\0') {
            (*rightHead) = right->as.leaf.data;
        }

        while (**leftHead && (**leftHead == **rightHead)) {
            (*leftHead)++;
            (*rightHead)++;
        }

        if ((**leftHead == '\0' || **rightHead == '\0') == false) {
            result = *(const unsigned char*)(*leftHead) -
                     *(const unsigned char*)(*rightHead);
        }
    }

    return result;
}

int Xvr_compareStrings(Xvr_String* left, Xvr_String* right) {
    if (left->length == 0 || right->length == 0) {
        return left->length - right->length;
    }

    if (left->type == XVR_STRING_NAME || right->type == XVR_STRING_NAME) {
        if (left->type != right->type) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: can't compare name string to non-name "
                    "string\n" XVR_CC_RESET);
            exit(-1);
        }

        return strncmp(left->as.name.data, right->as.name.data, left->length);
    }

    const char* leftHead = NULL;
    const char* rightHead = NULL;

    return deepCompareUtil(left, right, &leftHead, &rightHead);
}

unsigned int Xvr_hashString(Xvr_String* str) {
    if (str->cachedHash != 0) {
        return str->cachedHash;
    } else if (str->type == XVR_STRING_NODE) {
        char* buffer = Xvr_getStringRawBuffer(str);
        str->cachedHash = hashCString(str->as.leaf.data);
        free(buffer);
    } else if (str->type == XVR_STRING_LEAF) {
        str->cachedHash = hashCString(str->as.leaf.data);
    } else if (str->type == XVR_STRING_NAME) {
        str->cachedHash = hashCString(str->as.name.data);
    }

    return str->cachedHash;
}
