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

#ifndef XVR_LLVM_MODULE_MANAGER_H
#define XVR_LLVM_MODULE_MANAGER_H

#include <llvm-c/Core.h>
#include <stdbool.h>
#include <stddef.h>

#include "xvr_llvm_context.h"

typedef struct Xvr_LLVMModuleManager Xvr_LLVMModuleManager;

Xvr_LLVMModuleManager* Xvr_LLVMModuleManagerCreate(Xvr_LLVMContext* ctx,
                                                   const char* name);
void Xvr_LLVMModuleManagerDestroy(Xvr_LLVMModuleManager* mgr);

LLVMModuleRef Xvr_LLVMModuleManagerGetModule(Xvr_LLVMModuleManager* mgr);
bool Xvr_LLVMModuleManagerAddFunction(Xvr_LLVMModuleManager* mgr,
                                      const char* name,
                                      LLVMTypeRef function_type);
void Xvr_LLVMModuleManagerRegisterFunctionType(Xvr_LLVMModuleManager* mgr,
                                               const char* name,
                                               LLVMTypeRef function_type);
LLVMValueRef Xvr_LLVMModuleManagerGetFunction(Xvr_LLVMModuleManager* mgr,
                                              const char* name);
LLVMTypeRef Xvr_LLVMModuleManagerGetFunctionType(Xvr_LLVMModuleManager* mgr,
                                                 const char* name);
bool Xvr_LLVMModuleManagerAddGlobal(Xvr_LLVMModuleManager* mgr,
                                    const char* name, LLVMTypeRef type);
LLVMValueRef Xvr_LLVMModuleManagerGetGlobal(Xvr_LLVMModuleManager* mgr,
                                            const char* name);

char* Xvr_LLVMModuleManagerPrintIR(Xvr_LLVMModuleManager* mgr, size_t* out_len);
bool Xvr_LLVMModuleManagerWriteBitcode(Xvr_LLVMModuleManager* mgr,
                                       const char* filepath);
bool Xvr_LLVMModuleManagerWriteObjectFile(Xvr_LLVMModuleManager* mgr,
                                          const char* filepath);

#endif
