/**
MIT License

Copyright (c) 2025 arfy slowy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef XVR_STACK_H
#define XVR_STACK_H

#include "xvr_common.h"
#include "xvr_value.h"

typedef struct Xvr_Stack { // 32 | 64 BITNESS
  unsigned int capacity;   // 4  | 4
  unsigned int count;      // 4  | 4
  Xvr_Value data[];             //-  | -
} Xvr_Stack;               // 8  | 8

XVR_API Xvr_Stack *Xvr_allocateStack();
XVR_API void Xvr_freeStack(Xvr_Stack *stack);

XVR_API void Xvr_pushStack(Xvr_Stack **stack, Xvr_Value value);
XVR_API Xvr_Value Xvr_peekStack(Xvr_Stack **stack);
XVR_API Xvr_Value Xvr_popStack(Xvr_Stack **stack);

#ifndef XVR_STACK_INITIAL_CAPACITY
#define XVR_STACK_INITIAL_CAPACITY 8
#endif // !XVR_STACK_INITIAL_CAPACITY

#ifndef XVR_STACK_EXPANSION_RATE
#define XVR_STACK_EXPANSION_RATE 2
#endif // !XVR_STACK_EXPANSION_RATE

#ifndef XVR_STACK_CONTRACTION_THRESHOLD
#define XVR_STACK_CONTRACTION_THRESHOLD (1 / 4)
#endif // !XVR_STACK_CONTRACTION_THRESHOLD

#ifndef XVR_STACK_OVERFLOW
#define XVR_STACK_OVERFLOW (1024 * 1204 / sizeof(Xvr_Value))
#endif // !XVR_STACK_OVERFLOW

#endif // !XVR_STACK_H
