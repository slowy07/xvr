#ifndef XVR_ARRAY_H
#define XVR_ARRAY_H
#include "xvr_common.h"
#include "xvr_value.h"

typedef struct Xvr_Array { // 32 | 64 BITNESS
  unsigned int capacity;   // 4  | 4
  unsigned int count;      // 4  | 4
  Xvr_Value data[];        //-  | -
} Xvr_Array;               // 8  | 8

XVR_API Xvr_Array *Xvr_resizeArray(Xvr_Array *array, unsigned int capacity);

#endif // !XVR_ARRAY_H
