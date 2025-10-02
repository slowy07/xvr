#ifndef XVR_VALUE_H
#define XVR_VALUE_H

#include "xvr_common.h"

struct Xvr_String;

typedef enum Xvr_ValueType {
  XVR_VALUE_NULL,
  XVR_VALUE_BOOLEAN,
  XVR_VALUE_INTEGER,
  XVR_VALUE_FLOAT,
  XVR_VALUE_STRING,
  XVR_VALUE_ARRAY,
  XVR_VALUE_DICTIONARY,
  XVR_VALUE_FUNCTION,
  XVR_VALUE_OPAQUE,
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
#define XVR_VALUE_IS_DICTIONARY(value) ((value).type == XVR_VALUE_DICTIONARY)
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

#define XVR_VALUE_IS_TRUTHY(value) Xvr_private_isTruthy(value)
XVR_API bool Xvr_private_isTruthy(Xvr_Value value);

#define XVR_VALUE_IS_EQUAL(left, right) Xvr_private_isEqual(left, right)
XVR_API bool Xvr_private_isEqual(Xvr_Value left, Xvr_Value right);

unsigned int Xvr_hashValue(Xvr_Value value);

#endif // !XVR_VALUE_H
