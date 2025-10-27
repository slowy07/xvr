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

#ifndef XVR_MODULE_BUNDLE_H
#define XVR_MODULE_BUNDLE_H

#include "xvr_ast.h"
#include "xvr_common.h"
#include "xvr_module.h"

typedef struct Xvr_ModuleBundle {
    unsigned char* ptr;
    unsigned int capacity;
    unsigned int count;
} Xvr_ModuleBundle;

XVR_API void Xvr_initModuleBundle(Xvr_ModuleBundle* bundle);
XVR_API void Xvr_appendModuleBundle(Xvr_ModuleBundle* bundle, Xvr_Ast* ast);
XVR_API void Xvr_freeModuleBundle(Xvr_ModuleBundle* bundle);

XVR_API void Xvr_bindModuleBundle(Xvr_ModuleBundle* bundle, unsigned char* ptr,
                                  unsigned int size);
XVR_API Xvr_Module Xvr_extractModuleFromBundle(Xvr_ModuleBundle* bundle,
                                               unsigned char index);

#endif  // !XVR_MODULE_BUNDLE_H
