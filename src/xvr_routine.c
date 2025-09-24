#include "xvr_routine.h"
#include "xvr_ast.h"
#include "xvr_memory.h"
#include <string.h>

static void expand(void **handle, int *capacity, int* count, int amount) {
  while ((*count) + amount > (*capacity)) {
    int oldCapacity = (*capacity);

    (*capacity) = XVR_GROW_CAPACITY(oldCapacity);
    (*handle) =
        XVR_GROW_ARRAY(unsigned char, (*handle), oldCapacity, (*capacity));
  }
}

static void emitByte(void **handle, int *capacity, int *count,
                     unsigned char byte) {
  expand(handle, capacity, count, 1);
  ((unsigned char *)(*handle))[(*count)++] = byte;
}

Xvr_routine Xvr_compileRoutine(Xvr_Ast *ast) {
  Xvr_routine rt;

  rt.param = NULL;
  rt.paramCapacity = 0;
  rt.paramCount = 0;

  rt.code = NULL;
  rt.codeCapacity = 0;
  rt.codeCount = 0;

  rt.data = NULL;
  rt.dataCapacity = 0;
  rt.dataCount = 0;

  rt.jump = NULL;
  rt.jumpCapacity = 0;
  rt.jumpCount = 0;

  return rt;
}

void Xvr_freeRoutine(Xvr_routine rt) {
  XVR_FREE_ARRAY(unsigned char, rt.param, rt.paramCapacity);
  XVR_FREE_ARRAY(unsigned char, rt.code, rt.codeCapacity);
  XVR_FREE_ARRAY(unsigned char, rt.data, rt.dataCapacity);
  XVR_FREE_ARRAY(int, rt.jump, rt.jumpCapacity);
}
