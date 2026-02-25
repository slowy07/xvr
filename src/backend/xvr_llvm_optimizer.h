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

#ifndef XVR_LLVM_OPTIMIZER_H
#define XVR_LLVM_OPTIMIZER_H

#include <stdbool.h>
#include <stddef.h>

#include "xvr_llvm_module_manager.h"

typedef enum Xvr_LLVMOptimizationLevel {
    XVR_LLVM_OPT_NONE,
    XVR_LLVM_OPT_LEGACY,
    XVR_LLVM_OPT_O1,
    XVR_LLVM_OPT_O2,
    XVR_LLVM_OPT_O3,
    XVR_LLVM_OPT_OS,
    XVR_LLVM_OPT_OZ
} Xvr_LLVMOptimizationLevel;

typedef struct Xvr_LLVMOptimizer Xvr_LLVMOptimizer;

Xvr_LLVMOptimizer* Xvr_LLVMOptimizerCreate(void);
void Xvr_LLVMOptimizerDestroy(Xvr_LLVMOptimizer* opt);

bool Xvr_LLVMOptimizerSetLevel(Xvr_LLVMOptimizer* opt,
                               Xvr_LLVMOptimizationLevel level);
bool Xvr_LLVMOptimizerRun(Xvr_LLVMOptimizer* opt,
                          Xvr_LLVMModuleManager* module);

bool Xvr_LLVMOptimizerAddPass(Xvr_LLVMOptimizer* opt, const char* pass_name);
bool Xvr_LLVMOptimizerAddStandardPasses(Xvr_LLVMOptimizer* opt);

#endif
