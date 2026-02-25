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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_ast_node.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_expression_emitter.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_type_mapper.h"
#include "xvr_refstring.h"

#define MAX_LOCAL_VARS 64

typedef struct {
    const char* name;
    LLVMValueRef value;
    Xvr_LiteralType type;
} Xvr_LLVMVariable;

struct Xvr_LLVMFunctionEmitter {
    Xvr_LLVMContext* context;
    Xvr_LLVMModuleManager* module;
    Xvr_LLVMIRBuilder* builder;
    Xvr_LLVMTypeMapper* type_mapper;
    Xvr_LLVMExpressionEmitter* expr_emitter;

    Xvr_LLVMVariable local_vars[MAX_LOCAL_VARS];
    int local_var_count;
    LLVMValueRef current_function;
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
    emitter->local_var_count = 0;

    return emitter;
}

void Xvr_LLVMFunctionEmitterDestroy(Xvr_LLVMFunctionEmitter* emitter) {
    free(emitter);
}

static void clear_local_vars(Xvr_LLVMFunctionEmitter* emitter) {
    emitter->local_var_count = 0;
    emitter->current_function = NULL;
}

static void add_local_var(Xvr_LLVMFunctionEmitter* emitter, const char* name,
                          LLVMValueRef value, Xvr_LiteralType type) {
    if (emitter->local_var_count >= MAX_LOCAL_VARS) {
        return;
    }
    emitter->local_vars[emitter->local_var_count].name = name;
    emitter->local_vars[emitter->local_var_count].value = value;
    emitter->local_vars[emitter->local_var_count].type = type;
    emitter->local_var_count++;
}

static LLVMValueRef lookup_local_var(Xvr_LLVMFunctionEmitter* emitter,
                                     const char* name) {
    for (int i = 0; i < emitter->local_var_count; i++) {
        if (emitter->local_vars[i].name &&
            strcmp(emitter->local_vars[i].name, name) == 0) {
            return emitter->local_vars[i].value;
        }
    }
    return NULL;
}

static Xvr_LiteralType get_var_type(Xvr_LLVMFunctionEmitter* emitter,
                                    const char* name) {
    for (int i = 0; i < emitter->local_var_count; i++) {
        if (emitter->local_vars[i].name &&
            strcmp(emitter->local_vars[i].name, name) == 0) {
            return emitter->local_vars[i].type;
        }
    }
    return XVR_LITERAL_ANY;
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

    clear_local_vars(emitter);

    Xvr_NodeBlock* block = (Xvr_NodeBlock*)&fn_decl->block->block;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(context);

    LLVMTypeRef return_type = LLVMInt32TypeInContext(llvm_ctx);

    if (fn_decl->returns &&
        fn_decl->returns->type == XVR_AST_NODE_FN_COLLECTION) {
        Xvr_NodeFnCollection* collection = &fn_decl->returns->fnCollection;
        if (collection->count > 0) {
            Xvr_ASTNode* firstReturn = &collection->nodes[0];
            if (firstReturn && firstReturn->type == XVR_AST_NODE_LITERAL) {
                Xvr_Literal typeLiteral = firstReturn->atomic.literal;
                if (typeLiteral.type == XVR_LITERAL_TYPE) {
                    Xvr_LiteralType declaredType =
                        XVR_AS_TYPE(typeLiteral).typeOf;
                    return_type = Xvr_LLVMTypeMapperGetType(
                        emitter->type_mapper, declaredType);
                    if (!return_type) {
                        return_type = LLVMInt32TypeInContext(llvm_ctx);
                    }
                }
            }
        }
    }

    LLVMTypeRef param_types[32];
    int param_count = 0;
    const char* param_names[32];
    Xvr_LiteralType param_types_xvr[32];

    if (fn_decl->arguments &&
        fn_decl->arguments->type == XVR_AST_NODE_FN_COLLECTION) {
        Xvr_NodeFnCollection* args = &fn_decl->arguments->fnCollection;
        for (int i = 0; i < args->count && i < 32; i++) {
            Xvr_ASTNode* arg = &args->nodes[i];
            if (arg->type == XVR_AST_NODE_VAR_DECL) {
                Xvr_NodeVarDecl* varDecl = &arg->varDecl;
                if (varDecl->identifier.type == XVR_LITERAL_IDENTIFIER &&
                    varDecl->identifier.as.string.ptr) {
                    param_names[param_count] =
                        (const char*)varDecl->identifier.as.string.ptr->data;
                } else {
                    param_names[param_count] = "arg";
                }
                Xvr_LiteralType varType = XVR_LITERAL_INTEGER;
                if (varDecl->typeLiteral.type == XVR_LITERAL_TYPE) {
                    varType = XVR_AS_TYPE(varDecl->typeLiteral).typeOf;
                }
                param_types_xvr[param_count] = varType;
                LLVMTypeRef arg_type =
                    Xvr_LLVMTypeMapperGetType(emitter->type_mapper, varType);
                if (!arg_type) {
                    arg_type = LLVMInt32TypeInContext(llvm_ctx);
                }
                param_types[param_count] = arg_type;
                param_count++;
            }
        }
    }

    LLVMTypeRef function_type =
        LLVMFunctionType(return_type, param_types, param_count, false);

    const char* fn_name = "xvr_fn";
    if (fn_decl->identifier.as.string.ptr) {
        fn_name = (const char*)fn_decl->identifier.as.string.ptr->data;
    }

    LLVMValueRef function = LLVMAddFunction(
        Xvr_LLVMModuleManagerGetModule(module), fn_name, function_type);

    emitter->current_function = function;

    for (int i = 0; i < param_count; i++) {
        LLVMValueRef param = LLVMGetParam(function, i);
        LLVMSetValueName(param, param_names[i]);
        add_local_var(emitter, param_names[i], param, param_types_xvr[i]);
    }

    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(function, "entry");
    Xvr_LLVMIRBuilderSetInsertPoint(builder, entry_block);

    LLVMValueRef return_value = NULL;
    for (int i = 0; i < block->count; i++) {
        Xvr_ASTNode* stmt = &block->nodes[i];

        if (stmt->type == XVR_AST_NODE_FN_RETURN) {
            Xvr_NodeFnReturn* ret = &stmt->returns;
            if (ret->returns) {
                return_value =
                    Xvr_LLVMExpressionEmitterEmit(expr_emitter, ret->returns);
            }
        } else if (stmt->type == XVR_AST_NODE_VAR_DECL) {
            Xvr_NodeVarDecl* varDecl = &stmt->varDecl;
            const char* var_name = NULL;
            if (varDecl->identifier.type == XVR_LITERAL_IDENTIFIER &&
                varDecl->identifier.as.string.ptr) {
                var_name = (const char*)varDecl->identifier.as.string.ptr->data;
            }

            if (var_name && varDecl->expression) {
                LLVMValueRef init_value = Xvr_LLVMExpressionEmitterEmit(
                    expr_emitter, varDecl->expression);
                if (init_value) {
                    LLVMTypeRef var_type = LLVMTypeOf(init_value);
                    LLVMValueRef alloca = Xvr_LLVMIRBuilderCreateAlloca(
                        builder, var_type, var_name);
                    Xvr_LLVMIRBuilderCreateStore(builder, init_value, alloca);
                    Xvr_LiteralType varType = XVR_LITERAL_INTEGER;
                    if (varDecl->typeLiteral.type == XVR_LITERAL_TYPE) {
                        varType = XVR_AS_TYPE(varDecl->typeLiteral).typeOf;
                    }
                    add_local_var(emitter, var_name, alloca, varType);
                }
            }
        } else {
            Xvr_LLVMExpressionEmitterEmit(expr_emitter, stmt);
        }
    }

    if (!return_value) {
        return_value = LLVMConstInt(return_type, 0, false);
    }
    LLVMBuildRet(Xvr_LLVMIRBuilderGetLLVMBuilder(builder), return_value);

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
