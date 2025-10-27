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
#include "xvr_common.h"
#include "xvr_module.h"
#include "xvr_scope.h"
#include "xvr_stack.h"

typedef struct Xvr_VM {
    unsigned char* code;

    // metadata
    unsigned int jumpsCount;
    unsigned int paramCount;
    unsigned int dataCount;
    unsigned int subsCount;

    unsigned int codeAddr;
    unsigned int jumpsAddr;
    unsigned int paramAddr;
    unsigned int dataAddr;
    unsigned int subsAddr;

    // execution utils
    unsigned int programCounter;

    // scope - block-level key/value pairs
    Xvr_Scope* scope;

    // stack - immediate-level values only
    Xvr_Stack* stack;

    // easy access to memory
    Xvr_Bucket* stringBucket;
    Xvr_Bucket* scopeBucket;
} Xvr_VM;

XVR_API void Xvr_resetVM(Xvr_VM* vm, bool preserveScope);

XVR_API void Xvr_initVM(Xvr_VM* vm);
XVR_API void Xvr_inheritVM(Xvr_VM* vm, Xvr_VM* parent);

XVR_API void Xvr_bindVM(Xvr_VM* vm, Xvr_Module* module, bool preserveScope);
XVR_API void Xvr_runVM(Xvr_VM* vm);

XVR_API void Xvr_freeVM(Xvr_VM* vm);

#endif  // !XVR_WM_H
