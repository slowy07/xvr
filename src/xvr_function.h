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

#ifndef XVR_FUNCTION_H
#define XVR_FUNCTION_H

#include "xvr_common.h"
#include "xvr_module.h"

typedef enum Xvr_FunctionType {
    XVR_FUNCTION_MODULE,
    XVF_FUNCTION_NATIVE,
} Xvr_FunctionType;

typedef union Xvr_FunctionModule {
    Xvr_FunctionType type;
    Xvr_Module module;
} Xvr_FunctionModule;

typedef union Xvr_FunctionNative {
    Xvr_FunctionType type;
    void* native;
} Xvr_FunctionNative;

typedef union Xvr_Function_t {
    Xvr_FunctionType type;
    Xvr_FunctionModule module;
    Xvr_FunctionNative native;
} Xvr_Function;

XVR_API Xvr_Function* Xvr_createModuleFunction(Xvr_Bucket** bucketHandle,
                                               Xvr_Module module);

#endif  // !XVR_FUNCTION_H
