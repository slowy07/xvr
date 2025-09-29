#ifndef XVR_ARRAY_H
#define XVR_ARRAY_H
#include "xvr_common.h"

typedef struct Xvr_Array { // 32 | 64 BITNESS
  unsigned int capacity;   // 4  | 4
  unsigned int count;      // 4  | 4
  char data[];             //-  | -
} Xvr_Array;               // 8  | 8

XVR_API Xvr_Array *Xvr_resizeArray(Xvr_Array *array, unsigned int capacity);

#define XVR_ALLOCATE_ARRAY(type, count)                                        \
  Xvr_resizeArray(NULL, sizeof(type) * (count))

#define XVR_FREE_ARRAY(type, array) Xvr_resizeArray(array, 0)

#define XVR_ADJUST_ARRAY(type, array, newCapacity)                             \
  Xvr_resizeArray(array, sizeof(type) * newCapacity)

#define XVR_DOUBLE_ARRAY_CAPACITY(type, array)                                 \
  Xvr_resizeArray(array, sizeof(type) * array->capacity < 8                    \
                             ? sizeof(type) * 8                                \
                             : sizeof(type) * array->capacity * 2)

#endif // !XVR_ARRAY_H
