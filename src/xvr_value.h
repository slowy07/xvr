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

#ifndef XVR_VALUE_H
#define XVR_VALUE_H

#include "xvr_common.h"
#include "xvr_print.h"

struct Xvr_String;

typedef enum Xvr_ValueType {
  XVR_VALUE_NULL,
  XVR_VALUE_BOOLEAN,
  XVR_VALUE_INTEGER,
  XVR_VALUE_FLOAT,
  XVR_VALUE_STRING,
  XVR_VALUE_ARRAY,
  XVR_VALUE_TABLE,
  XVR_VALUE_FUNCTION,
  XVR_VALUE_OPAQUE,

  XVR_VALUE_TYPE,
  XVR_VALUE_ANY,
  XVR_VALUE_UNKNOWN,
} Xvr_ValueType;

// 8 bytes in size
typedef struct Xvr_Value { // 32 | 64 BITNESS
  union {
    bool boolean;              // 1  | 1
    int integer;               // 4  | 4
    float number;              // 4  | 4
    struct Xvr_String *string; // 4 | 8
  } as;                        // 4  | 4

  Xvr_ValueType type; // 4  | 4
} Xvr_Value;          // 8  | 8

#define XVR_VALUE_IS_NULL(value) ((value).type == XVR_VALUE_NULL)
#define XVR_VALUE_IS_BOOLEAN(value) ((value).type == XVR_VALUE_BOOLEAN)
#define XVR_VALUE_IS_INTEGER(value) ((value).type == XVR_VALUE_INTEGER)
#define XVR_VALUE_IS_FLOAT(value) ((value).type == XVR_VALUE_FLOAT)
#define XVR_VALUE_IS_STRING(value) ((value).type == XVR_VALUE_STRING)
#define XVR_VALUE_IS_ARRAY(value) ((value).type == XVR_VALUE_ARRAY)
#define XVR_VALUE_IS_TABLE(value) ((value).type == XVR_VALUE_TABLE)
#define XVR_VALUE_IS_FUNCTION(value) ((value).type == XVR_VALUE_FUNCTION)
#define XVR_VALUE_IS_OPAQUE(value) ((value).type == XVR_VALUE_OPAQUE)

#define XVR_VALUE_AS_BOOLEAN(value) ((value).as.boolean)
#define XVR_VALUE_AS_INTEGER(value) ((value).as.integer)
#define XVR_VALUE_AS_FLOAT(value) ((value).as.number)
#define XVR_VALUE_AS_STRING(value) ((value).as.string)

#define XVR_VALUE_FROM_NULL() ((Xvr_Value){{.integer = 0}, XVR_VALUE_NULL})
#define XVR_VALUE_FROM_BOOLEAN(value)                                          \
  ((Xvr_Value){{.boolean = value}, XVR_VALUE_BOOLEAN})
#define XVR_VALUE_FROM_INTEGER(value)                                          \
  ((Xvr_Value){{.integer = value}, XVR_VALUE_INTEGER})
#define XVR_VALUE_FROM_FLOAT(value)                                            \
  ((Xvr_Value){{.number = value}, XVR_VALUE_FLOAT})
#define XVR_VALUE_FROM_STRING(value)                                           \
  ((Xvr_Value){{.string = value}, XVR_VALUE_STRING})

XVR_API unsigned int Xvr_hashValue(Xvr_Value value);

XVR_API Xvr_Value Xvr_copyValue(Xvr_Value value);
XVR_API void Xvr_freeValue(Xvr_Value value);

XVR_API bool Xvr_checkValueIsTruthy(Xvr_Value value);
XVR_API bool Xvr_checkValuesAreEqual(Xvr_Value left, Xvr_Value right);
XVR_API bool Xvr_checkValuesAreCompareable(Xvr_Value left, Xvr_Value right);
XVR_API int Xvr_compareValues(Xvr_Value left, Xvr_Value right);

XVR_API void Xvr_stringifyValue(Xvr_Value value, Xvr_callbackType callback);

XVR_API const char *Xvr_private_getValueTypeAsCString(Xvr_ValueType type);

#endif // !XVR_VALUE_H
