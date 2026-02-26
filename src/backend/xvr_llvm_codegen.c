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

#include "xvr_llvm_codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* Xvr_private_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

#include "xvr_ast_node.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_control_flow.h"
#include "xvr_llvm_expression_emitter.h"
#include "xvr_llvm_function_emitter.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_optimizer.h"
#include "xvr_llvm_target.h"
#include "xvr_llvm_type_mapper.h"

struct Xvr_LLVMCodegen {
    Xvr_LLVMContext* context;
    Xvr_LLVMModuleManager* module;
    Xvr_LLVMIRBuilder* builder;
    Xvr_LLVMTypeMapper* type_mapper;
    Xvr_LLVMExpressionEmitter* expr_emitter;
    Xvr_LLVMFunctionEmitter* fn_emitter;
    Xvr_LLVMControlFlow* control_flow;
    Xvr_LLVMOptimizer* optimizer;
    Xvr_LLVMTargetMachine* target_machine;

    bool has_error;
    char* error_message;
};

Xvr_LLVMCodegen* Xvr_LLVMCodegenCreate(const char* module_name) {
    if (!module_name) {
        return NULL;
    }

    Xvr_LLVMCodegen* codegen = calloc(1, sizeof(Xvr_LLVMCodegen));
    if (!codegen) {
        return NULL;
    }

    codegen->context = Xvr_LLVMContextCreate();
    if (!codegen->context) {
        free(codegen);
        return NULL;
    }

    codegen->module =
        Xvr_LLVMModuleManagerCreate(codegen->context, module_name);
    if (!codegen->module) {
        Xvr_LLVMContextDestroy(codegen->context);
        free(codegen);
        return NULL;
    }

    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(codegen->module);
    codegen->builder = Xvr_LLVMIRBuilderCreate(codegen->context, module);
    if (!codegen->builder) {
        Xvr_LLVMModuleManagerDestroy(codegen->module);
        Xvr_LLVMContextDestroy(codegen->context);
        free(codegen);
        return NULL;
    }

    codegen->type_mapper = Xvr_LLVMTypeMapperCreate(codegen->context);
    if (!codegen->type_mapper) {
        Xvr_LLVMIRBuilderDestroy(codegen->builder);
        Xvr_LLVMModuleManagerDestroy(codegen->module);
        Xvr_LLVMContextDestroy(codegen->context);
        free(codegen);
        return NULL;
    }

    codegen->expr_emitter =
        Xvr_LLVMExpressionEmitterCreate(codegen->context, codegen->module,
                                        codegen->builder, codegen->type_mapper);
    if (!codegen->expr_emitter) {
        Xvr_LLVMTypeMapperDestroy(codegen->type_mapper);
        Xvr_LLVMIRBuilderDestroy(codegen->builder);
        Xvr_LLVMModuleManagerDestroy(codegen->module);
        Xvr_LLVMContextDestroy(codegen->context);
        free(codegen);
        return NULL;
    }

    codegen->fn_emitter = Xvr_LLVMFunctionEmitterCreate(
        codegen->context, codegen->module, codegen->builder,
        codegen->type_mapper, codegen->expr_emitter);
    if (!codegen->fn_emitter) {
        Xvr_LLVMExpressionEmitterDestroy(codegen->expr_emitter);
        Xvr_LLVMTypeMapperDestroy(codegen->type_mapper);
        Xvr_LLVMIRBuilderDestroy(codegen->builder);
        Xvr_LLVMModuleManagerDestroy(codegen->module);
        Xvr_LLVMContextDestroy(codegen->context);
        free(codegen);
        return NULL;
    }

    Xvr_LLVMExpressionEmitterSetFnEmitter(codegen->expr_emitter,
                                          codegen->fn_emitter);

    codegen->control_flow = Xvr_LLVMControlFlowCreate(
        codegen->context, codegen->module, codegen->builder,
        codegen->type_mapper, codegen->expr_emitter);
    if (!codegen->control_flow) {
        Xvr_LLVMFunctionEmitterDestroy(codegen->fn_emitter);
        Xvr_LLVMExpressionEmitterDestroy(codegen->expr_emitter);
        Xvr_LLVMTypeMapperDestroy(codegen->type_mapper);
        Xvr_LLVMIRBuilderDestroy(codegen->builder);
        Xvr_LLVMModuleManagerDestroy(codegen->module);
        Xvr_LLVMContextDestroy(codegen->context);
        free(codegen);
        return NULL;
    }

    codegen->optimizer = Xvr_LLVMOptimizerCreate();
    if (!codegen->optimizer) {
        Xvr_LLVMControlFlowDestroy(codegen->control_flow);
        Xvr_LLVMFunctionEmitterDestroy(codegen->fn_emitter);
        Xvr_LLVMExpressionEmitterDestroy(codegen->expr_emitter);
        Xvr_LLVMTypeMapperDestroy(codegen->type_mapper);
        Xvr_LLVMIRBuilderDestroy(codegen->builder);
        Xvr_LLVMModuleManagerDestroy(codegen->module);
        Xvr_LLVMContextDestroy(codegen->context);
        free(codegen);
        return NULL;
    }

    Xvr_LLVMOptimizerSetLevel(codegen->optimizer, XVR_LLVM_OPT_O2);

    codegen->has_error = false;
    codegen->error_message = NULL;

    return codegen;
}

void Xvr_LLVMCodegenDestroy(Xvr_LLVMCodegen* codegen) {
    if (!codegen) {
        return;
    }

    if (codegen->target_machine) {
        Xvr_LLVMTargetMachineDestroy(codegen->target_machine);
    }
    if (codegen->optimizer) {
        Xvr_LLVMOptimizerDestroy(codegen->optimizer);
    }
    if (codegen->control_flow) {
        Xvr_LLVMControlFlowDestroy(codegen->control_flow);
    }
    if (codegen->fn_emitter) {
        Xvr_LLVMFunctionEmitterDestroy(codegen->fn_emitter);
    }
    if (codegen->expr_emitter) {
        Xvr_LLVMExpressionEmitterDestroy(codegen->expr_emitter);
    }
    if (codegen->type_mapper) {
        Xvr_LLVMTypeMapperDestroy(codegen->type_mapper);
    }
    if (codegen->builder) {
        Xvr_LLVMIRBuilderDestroy(codegen->builder);
    }
    if (codegen->module) {
        Xvr_LLVMModuleManagerDestroy(codegen->module);
    }
    if (codegen->context) {
        Xvr_LLVMContextDestroy(codegen->context);
    }

    free(codegen->error_message);
    free(codegen);
}

bool Xvr_LLVMCodegenSetOptimizationLevel(Xvr_LLVMCodegen* codegen,
                                         Xvr_LLVMOptimizationLevel level) {
    if (!codegen || !codegen->optimizer) {
        return false;
    }
    return Xvr_LLVMOptimizerSetLevel(codegen->optimizer, level);
}

bool Xvr_LLVMCodegenSetTargetTriple(Xvr_LLVMCodegen* codegen,
                                    const char* triple) {
    (void)codegen;
    (void)triple;
    return true;
}

bool Xvr_LLVMCodegenSetTargetCPU(Xvr_LLVMCodegen* codegen, const char* cpu) {
    (void)codegen;
    (void)cpu;
    return true;
}

static void set_error(Xvr_LLVMCodegen* codegen, const char* message) {
    if (!codegen) {
        return;
    }
    free(codegen->error_message);
    codegen->error_message = message ? Xvr_private_strdup(message) : NULL;
    codegen->has_error = (codegen->error_message != NULL);
}

bool Xvr_LLVMCodegenEmitAST(Xvr_LLVMCodegen* codegen, Xvr_ASTNode* ast) {
    if (!codegen || !ast) {
        return false;
    }

    if (Xvr_LLVMContextHasError(codegen->context)) {
        set_error(codegen, Xvr_LLVMContextGetErrorMessage(codegen->context));
        return false;
    }

    if (ast->type == XVR_AST_NODE_FN_COLLECTION) {
        return Xvr_LLVMFunctionEmitterEmitCollection(codegen->fn_emitter, ast);
    }

    if (ast->type == XVR_AST_NODE_FN_DECL) {
        return Xvr_LLVMFunctionEmitterEmit(codegen->fn_emitter, ast);
    }

    return false;
}

char* Xvr_LLVMCodegenPrintIR(Xvr_LLVMCodegen* codegen, size_t* out_len) {
    if (!codegen || !out_len) {
        return NULL;
    }
    return Xvr_LLVMModuleManagerPrintIR(codegen->module, out_len);
}

bool Xvr_LLVMCodegenWriteBitcode(Xvr_LLVMCodegen* codegen,
                                 const char* filepath) {
    if (!codegen || !filepath) {
        return false;
    }
    return Xvr_LLVMModuleManagerWriteBitcode(codegen->module, filepath);
}

bool Xvr_LLVMCodegenWriteObjectFile(Xvr_LLVMCodegen* codegen,
                                    const char* filepath) {
    (void)codegen;
    (void)filepath;
    return false;
}

bool Xvr_LLVMCodegenExecuteJIT(Xvr_LLVMCodegen* codegen) {
    (void)codegen;
    return false;
}

bool Xvr_LLVMCodegenHasError(Xvr_LLVMCodegen* codegen) {
    return codegen && codegen->has_error;
}

const char* Xvr_LLVMCodegenGetError(Xvr_LLVMCodegen* codegen) {
    if (!codegen) {
        return NULL;
    }
    return codegen->error_message;
}
