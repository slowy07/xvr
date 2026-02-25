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

#include "xvr_llvm_optimizer.h"

#include <stdlib.h>
#include <string.h>

#include "xvr_llvm_module_manager.h"

struct Xvr_LLVMOptimizer {
    Xvr_LLVMOptimizationLevel level;
    void* pass_manager;
};

Xvr_LLVMOptimizer* Xvr_LLVMOptimizerCreate(void) {
    Xvr_LLVMOptimizer* opt = calloc(1, sizeof(Xvr_LLVMOptimizer));
    if (!opt) {
        return NULL;
    }
    opt->level = XVR_LLVM_OPT_O2;
    return opt;
}

void Xvr_LLVMOptimizerDestroy(Xvr_LLVMOptimizer* opt) { free(opt); }

bool Xvr_LLVMOptimizerSetLevel(Xvr_LLVMOptimizer* opt,
                               Xvr_LLVMOptimizationLevel level) {
    if (!opt) {
        return false;
    }
    opt->level = level;
    return true;
}

bool Xvr_LLVMOptimizerRun(Xvr_LLVMOptimizer* opt,
                          Xvr_LLVMModuleManager* module) {
    (void)opt;
    (void)module;
    return false;
}

bool Xvr_LLVMOptimizerAddPass(Xvr_LLVMOptimizer* opt, const char* pass_name) {
    (void)opt;
    (void)pass_name;
    return false;
}

bool Xvr_LLVMOptimizerAddStandardPasses(Xvr_LLVMOptimizer* opt) {
    (void)opt;
    return true;
}
