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

#ifndef XVR_WM_H
#define XVR_WM_H

#include "xvr_bucket.h"
#include "xvr_bytecode.h"
#include "xvr_common.h"
#include "xvr_scope.h"
#include "xvr_stack.h"

typedef struct Xvr_VM {
  unsigned char *module;
  unsigned int moduleSize;

  unsigned int paramSize;
  unsigned int jumpsSize;
  unsigned int dataSize;
  unsigned int subsSize;

  unsigned int paramAddr;
  unsigned int codeAddr;
  unsigned int jumpsAddr;
  unsigned int dataAddr;
  unsigned int subsAddr;

  unsigned int programCounter;
  Xvr_Stack *stack;

  Xvr_Scope *scope;
  Xvr_Bucket *stringBucket;
  Xvr_Bucket *scopeBucket;
} Xvr_VM;

XVR_API void Xvr_initVM(Xvr_VM *vm);
XVR_API void Xvr_bindVM(Xvr_VM *vm,
                        struct Xvr_Bytecode *bc); // process the version data

XVR_API void
Xvr_bindVMToModule(Xvr_VM *vm,
                   unsigned char *module); // process the routine only

XVR_API void Xvr_runVM(Xvr_VM *vm);
XVR_API void Xvr_freeVM(Xvr_VM *vm);
XVR_API void Xvr_resetVM(Xvr_VM *vm);

#endif // !XVR_WM_H
