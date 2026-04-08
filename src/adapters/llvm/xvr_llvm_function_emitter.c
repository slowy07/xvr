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
    int array_count;  // 0 if not an array
} Xvr_LLVMVariable;

struct Xvr_LLVMFunctionEmitter {
    Xvr_LLVMContext* context;
    Xvr_LLVMModuleManager* module;
    Xvr_LLVMIRBuilder* builder;
    Xvr_LLVMTypeMapper* type_mapper;
    Xvr_LLVMExpressionEmitter* expr_emitter;

    Xvr_LLVMVariable local_vars[MAX_LOCAL_VARS];
    int local_var_count;
    int scope_stack[32];  // Track var count at each scope level
    int scope_depth;
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
                          LLVMValueRef value, Xvr_LiteralType type,
                          int array_count) {
    if (emitter->local_var_count >= MAX_LOCAL_VARS) {
        return;
    }
    emitter->local_vars[emitter->local_var_count].name = name;
    emitter->local_vars[emitter->local_var_count].value = value;
    emitter->local_vars[emitter->local_var_count].type = type;
    emitter->local_vars[emitter->local_var_count].array_count = array_count;
    emitter->local_var_count++;
}

static LLVMValueRef lookup_local_var(Xvr_LLVMFunctionEmitter* emitter,
                                     const char* name) {
    /* Search backwards to find the most recent variable with this name (handles
     * shadowing) */
    for (int i = emitter->local_var_count - 1; i >= 0; i--) {
        if (emitter->local_vars[i].name &&
            strcmp(emitter->local_vars[i].name, name) == 0) {
            return emitter->local_vars[i].value;
        }
    }
    return NULL;
}

LLVMValueRef Xvr_LLVMFunctionEmitterLookupVar(Xvr_LLVMFunctionEmitter* emitter,
                                              const char* name) {
    return lookup_local_var(emitter, name);
}

LLVMValueRef Xvr_LLVMFunctionEmitterLookupVarWithType(
    Xvr_LLVMFunctionEmitter* emitter, const char* name,
    Xvr_LiteralType* out_type) {
    if (!emitter || !name || !out_type) {
        return NULL;
    }

    for (int i = emitter->local_var_count - 1; i >= 0; i--) {
        if (emitter->local_vars[i].name &&
            strcmp(emitter->local_vars[i].name, name) == 0) {
            *out_type = emitter->local_vars[i].type;
            return emitter->local_vars[i].value;
        }
    }
    return NULL;
}

int Xvr_LLVMFunctionEmitterLookupVarArrayCount(Xvr_LLVMFunctionEmitter* emitter,
                                               const char* name) {
    if (!emitter || !name) {
        return 0;
    }

    for (int i = emitter->local_var_count - 1; i >= 0; i--) {
        if (emitter->local_vars[i].name &&
            strcmp(emitter->local_vars[i].name, name) == 0) {
            return emitter->local_vars[i].array_count;
        }
    }
    return 0;
}

LLVMValueRef Xvr_LLVMFunctionEmitterGetCurrentFunction(
    Xvr_LLVMFunctionEmitter* emitter) {
    if (!emitter) {
        return NULL;
    }
    return emitter->current_function;
}

void Xvr_LLVMFunctionEmitterSetCurrentFunction(Xvr_LLVMFunctionEmitter* emitter,
                                               LLVMValueRef function) {
    if (!emitter) {
        return;
    }
    emitter->current_function = function;
}

void Xvr_LLVMFunctionEmitterAddLocalVar(Xvr_LLVMFunctionEmitter* emitter,
                                        const char* name, LLVMValueRef alloca,
                                        Xvr_LiteralType type, int array_count) {
    if (!emitter || !name || !alloca) {
        return;
    }
    add_local_var(emitter, name, alloca, type, array_count);
}

void Xvr_LLVMFunctionEmitterEnterScope(Xvr_LLVMFunctionEmitter* emitter) {
    if (!emitter) return;
    if (emitter->scope_depth < 32) {
        emitter->scope_stack[emitter->scope_depth] = emitter->local_var_count;
        emitter->scope_depth++;
    }
}

void Xvr_LLVMFunctionEmitterExitScope(Xvr_LLVMFunctionEmitter* emitter) {
    if (!emitter || emitter->scope_depth <= 0) return;
    emitter->scope_depth--;
    emitter->local_var_count = emitter->scope_stack[emitter->scope_depth];
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

static const char* literal_type_name(Xvr_LiteralType type) {
    switch (type) {
    case XVR_LITERAL_INTEGER:
        return "int";
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
    case XVR_LITERAL_FLOAT:
        return "float";
    case XVR_LITERAL_FLOAT16:
        return "float16";
    case XVR_LITERAL_FLOAT32:
        return "float32";
    case XVR_LITERAL_FLOAT64:
        return "float64";
    case XVR_LITERAL_BOOLEAN:
        return "bool";
    case XVR_LITERAL_STRING:
        return "string";
    case XVR_LITERAL_ARRAY:
        return "array";
    case XVR_LITERAL_DICTIONARY:
        return "dict";
    case XVR_LITERAL_VOID:
        return "void";
    case XVR_LITERAL_ANY:
        return "any";
    default:
        return "unknown";
    }
}

static bool types_match(LLVMTypeRef expected, LLVMTypeRef actual) {
    if (!expected || !actual) {
        return false;
    }
    LLVMTypeKind expected_kind = LLVMGetTypeKind(expected);
    LLVMTypeKind actual_kind = LLVMGetTypeKind(actual);
    return expected_kind == actual_kind;
}

static bool check_return_type_compatibility(Xvr_LLVMFunctionEmitter* emitter,
                                            LLVMTypeRef expected_type,
                                            LLVMValueRef return_value,
                                            const char* fn_name) {
    if (!return_value) {
        return true;
    }

    LLVMTypeRef actual_type = LLVMTypeOf(return_value);
    LLVMTypeKind expected_kind = LLVMGetTypeKind(expected_type);
    LLVMTypeKind actual_kind = LLVMGetTypeKind(actual_type);

    if (expected_kind == LLVMVoidTypeKind) {
        return true;
    }

    if (expected_kind != actual_kind) {
        char error_msg[512];
        const char* expected_name =
            LLVMGetTypeKind(expected_type) == LLVMIntegerTypeKind
                ? "int"
                : (LLVMGetTypeKind(expected_type) == LLVMFloatTypeKind
                       ? "float"
                       : (LLVMGetTypeKind(expected_type) == LLVMPointerTypeKind
                              ? "ptr"
                              : "void"));
        const char* actual_name =
            LLVMGetTypeKind(actual_type) == LLVMIntegerTypeKind
                ? "int"
                : (LLVMGetTypeKind(actual_type) == LLVMFloatTypeKind
                       ? "float"
                       : (LLVMGetTypeKind(actual_type) == LLVMPointerTypeKind
                              ? "ptr"
                              : "void"));
        snprintf(error_msg, sizeof(error_msg),
                 "function '%s': return type mismatch: expected '%s', got '%s'",
                 fn_name ? fn_name : "unknown", expected_name, actual_name);
        Xvr_LLVMContextSetError(emitter->context, error_msg);
        return false;
    }

    return true;
}

static bool emit_function_body(Xvr_LLVMFunctionEmitter* emitter,
                               Xvr_NodeFnDecl* fn_decl) {
    Xvr_LLVMIRBuilder* builder = emitter->builder;
    Xvr_LLVMExpressionEmitter* expr_emitter = emitter->expr_emitter;
    Xvr_LLVMModuleManager* module = emitter->module;
    Xvr_LLVMContext* context = emitter->context;

    if (!fn_decl->block) {
        Xvr_LLVMContextSetError(context, "function has no body");
        return false;
    }

    clear_local_vars(emitter);

    Xvr_NodeBlock* block = (Xvr_NodeBlock*)&fn_decl->block->block;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(context);

    LLVMTypeRef return_type = LLVMInt32TypeInContext(llvm_ctx);
    bool is_void_function = false;
    bool has_explicit_return = false;
    Xvr_LiteralType declared_return_type = XVR_LITERAL_INTEGER;

    if (fn_decl->returns &&
        fn_decl->returns->type == XVR_AST_NODE_FN_COLLECTION) {
        Xvr_NodeFnCollection* collection = &fn_decl->returns->fnCollection;
        if (collection->count > 0) {
            Xvr_ASTNode* firstReturn = &collection->nodes[0];
            if (firstReturn && firstReturn->type == XVR_AST_NODE_LITERAL) {
                Xvr_Literal typeLiteral = firstReturn->atomic.literal;
                if (typeLiteral.type == XVR_LITERAL_TYPE) {
                    declared_return_type = XVR_AS_TYPE(typeLiteral).typeOf;
                    if (declared_return_type == XVR_LITERAL_VOID) {
                        is_void_function = true;
                        return_type = LLVMVoidTypeInContext(llvm_ctx);
                    } else if (declared_return_type == XVR_LITERAL_ANY) {
                        return_type = LLVMInt32TypeInContext(llvm_ctx);
                    } else {
                        return_type = Xvr_LLVMTypeMapperGetType(
                            emitter->type_mapper, declared_return_type);
                        if (!return_type) {
                            char error_msg[256];
                            snprintf(error_msg, sizeof(error_msg),
                                     "unsupported return type: %s",
                                     literal_type_name(declared_return_type));
                            Xvr_LLVMContextSetError(context, error_msg);
                            return false;
                        }
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
                Xvr_LiteralType varType = XVR_LITERAL_ANY;
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

    Xvr_LLVMModuleManagerRegisterFunctionType(module, fn_name, function_type);

    emitter->current_function = function;

    LLVMBasicBlockRef entry_block =
        LLVMAppendBasicBlockInContext(llvm_ctx, function, "entry");
    Xvr_LLVMIRBuilderSetInsertPoint(builder, entry_block);

    for (int i = 0; i < param_count; i++) {
        LLVMValueRef param = LLVMGetParam(function, i);
        LLVMSetValueName(param, param_names[i]);

        LLVMTypeRef param_type = LLVMTypeOf(param);
        LLVMValueRef alloca =
            Xvr_LLVMIRBuilderCreateAlloca(builder, param_type, param_names[i]);
        Xvr_LLVMIRBuilderCreateStore(builder, param, alloca);

        add_local_var(emitter, param_names[i], alloca, param_types_xvr[i], 0);
    }

    LLVMValueRef return_value = NULL;
    bool found_return_in_block = false;
    int last_expr_index = block->count - 1;

    for (int i = 0; i < block->count; i++) {
        Xvr_ASTNode* stmt = &block->nodes[i];

        if (stmt->type == XVR_AST_NODE_FN_RETURN) {
            has_explicit_return = true;
            found_return_in_block = true;
            Xvr_NodeFnReturn* ret = &stmt->returns;
            if (ret->returns) {
                if (ret->returns->type == XVR_AST_NODE_FN_COLLECTION) {
                    Xvr_NodeFnCollection* coll = &ret->returns->fnCollection;
                    if (coll->count > 0) {
                        return_value = Xvr_LLVMExpressionEmitterEmit(
                            expr_emitter, &coll->nodes[0]);
                    }
                } else {
                    return_value = Xvr_LLVMExpressionEmitterEmit(expr_emitter,
                                                                 ret->returns);
                }
                if (Xvr_LLVMContextHasError(context)) {
                    return false;
                }
            }

            if (is_void_function && return_value) {
                Xvr_LLVMContextSetError(context,
                                        "void function cannot return a value");
                return false;
            }

            if (return_value &&
                !check_return_type_compatibility(emitter, return_type,
                                                 return_value, fn_name)) {
                return false;
            }
        } else if (i == last_expr_index && !has_explicit_return) {
            LLVMValueRef implicit_val =
                Xvr_LLVMExpressionEmitterEmit(expr_emitter, stmt);
            if (Xvr_LLVMContextHasError(context)) {
                return false;
            }
            if (implicit_val) {
                LLVMTypeRef implicit_type = LLVMTypeOf(implicit_val);
                LLVMTypeKind implicit_kind = LLVMGetTypeKind(implicit_type);
                LLVMTypeKind return_kind = LLVMGetTypeKind(return_type);
                if (implicit_kind == return_kind || is_void_function) {
                    if (!is_void_function) {
                        return_value = implicit_val;
                    }
                }
            }
        } else {
            Xvr_LLVMExpressionEmitterEmit(expr_emitter, stmt);
            if (Xvr_LLVMContextHasError(context)) {
                return false;
            }
        }
    }

    if (is_void_function) {
        if (return_value) {
            Xvr_LLVMContextSetError(context,
                                    "void function cannot return a value");
            return false;
        }
        LLVMBuildRetVoid(Xvr_LLVMIRBuilderGetLLVMBuilder(builder));
    } else {
        if (!return_value) {
            if (found_return_in_block) {
                return true;
            }
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg),
                     "function '%s': missing return statement",
                     fn_name ? fn_name : "unknown");
            Xvr_LLVMContextSetError(context, error_msg);
            return false;
        }
        LLVMBuildRet(Xvr_LLVMIRBuilderGetLLVMBuilder(builder), return_value);
    }

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
