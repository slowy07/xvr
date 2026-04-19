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

/* FIXME: Optimizer crashes when integrated into codegen pipeline.
 * The PassBuilder pipeline causes SIGSEGV (exit code 139) when
 * Xvr_LLVMCodegenRunOptimizer is called after EmitAST.
 *
 * TODO: Debug target machine configuration - the triple may not be
 * properly initialized when LLVMRunPasses is called.
 *
 * TODO: Consider using a simpler optimization approach or deferring
 * optimization until object file emission.
 */

#include "xvr_llvm_optimizer.h"

#include <llvm-c/Core.h>
#include <llvm-c/Error.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_target.h"

struct Xvr_LLVMOptimizer {
    Xvr_LLVMOptimizationLevel level;
    LLVMTargetMachineRef target_machine;
};

Xvr_LLVMOptimizer* Xvr_LLVMOptimizerCreate(void) {
    Xvr_LLVMOptimizer* opt = (Xvr_LLVMOptimizer*)calloc(1, sizeof(Xvr_LLVMOptimizer));
    if (!opt) {
        return NULL;
    }
    opt->level = XVR_LLVM_OPT_O2;
    opt->target_machine = NULL;
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

bool Xvr_LLVMOptimizerSetTargetMachine(Xvr_LLVMOptimizer* opt,
                                       LLVMTargetMachineRef tm) {
    if (!opt) {
        return false;
    }
    opt->target_machine = tm;
    return true;
}

static const char* get_pass_pipeline(Xvr_LLVMOptimizationLevel level) {
    switch (level) {
    case XVR_LLVM_OPT_NONE:
        return "";
    case XVR_LLVM_OPT_LEGACY:
    case XVR_LLVM_OPT_O1:
        return "default<O1>";
    case XVR_LLVM_OPT_O2:
        return "default<O2>";
    case XVR_LLVM_OPT_O3:
        return "default<O3>";
    case XVR_LLVM_OPT_OS:
        return "default<Os>";
    case XVR_LLVM_OPT_OZ:
        return "default<Oz>";
    default:
        return "default<O2>";
    }
}

bool Xvr_LLVMOptimizerRun(Xvr_LLVMOptimizer* opt,
                          Xvr_LLVMModuleManager* module) {
    if (!opt || !module) {
        return false;
    }

    if (opt->level == XVR_LLVM_OPT_NONE) {
        return true;
    }

    LLVMModuleRef llvm_module = Xvr_LLVMModuleManagerGetModule(module);
    if (!llvm_module) {
        return false;
    }

    const char* pipeline = get_pass_pipeline(opt->level);
    if (!pipeline || pipeline[0] == '\0') {
        return true;
    }

    LLVMTargetMachineRef tm = NULL;
    if (opt->target_machine) {
        tm = opt->target_machine;
    } else {
        Xvr_LLVMTargetConfig* config = Xvr_LLVMTargetConfigCreate();
        if (!config) {
            return false;
        }
        Xvr_LLVMTargetConfigSetTriple(config, "x86_64-pc-linux-gnu");
        Xvr_LLVMTargetConfigSetReloc(config, "PIC");
        Xvr_LLVMTargetConfigSetCodeModel(config, "jitdefault");
        Xvr_LLVMTargetMachine* xvr_tm = Xvr_LLVMTargetMachineCreate(config);
        Xvr_LLVMTargetConfigDestroy(config);
        if (!xvr_tm) {
            return false;
        }
        tm = Xvr_LLVMTargetMachineGetLLVMTargetMachine(xvr_tm);
        Xvr_LLVMTargetMachineDestroy(xvr_tm);
    }

    if (!tm) {
        return false;
    }

    LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();
    if (!options) {
        return false;
    }

    LLVMPassBuilderOptionsSetVerifyEach(options, 0);
    LLVMPassBuilderOptionsSetDebugLogging(options, 0);
    LLVMPassBuilderOptionsSetLoopInterleaving(options, 1);
    LLVMPassBuilderOptionsSetLoopVectorization(options, 0);
    LLVMPassBuilderOptionsSetSLPVectorization(options, 0);
    LLVMPassBuilderOptionsSetLoopUnrolling(options, 1);
    LLVMPassBuilderOptionsSetMergeFunctions(options, 0);
    LLVMPassBuilderOptionsSetCallGraphProfile(options, 0);
    LLVMPassBuilderOptionsSetInlinerThreshold(options, 225);

    LLVMErrorRef err = LLVMRunPasses(llvm_module, pipeline, tm, options);

    LLVMDisposePassBuilderOptions(options);

    if (err) {
        char* err_msg = LLVMGetErrorMessage(err);
        LLVMConsumeError(err);
        if (err_msg) {
            free(err_msg);
        }
        return false;
    }

    return true;
}

bool Xvr_LLVMOptimizerAddPass(Xvr_LLVMOptimizer* opt, const char* pass_name) {
    (void)opt;
    (void)pass_name;
    return false;
}

bool Xvr_LLVMOptimizerAddStandardPasses(Xvr_LLVMOptimizer* opt) {
    if (!opt) {
        return false;
    }
    return true;
}
