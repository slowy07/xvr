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

#include "xvr_llvm_module_manager.h"

#include <llvm-c/BitWriter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../xvr_string_utils.h"
#include "xvr_llvm_context.h"

#define MAX_FUNCTIONS 256

typedef struct {
    char name[128];
    LLVMTypeRef type;
} FunctionEntry;

struct Xvr_LLVMModuleManager {
    Xvr_LLVMContext* context;
    LLVMModuleRef module;
    FunctionEntry functions[MAX_FUNCTIONS];
    int function_count;
    void* global_table;
};

Xvr_LLVMModuleManager* Xvr_LLVMModuleManagerCreate(Xvr_LLVMContext* ctx,
                                                   const char* name) {
    if (!ctx) {
        return NULL;
    }

    Xvr_LLVMModuleManager* mgr = calloc(1, sizeof(Xvr_LLVMModuleManager));
    if (!mgr) {
        return NULL;
    }

    mgr->context = ctx;
    const char* module_name = name ? name : "xvr_module";
    mgr->module = LLVMModuleCreateWithNameInContext(
        module_name, Xvr_LLVMContextGetLLVMContext(ctx));

    return mgr;
}

void Xvr_LLVMModuleManagerDestroy(Xvr_LLVMModuleManager* mgr) {
    if (!mgr) {
        return;
    }
    if (mgr->module) {
        LLVMDisposeModule(mgr->module);
    }
    free(mgr);
}

LLVMModuleRef Xvr_LLVMModuleManagerGetModule(Xvr_LLVMModuleManager* mgr) {
    if (!mgr) {
        return NULL;
    }
    return mgr->module;
}

bool Xvr_LLVMModuleManagerAddFunction(Xvr_LLVMModuleManager* mgr,
                                      const char* name,
                                      LLVMTypeRef function_type) {
    if (!mgr || !name || !function_type) {
        return false;
    }

    LLVMValueRef fn = LLVMGetNamedFunction(mgr->module, name);
    if (fn) {
        return false;
    }

    fn = LLVMAddFunction(mgr->module, name, function_type);
    if (fn && mgr->function_count < MAX_FUNCTIONS) {
        size_t name_len = xvr_safe_strlen(name, 128);
        size_t copy_len = (name_len < 128) ? name_len : 127;
        for (size_t i = 0; i < copy_len; i++) {
            mgr->functions[mgr->function_count].name[i] = name[i];
        }
        mgr->functions[mgr->function_count].name[127] = '\0';
        mgr->functions[mgr->function_count].type = function_type;
        mgr->function_count++;
    }
    return (fn != NULL);
}

void Xvr_LLVMModuleManagerRegisterFunctionType(Xvr_LLVMModuleManager* mgr,
                                               const char* name,
                                               LLVMTypeRef function_type) {
    if (!mgr || !name || !function_type) {
        return;
    }
    if (mgr->function_count < MAX_FUNCTIONS) {
        for (int i = 0; i < mgr->function_count; i++) {
            if (strcmp(mgr->functions[i].name, name) == 0) {
                mgr->functions[i].type = function_type;
                return;
            }
        }
        size_t name_len = xvr_safe_strlen(name, 128);
        size_t copy_len = (name_len < 128) ? name_len : 127;
        for (size_t i = 0; i < copy_len; i++) {
            mgr->functions[mgr->function_count].name[i] = name[i];
        }
        mgr->functions[mgr->function_count].name[127] = '\0';
        mgr->functions[mgr->function_count].type = function_type;
        mgr->function_count++;
    }
}

LLVMValueRef Xvr_LLVMModuleManagerGetFunction(Xvr_LLVMModuleManager* mgr,
                                              const char* name) {
    if (!mgr || !name) {
        return NULL;
    }
    return LLVMGetNamedFunction(mgr->module, name);
}

LLVMTypeRef Xvr_LLVMModuleManagerGetFunctionType(Xvr_LLVMModuleManager* mgr,
                                                 const char* name) {
    if (!mgr || !name) {
        return NULL;
    }
    for (int i = 0; i < mgr->function_count; i++) {
        if (strcmp(mgr->functions[i].name, name) == 0) {
            return mgr->functions[i].type;
        }
    }
    return NULL;
}

bool Xvr_LLVMModuleManagerAddGlobal(Xvr_LLVMModuleManager* mgr,
                                    const char* name, LLVMTypeRef type) {
    if (!mgr || !name || !type) {
        return false;
    }

    LLVMValueRef global = LLVMGetNamedGlobal(mgr->module, name);
    if (global) {
        return false;
    }

    global = LLVMAddGlobal(mgr->module, type, name);
    return (global != NULL);
}

LLVMValueRef Xvr_LLVMModuleManagerGetGlobal(Xvr_LLVMModuleManager* mgr,
                                            const char* name) {
    if (!mgr || !name) {
        return NULL;
    }
    return LLVMGetNamedGlobal(mgr->module, name);
}

char* Xvr_LLVMModuleManagerPrintIR(Xvr_LLVMModuleManager* mgr,
                                   size_t* out_len) {
    if (!mgr || !out_len) {
        return NULL;
    }

    char* ir = LLVMPrintModuleToString(mgr->module);
    if (!ir) {
        return NULL;
    }

    size_t ir_len = xvr_safe_strlen(ir, 4096);
    *out_len = ir_len + 1;
    char* result = malloc(*out_len);
    if (result && ir_len > 0 && ir_len < *out_len) {
        for (size_t i = 0; i < ir_len; i++) {
            result[i] = ir[i];
        }
        result[ir_len] = '\0';
    } else {
        free(result);
        result = NULL;
    }
    LLVMDisposeMessage(ir);
    return result;
}

bool Xvr_LLVMModuleManagerWriteBitcode(Xvr_LLVMModuleManager* mgr,
                                       const char* filepath) {
    if (!mgr || !filepath) {
        return false;
    }

    LLVMBool result = LLVMWriteBitcodeToFile(mgr->module, filepath);
    return (result == 0);
}

bool Xvr_LLVMModuleManagerWriteObjectFile(Xvr_LLVMModuleManager* mgr,
                                          const char* filepath) {
    (void)mgr;
    (void)filepath;
    return false;
}
