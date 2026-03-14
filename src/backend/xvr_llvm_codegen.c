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

#include <dlfcn.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
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

    Xvr_LLVMExpressionEmitterSetControlFlow(codegen->expr_emitter,
                                            codegen->control_flow);

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

    Xvr_LLVMTargetConfig* target_config = Xvr_LLVMTargetConfigCreate();
    if (!target_config) {
        Xvr_LLVMOptimizerDestroy(codegen->optimizer);
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
    Xvr_LLVMTargetConfigSetReloc(target_config, "PIC");
    Xvr_LLVMTargetConfigSetCodeModel(target_config, "jitdefault");

    codegen->target_machine = Xvr_LLVMTargetMachineCreate(target_config);
    if (!codegen->target_machine) {
        Xvr_LLVMTargetConfigDestroy(target_config);
        Xvr_LLVMOptimizerDestroy(codegen->optimizer);
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

static bool emit_main_function(Xvr_LLVMCodegen* codegen, Xvr_ASTNode* stmt);
static bool ensure_main_function(Xvr_LLVMCodegen* codegen);
static void finalize_main_function(Xvr_LLVMCodegen* codegen);

static bool ensure_main_function(Xvr_LLVMCodegen* codegen) {
    static bool main_created = false;
    if (main_created) {
        return true;
    }

    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(codegen->context);
    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(codegen->module);
    LLVMBuilderRef builder = Xvr_LLVMIRBuilderGetLLVMBuilder(codegen->builder);

    LLVMTypeRef int32_type = LLVMInt32TypeInContext(llvm_ctx);
    LLVMTypeRef main_fn_type = LLVMFunctionType(int32_type, NULL, 0, false);
    LLVMAddFunction(module, "main", main_fn_type);

    LLVMValueRef main_fn = LLVMGetNamedFunction(module, "main");
    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(llvm_ctx, main_fn, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    Xvr_LLVMFunctionEmitterSetCurrentFunction(codegen->fn_emitter, main_fn);

    main_created = true;
    return true;
}

static void finalize_main_function(Xvr_LLVMCodegen* codegen) {
    static bool finalized = false;
    if (finalized) {
        return;
    }

    LLVMBuilderRef builder = Xvr_LLVMIRBuilderGetLLVMBuilder(codegen->builder);
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(codegen->context);
    LLVMTypeRef int32_type = LLVMInt32TypeInContext(llvm_ctx);

    LLVMBuildRet(builder, LLVMConstInt(int32_type, 0, false));
    finalized = true;
}

static bool emit_main_function(Xvr_LLVMCodegen* codegen, Xvr_ASTNode* stmt) {
    ensure_main_function(codegen);

    if (stmt->type == XVR_AST_NODE_VAR_DECL) {
        Xvr_NodeVarDecl* varDecl = &stmt->varDecl;
        const char* var_name = NULL;
        if (varDecl->identifier.type == XVR_LITERAL_IDENTIFIER &&
            varDecl->identifier.as.string.ptr) {
            var_name = (const char*)varDecl->identifier.as.string.ptr->data;
        }

        if (var_name && varDecl->expression) {
            Xvr_LLVMExpressionEmitter* expr_emitter = codegen->expr_emitter;
            LLVMValueRef init_value = Xvr_LLVMExpressionEmitterEmit(
                expr_emitter, varDecl->expression);
            if (init_value) {
                LLVMTypeRef var_type = LLVMTypeOf(init_value);
                LLVMValueRef alloca = Xvr_LLVMIRBuilderCreateAlloca(
                    codegen->builder, var_type, var_name);
                Xvr_LLVMIRBuilderCreateStore(codegen->builder, init_value,
                                             alloca);
                Xvr_LiteralType varType = XVR_LITERAL_INTEGER;
                if (varDecl->typeLiteral.type == XVR_LITERAL_TYPE &&
                    XVR_AS_TYPE(varDecl->typeLiteral).typeOf !=
                        XVR_LITERAL_TYPE &&
                    XVR_AS_TYPE(varDecl->typeLiteral).typeOf !=
                        XVR_LITERAL_ANY) {
                    varType = XVR_AS_TYPE(varDecl->typeLiteral).typeOf;
                } else {
                    LLVMTypeKind kind = LLVMGetTypeKind(var_type);
                    if (kind == LLVMPointerTypeKind) {
                        varType = XVR_LITERAL_STRING;
                    } else if (kind == LLVMFloatTypeKind) {
                        varType = XVR_LITERAL_FLOAT;
                    } else if (kind == LLVMDoubleTypeKind) {
                        varType = XVR_LITERAL_FLOAT64;
                    } else if (kind == LLVMArrayTypeKind) {
                        varType = XVR_LITERAL_ARRAY;
                    } else if (kind == LLVMIntegerTypeKind) {
                        unsigned int bits = LLVMGetIntTypeWidth(var_type);
                        if (bits == 1) {
                            varType = XVR_LITERAL_BOOLEAN;
                        } else if (bits == 8) {
                            varType = XVR_LITERAL_INT8;
                        } else if (bits == 16) {
                            varType = XVR_LITERAL_INT16;
                        } else if (bits == 32) {
                            varType = XVR_LITERAL_INTEGER;
                        } else if (bits == 64) {
                            varType = XVR_LITERAL_INT64;
                        }
                    }
                }
                Xvr_LLVMFunctionEmitterAddLocalVar(
                    codegen->fn_emitter, var_name, alloca, varType, 0);
            }
        }
        return true;
    }

    Xvr_LLVMExpressionEmitterEmit(codegen->expr_emitter, stmt);

    return true;
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

    return emit_main_function(codegen, ast);
}

char* Xvr_LLVMCodegenPrintIR(Xvr_LLVMCodegen* codegen, size_t* out_len) {
    if (!codegen || !out_len) {
        return NULL;
    }
    finalize_main_function(codegen);
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
    if (!codegen || !filepath) {
        return false;
    }
    if (!codegen->target_machine) {
        return false;
    }
    finalize_main_function(codegen);
    return Xvr_LLVMTargetMachineEmitToFile(codegen->target_machine,
                                           codegen->module, filepath, 0);
}

bool Xvr_LLVMCodegenExecuteJIT(Xvr_LLVMCodegen* codegen) {
    if (!codegen) {
        return false;
    }

    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(codegen->module);
    if (!module) {
        return false;
    }

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
