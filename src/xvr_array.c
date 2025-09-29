#include "xvr_array.h"
#include "xvr_console_colors.h"

#include <stdio.h>
#include <stdlib.h>

Xvr_Array *Xvr_resizeArray(Xvr_Array *paramArray, unsigned int capacity) {
  if (capacity == 0) {
    free(paramArray);
    return NULL;
  }

  Xvr_Array *array = realloc(paramArray, capacity + sizeof(Xvr_Array));

  if (array == NULL) {
    fprintf(
        stderr,
        XVR_CC_ERROR
        "ERROR: Failed to allocate a 'Xvr_Array' of %d capacity\n" XVR_CC_RESET,
        (int)capacity);
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
