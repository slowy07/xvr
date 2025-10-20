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

#ifndef XVR_STRING_H
#define XVR_STRING_H

#include <assert.h>

#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_value.h"

union Xvr_String_t;

typedef enum Xvr_StringType {
    XVR_STRING_NODE,
    XVR_STRING_LEAF,
    XVR_STRING_NAME,
} Xvr_StringType;

typedef struct Xvr_StringInfo {
    Xvr_StringType type;
    unsigned int length;
    unsigned int refCount;
    unsigned int cachedHash;
} Xvr_StringInfo;

typedef struct Xvr_StringNode {
    Xvr_StringInfo _padding;
    union Xvr_String_t* left;
    union Xvr_String_t* right;
} Xvr_StringNode;

typedef struct Xvr_StringLeaf {
    Xvr_StringInfo _padding;
    char data[];
} Xvr_StringLeaf;

typedef struct Xvr_StringName {
    Xvr_StringInfo _padding;
    Xvr_ValueType varType;
    bool varConstant;
    char data[];
} Xvr_StringName;

typedef union Xvr_String_t {
    Xvr_StringInfo info;
    Xvr_StringNode node;
    Xvr_StringLeaf leaf;
    Xvr_StringName name;
} Xvr_String;

XVR_API Xvr_String* Xvr_createString(Xvr_Bucket** bucketHandle,
                                     const char* cstring);
XVR_API Xvr_String* Xvr_createStringLength(Xvr_Bucket** bucketHandle,
                                           const char* cstring,
                                           unsigned int length);

XVR_API Xvr_String* Xvr_createNameStringLength(Xvr_Bucket** bucketHandle,
                                               const char* cname,
                                               unsigned int length,
                                               Xvr_ValueType varType,
                                               bool constant);

XVR_API Xvr_String* Xvr_copyString(Xvr_String* str);
XVR_API Xvr_String* Xvr_deepCopyString(Xvr_Bucket** bucketHandle,
                                       Xvr_String* str);

XVR_API Xvr_String* Xvr_concatStrings(Xvr_Bucket** bucketHandle,
                                      Xvr_String* left, Xvr_String* right);

XVR_API void Xvr_freeString(Xvr_String* str);

XVR_API unsigned int Xvr_getStringLength(Xvr_String* str);
XVR_API unsigned int Xvr_getStringRefCount(Xvr_String* str);
XVR_API Xvr_ValueType Xvr_getNameStringVarType(Xvr_String* str);
XVR_API Xvr_ValueType Xvr_getNameStringVarConstant(Xvr_String* str);

XVR_API char* Xvr_getStringRawBuffer(Xvr_String* str);

XVR_API int Xvr_compareStrings(Xvr_String* left, Xvr_String* right);

XVR_API unsigned int Xvr_hashString(Xvr_String* string);

#endif  // !XVR_STRING_H
