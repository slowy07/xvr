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
#include <string.h>

#include "../../sema/xvr_builtin.h"

static const char* literal_type_name(Xvr_LiteralType type) {
    switch (type) {
    case XVR_LITERAL_BOOLEAN:
        return "bool";
    case XVR_LITERAL_INTEGER:
        return "int";
    case XVR_LITERAL_FLOAT:
        return "float";
    case XVR_LITERAL_FLOAT64:
        return "float64";
    case XVR_LITERAL_STRING:
        return "string";
    case XVR_LITERAL_ARRAY:
        return "array";
    case XVR_LITERAL_DICTIONARY:
        return "dict";
    case XVR_LITERAL_FUNCTION:
        return "function";
    case XVR_LITERAL_IDENTIFIER:
        return "identifier";
    case XVR_LITERAL_TYPE:
        return "type";
    case XVR_LITERAL_NULL:
        return "null";
    case XVR_LITERAL_INT8:
        return "int8";
    case XVR_LITERAL_INT16:
        return "int16";
    case XVR_LITERAL_INT32:
        return "int32";
    case XVR_LITERAL_INT64:
        return "int64";
    case XVR_LITERAL_UINT8:
        return "uint8";
    case XVR_LITERAL_UINT16:
        return "uint16";
    case XVR_LITERAL_UINT32:
        return "uint32";
    case XVR_LITERAL_UINT64:
        return "uint64";
    case XVR_LITERAL_FLOAT16:
        return "float16";
    case XVR_LITERAL_FLOAT32:
        return "float32";
    default:
        return "unknown";
    }
}

static bool is_integer_type(Xvr_LiteralType type) {
    return type == XVR_LITERAL_INTEGER || type == XVR_LITERAL_INT8 ||
           type == XVR_LITERAL_INT16 || type == XVR_LITERAL_INT32 ||
           type == XVR_LITERAL_INT64 || type == XVR_LITERAL_UINT8 ||
           type == XVR_LITERAL_UINT16 || type == XVR_LITERAL_UINT32 ||
           type == XVR_LITERAL_UINT64;
}
#include <stdlib.h>
#include <string.h>

#include "xvr_common.h"

static char* Xvr_private_strdup(const char* str) { return Xvr_strdup(str); }

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
    Xvr_ModuleResolver* module_resolver;

    bool has_error;
    char* error_message;
    bool main_created;
};

Xvr_LLVMCodegen* Xvr_LLVMCodegenCreate(const char* module_name) {
    if (!module_name) {
        return NULL;
    }

    Xvr_LLVMCodegen* codegen = (Xvr_LLVMCodegen*) calloc(1, sizeof(Xvr_LLVMCodegen));
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

    codegen->module_resolver = Xvr_ModuleResolverCreate("./lib/std");

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

    if (Xvr_commandLine.asmSyntax) {
        Xvr_AsmSyntax syntax =
            Xvr_AsmSyntaxFromString(Xvr_commandLine.asmSyntax);
        Xvr_LLVMTargetConfigSetAsmSyntax(target_config, syntax);
    }

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
    if (codegen->module_resolver) {
        Xvr_ModuleResolverDestroy(codegen->module_resolver);
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

bool Xvr_LLVMCodegenRunOptimizer(Xvr_LLVMCodegen* codegen) {
    if (!codegen || !codegen->optimizer) {
        return false;
    }

    if (codegen->target_machine) {
        Xvr_LLVMOptimizerSetTargetMachine(
            codegen->optimizer,
            Xvr_LLVMTargetMachineGetLLVMTargetMachine(codegen->target_machine));
    }

    return Xvr_LLVMOptimizerRun(codegen->optimizer, codegen->module);
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
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(codegen->context);
    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(codegen->module);
    LLVMBuilderRef builder = Xvr_LLVMIRBuilderGetLLVMBuilder(codegen->builder);

    LLVMValueRef main_fn = LLVMGetNamedFunction(module, "main");
    if (!main_fn) {
        LLVMTypeRef int32_type = LLVMInt32TypeInContext(llvm_ctx);
        LLVMTypeRef main_fn_type = LLVMFunctionType(int32_type, NULL, 0, false);
        LLVMAddFunction(module, "main", main_fn_type);
        main_fn = LLVMGetNamedFunction(module, "main");

        LLVMBasicBlockRef entry =
            LLVMAppendBasicBlockInContext(llvm_ctx, main_fn, "entry");
        LLVMPositionBuilderAtEnd(builder, entry);
    } else {
        LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(main_fn);
        if (entry) {
            LLVMBasicBlockRef current = LLVMGetInsertBlock(builder);
            if (current && LLVMGetBasicBlockParent(current) == main_fn) {
                return true;
            }
            LLVMValueRef block_parent = LLVMGetBasicBlockParent(entry);
            if (!block_parent) {
                entry =
                    LLVMAppendBasicBlockInContext(llvm_ctx, main_fn, "entry");
            }
            LLVMPositionBuilderAtEnd(builder, entry);
        } else {
            LLVMBasicBlockRef new_entry =
                LLVMAppendBasicBlockInContext(llvm_ctx, main_fn, "entry");
            LLVMPositionBuilderAtEnd(builder, new_entry);
        }
    }

    Xvr_LLVMFunctionEmitterSetCurrentFunction(codegen->fn_emitter, main_fn);

    codegen->main_created = true;
    return true;
}

static void finalize_main_function(Xvr_LLVMCodegen* codegen) {
    if (!codegen->main_created) {
        return;
    }

    LLVMBuilderRef builder = Xvr_LLVMIRBuilderGetLLVMBuilder(codegen->builder);
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(codegen->context);
    LLVMTypeRef int32_type = LLVMInt32TypeInContext(llvm_ctx);

    LLVMBuildRet(builder, LLVMConstInt(int32_type, 0, false));
}

static bool emit_main_function(Xvr_LLVMCodegen* codegen, Xvr_ASTNode* stmt) {
    ensure_main_function(codegen);

    Xvr_LLVMExpressionEmitterEmit(codegen->expr_emitter, stmt);

    if (Xvr_LLVMContextHasError(codegen->context)) {
        return false;
    }

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

    if (ast->type == XVR_AST_NODE_IMPORT) {
        Xvr_Literal* ident = &ast->import.identifier;
        if (ident->type == XVR_LITERAL_IDENTIFIER && ident->as.identifier.ptr) {
            const char* module_name = ident->as.identifier.ptr->data;
            char* module_path = NULL;
            if (Xvr_ModuleResolverResolve(codegen->module_resolver, module_name,
                                          &module_path)) {
                Xvr_ASTNode** module_nodes = NULL;
                int module_node_count = 0;
                if (Xvr_ModuleResolverLoadModule(codegen->module_resolver,
                                                 module_path, &module_nodes,
                                                 &module_node_count)) {
                    for (int i = 0; i < module_node_count; i++) {
                        if (!Xvr_LLVMCodegenEmitAST(codegen, module_nodes[i])) {
                            for (int j = i; j < module_node_count; j++) {
                                Xvr_freeASTNode(module_nodes[j]);
                            }
                            free(module_nodes);
                            free(module_path);
                            return false;
                        }
                    }
                    for (int i = 0; i < module_node_count; i++) {
                        Xvr_freeASTNode(module_nodes[i]);
                    }
                    free(module_nodes);
                }
                free(module_path);
            }
        }
        return true;
    }

    if (ast->type == XVR_AST_NODE_FN_COLLECTION) {
        bool result =
            Xvr_LLVMFunctionEmitterEmitCollection(codegen->fn_emitter, ast);
        if (result) {
            ensure_main_function(codegen);
        }
        return result;
    }

    if (ast->type == XVR_AST_NODE_FN_DECL) {
        bool result = Xvr_LLVMFunctionEmitterEmit(codegen->fn_emitter, ast);
        if (result) {
            ensure_main_function(codegen);
        }
        return result;
    }

    if (!emit_main_function(codegen, ast)) {
        if (Xvr_LLVMContextHasError(codegen->context)) {
            set_error(codegen,
                      Xvr_LLVMContextGetErrorMessage(codegen->context));
        }
        return false;
    }

    return true;
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
                                    const char* filepath, int filetype) {
    if (!codegen || !filepath) {
        return false;
    }
    if (!codegen->target_machine) {
        return false;
    }
    finalize_main_function(codegen);
    return Xvr_LLVMTargetMachineEmitToFile(codegen->target_machine,
                                           codegen->module, filepath, filetype);
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
