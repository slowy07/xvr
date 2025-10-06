#include "xvr_array.h"
#include "xvr_console_colors.h"
#include "xvr_value.h"

#include <stdio.h>
#include <stdlib.h>

Xvr_Array *Xvr_resizeArray(Xvr_Array *paramArray, unsigned int capacity) {
  if (capacity == 0) {
    free(paramArray);
    return NULL;
  }

  unsigned int originalCapacity = paramArray == NULL ? 0 : paramArray->capacity;
  Xvr_Array *array =
      realloc(paramArray, capacity * sizeof(Xvr_Value) + sizeof(Xvr_Array));

  if (array == NULL) {
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Failed to resizing a 'Xvr_Array' from %d to "
                         "%d capacity\n" XVR_CC_RESET,
            (int)originalCapacity, (int)capacity);
    exit(-1);
  }

  array->capacity = capacity;
  array->count =
      paramArray == NULL
          ? 0
          : (array->count > capacity ? capacity
                                     : array->count); // truncate lost data

  return array;
}
