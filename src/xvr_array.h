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

#ifndef XVR_ARRAY_INITIAL_CAPACITY
#define XVR_ARRAY_INITIAL_CAPACITY 8
#endif // !XVR_ARRAY_INITIAL_CAPACITY

#ifndef XVR_ARRAY_EXPANSION_RATE
#define XVR_ARRAY_EXPANSION_RATE 2
#endif // !XVR_ARRAY_EXPANSION_RATE

#ifndef XVR_ARRAY_ALLOCATE
#define XVR_ARRAY_ALLOCATE() Xvr_resizeArray(NULL, XVR_ARRAY_INITIAL_CAPACITY)
#endif // !XVR_ARRAY_ALLOCATE

#ifndef XVR_ARRAY_EXPAND
#define XVR_ARRAY_EXPAND(array)                                                \
  (array = (array != NULL && (array)->count + 1 > (array)->capacity            \
                ? Xvr_resizeArray(array, (array)->capacity *                   \
                                             XVR_ARRAY_EXPANSION_RATE)         \
                : array))
#endif // !XVR_ARRAY_EXPAND

#endif // !XVR_ARRAY_H
