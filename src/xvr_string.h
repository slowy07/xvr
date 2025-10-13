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

#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_value.h"
#include <assert.h>

typedef struct Xvr_String { // 32 | 64 BITNESS
  enum Xvr_StringType {
    XVR_STRING_NODE,
    XVR_STRING_LEAF,
    XVR_STRING_NAME,
  } type; // 4  | 4

  unsigned int length;     // 4  | 4
  unsigned int refCount;   // 4  | 4
  unsigned int cachedHash; // 4 | 4

  int _padding; // 4  | 4

  union {
    struct {
      struct Xvr_String *left;  // 4  | 8
      struct Xvr_String *right; // 4  | 8
    } node;                     // 8  | 16

    struct {
      int _dummy;  // 4  | 4
      char data[]; //-  | -
    } leaf;

    struct {
      Xvr_ValueType type; // 4 | 4
      bool constant;      // 1 | 1
      char data[];        // - | -
    } name;
  } as;       // 8  | 16
} Xvr_String; // 24 | 32

XVR_API Xvr_String *Xvr_createString(Xvr_Bucket **bucketHandle,
                                     const char *cstring);
XVR_API Xvr_String *Xvr_createStringLength(Xvr_Bucket **bucketHandle,
                                           const char *cstring,
                                           unsigned int length);

XVR_API Xvr_String *Xvr_createNameStringLength(Xvr_Bucket **bucketHandle,
                                               const char *cname,
                                               unsigned int length,
                                               Xvr_ValueType type,
                                               bool constant);
XVR_API Xvr_String *Xvr_copyString(Xvr_String *str);
XVR_API Xvr_String *Xvr_deepCopyString(Xvr_Bucket **bucketHandle,
                                       Xvr_String *str);

XVR_API Xvr_String *Xvr_concatStrings(Xvr_Bucket **bucketHandle,
                                      Xvr_String *left, Xvr_String *right);

XVR_API void Xvr_freeString(Xvr_String *str);

XVR_API unsigned int Xvr_getStringLength(Xvr_String *str);
XVR_API unsigned int Xvr_getStringRefCount(Xvr_String *str);
XVR_API Xvr_ValueType Xvr_getNameStringType(Xvr_String *str);
XVR_API Xvr_ValueType Xvr_getNameStringConstant(Xvr_String *str);
XVR_API char *Xvr_getStringRawBuffer(Xvr_String *str);

XVR_API int Xvr_compareStrings(Xvr_String *left, Xvr_String *right);

XVR_API unsigned int Xvr_hashString(Xvr_String *string);

#endif // !XVR_STRING_H
