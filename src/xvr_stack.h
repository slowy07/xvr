#ifndef XVR_STACK_H
#define XVR_STACK_H

#include "xvr_common.h"
#include "xvr_value.h"

typedef struct Xvr_Stack { // 32 | 64 BITNESS
  unsigned int capacity;   // 4  | 4
  unsigned int count;      // 4  | 4
  char data[];             //-  | -
} Xvr_Stack;               // 8  | 8

XVR_API Xvr_Stack *Xvr_allocateStack();
XVR_API void Xvr_freeStack(Xvr_Stack *stack);

XVR_API void Xvr_pushStack(Xvr_Stack **stack, Xvr_Value value);
XVR_API Xvr_Value Xvr_peekStack(Xvr_Stack **stack);
XVR_API Xvr_Value Xvr_popStack(Xvr_Stack **stack);

#endif // !XVR_STACK_H
