#ifndef XVR_WM_H
#define XVR_WM_H

#include "xvr_common.h"
#include "xvr_stack.h"

typedef struct Xvr_VM {
  unsigned char *bc;
  unsigned int bcSize;

  unsigned char *routine;
  unsigned int routineSize;

  unsigned int paramCount;
  unsigned int jumpsCount;
  unsigned int dataCount;
  unsigned int subsCount;

  unsigned int paramAddr;
  unsigned int codeAddr;
  unsigned int jumpsAddr;
  unsigned int dataAddr;
  unsigned int subsAddr;

  unsigned int routineCounter;
  Xvr_Stack *stack;
} Xvr_VM;

XVR_API void Xvr_bindVM(Xvr_VM *vm, unsigned char *bytecode,
                        unsigned int bytecodeSize); // process the version data
XVR_API void
Xvr_bindVMToRoutine(Xvr_VM *vm,
                    unsigned char *routine); // process the routine only

XVR_API void Xvr_runVM(Xvr_VM *vm);
XVR_API void Xvr_freeVM(Xvr_VM *vm);
XVR_API void Xvr_resetVM(Xvr_VM *vm);

#endif // !XVR_WM_H
