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

#include "xvr_llvm_function_emitter.h"

#include <stdlib.h>
#include <string.h>

#include "xvr_ast_node.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_expression_emitter.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_type_mapper.h"

struct Xvr_LLVMFunctionEmitter {
    Xvr_LLVMContext* context;
    Xvr_LLVMModuleManager* module;
    Xvr_LLVMIRBuilder* builder;
    Xvr_LLVMTypeMapper* type_mapper;
    Xvr_LLVMExpressionEmitter* expr_emitter;
};

Xvr_LLVMFunctionEmitter* Xvr_LLVMFunctionEmitterCreate(
    Xvr_LLVMContext* ctx, Xvr_LLVMModuleManager* module,
    Xvr_LLVMIRBuilder* builder, Xvr_LLVMTypeMapper* type_mapper,
    Xvr_LLVMExpressionEmitter* expr_emitter) {
    if (!ctx || !module || !builder || !type_mapper || !expr_emitter) {
        return NULL;
    }

    Xvr_LLVMFunctionEmitter* emitter =
        calloc(1, sizeof(Xvr_LLVMFunctionEmitter));
    if (!emitter) {
        return NULL;
    }

    emitter->context = ctx;
    emitter->module = module;
    emitter->builder = builder;
    emitter->type_mapper = type_mapper;
    emitter->expr_emitter = expr_emitter;

    return emitter;
}

void Xvr_LLVMFunctionEmitterDestroy(Xvr_LLVMFunctionEmitter* emitter) {
    free(emitter);
}

static bool emit_function_body(Xvr_LLVMFunctionEmitter* emitter,
                               Xvr_NodeFnDecl* fn_decl) {
    Xvr_LLVMIRBuilder* builder = emitter->builder;
    Xvr_LLVMExpressionEmitter* expr_emitter = emitter->expr_emitter;
    Xvr_LLVMModuleManager* module = emitter->module;
    Xvr_LLVMContext* context = emitter->context;

    if (!fn_decl->block) {
        return false;
    }

    Xvr_NodeBlock* block = (Xvr_NodeBlock*)&fn_decl->block->block;

    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(context);
    LLVMTypeRef return_type = LLVMInt32TypeInContext(llvm_ctx);
    LLVMTypeRef function_type = LLVMFunctionType(return_type, NULL, 0, false);

    const char* fn_name = "xvr_fn";
    if (fn_decl->identifier.as.string.ptr) {
        fn_name = (const char*)fn_decl->identifier.as.string.ptr->data;
    }

    LLVMValueRef function = LLVMAddFunction(
        Xvr_LLVMModuleManagerGetModule(module), fn_name, function_type);

    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(function, "entry");
    Xvr_LLVMIRBuilderSetInsertPoint(builder, entry_block);

    for (int i = 0; i < block->count; i++) {
        Xvr_ASTNode* stmt = &block->nodes[i];
        if (stmt->type == XVR_AST_NODE_FN_RETURN) {
            Xvr_NodeFnReturn* ret = &stmt->returns;
            if (ret->returns) {
                Xvr_LLVMExpressionEmitterEmit(expr_emitter, ret->returns);
            }
        }
    }

    LLVMValueRef zero =
        LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, false);
    LLVMBuildRet(Xvr_LLVMIRBuilderGetLLVMBuilder(builder), zero);

    return true;
}

bool Xvr_LLVMFunctionEmitterEmit(Xvr_LLVMFunctionEmitter* emitter,
                                 Xvr_ASTNode* fn_decl) {
    if (!emitter || !fn_decl) {
        return false;
    }

    if (fn_decl->type != XVR_AST_NODE_FN_DECL) {
        return false;
    }

    Xvr_NodeFnDecl* decl = &fn_decl->fnDecl;
    return emit_function_body(emitter, decl);
}

bool Xvr_LLVMFunctionEmitterEmitCollection(Xvr_LLVMFunctionEmitter* emitter,
                                           Xvr_ASTNode* fn_collection) {
    if (!emitter || !fn_collection) {
        return false;
    }

    if (fn_collection->type != XVR_AST_NODE_FN_COLLECTION) {
        return false;
    }

    Xvr_NodeFnCollection* collection = &fn_collection->fnCollection;

    for (int i = 0; i < collection->count; i++) {
        Xvr_ASTNode* fn_node = &collection->nodes[i];
        if (!Xvr_LLVMFunctionEmitterEmit(emitter, fn_node)) {
            return false;
        }
    }

    return true;
}
