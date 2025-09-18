#include "xvr_memory.h"
#include "xvr_console_color.h"
#include <stdio.h>
#include <stdlib.h>

void *Xvr_reallocate(void *pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, newSize);

  if (result == NULL) {
    fprintf(stderr,
            XVR_CC_ERROR "[internal] error: memory allocation error (requested "
                         "%d, replacing %d)\n" XVR_CC_RESET,
            (int)newSize, (int)oldSize);
    exit(1);
  }

  return result;
}
