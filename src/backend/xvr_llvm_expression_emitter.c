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

#include "xvr_llvm_expression_emitter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_ast_node.h"
#include "xvr_cast_emit.h"
#include "xvr_format_string.h"
#include "xvr_literal.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_control_flow.h"
#include "xvr_llvm_function_emitter.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_type_mapper.h"
#include "xvr_opcodes.h"
#include "xvr_refstring.h"
#include "xvr_type.h"

/**
 * @brief Internal structure for expression emitter
 *
 * Holds references to all components needed for code generation.
 * Does not own the referenced objects.
 */
struct Xvr_LLVMExpressionEmitter {
    Xvr_LLVMContext* context;
    Xvr_LLVMModuleManager* module;
    Xvr_LLVMIRBuilder* builder;
    Xvr_LLVMTypeMapper* type_mapper;
    Xvr_LLVMControlFlow* control_flow;
    Xvr_LLVMCastEmitter* cast_emitter;
    void* fn_emitter;
};

/* Forward declaration */
static LLVMValueRef lookup_var(Xvr_LLVMExpressionEmitter* emitter,
                               const char* name);

Xvr_LLVMExpressionEmitter* Xvr_LLVMExpressionEmitterCreate(
    Xvr_LLVMContext* ctx, Xvr_LLVMModuleManager* module,
    Xvr_LLVMIRBuilder* builder, Xvr_LLVMTypeMapper* type_mapper) {
    /* Validate all required parameters */
    if (!ctx || !module || !builder || !type_mapper) {
        return NULL;
    }

    /* Allocate emitter structure */
    Xvr_LLVMExpressionEmitter* emitter =
        calloc(1, sizeof(Xvr_LLVMExpressionEmitter));
    if (!emitter) {
        return NULL;
    }

    /* Store references (no ownership transfer) */
    emitter->context = ctx;
    emitter->module = module;
    emitter->builder = builder;
    emitter->type_mapper = type_mapper;
    emitter->fn_emitter = NULL;

    LLVMModuleRef llvm_module = Xvr_LLVMModuleManagerGetModule(module);
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
    emitter->cast_emitter =
        Xvr_LLVMCastEmitterCreate(llvm_module, llvm_builder);

    return emitter;
}

void Xvr_LLVMExpressionEmitterSetFnEmitter(Xvr_LLVMExpressionEmitter* emitter,
                                           void* fn_emitter) {
    if (!emitter) {
        return;
    }
    emitter->fn_emitter = fn_emitter;
}

void Xvr_LLVMExpressionEmitterSetControlFlow(Xvr_LLVMExpressionEmitter* emitter,
                                             Xvr_LLVMControlFlow* cf) {
    if (!emitter) {
        return;
    }
    emitter->control_flow = cf;
}

LLVMValueRef Xvr_LLVMExpressionEmitterGetCurrentFunction(
    Xvr_LLVMExpressionEmitter* emitter) {
    if (!emitter || !emitter->fn_emitter) {
        return NULL;
    }
    Xvr_LLVMFunctionEmitter* fn_emitter =
        (Xvr_LLVMFunctionEmitter*)emitter->fn_emitter;
    return Xvr_LLVMFunctionEmitterGetCurrentFunction(fn_emitter);
}

void Xvr_LLVMExpressionEmitterDestroy(Xvr_LLVMExpressionEmitter* emitter) {
    if (!emitter) {
        return;
    }
    if (emitter->cast_emitter) {
        Xvr_LLVMCastEmitterDestroy(emitter->cast_emitter);
    }
    /* Note: We don't destroy the other referenced objects as we don't own them
     */
    free(emitter);
}

/**
 * @brief Converts XVR literal to LLVM constant value
 * @param emitter Expression emitter
 * @param literal XVR literal value
 * @return LLVM constant value
 *
 * Handles all XVR literal types:
 * - NULL -> i8* null
 * - BOOLEAN -> i1
 * - INTEGER -> i32
 * - FLOAT -> float
 * - STRING -> i8* (global string)
 */
static LLVMValueRef emit_literal_value(Xvr_LLVMExpressionEmitter* emitter,
                                       Xvr_Literal literal) {
    Xvr_LLVMContext* ctx = emitter->context;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(ctx);
    LLVMBuilderRef builder = Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);

    switch (literal.type) {
    case XVR_LITERAL_NULL:
        /* NULL literals become i8* null pointer */
        return LLVMConstNull(
            LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0));

    case XVR_LITERAL_BOOLEAN:
        /* Boolean: 0 = false, 1 = true */
        return LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx),
                            (literal.as.boolean ? 1ULL : 0ULL), false);

    case XVR_LITERAL_INTEGER:
        /* Integer: signed 32-bit integer */
        return LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx),
                            (unsigned long long)literal.as.integer, true);

    case XVR_LITERAL_INT8:
        return LLVMConstInt(LLVMInt8TypeInContext(llvm_ctx),
                            (unsigned long long)literal.as.int8_value, true);

    case XVR_LITERAL_INT16:
        return LLVMConstInt(LLVMInt16TypeInContext(llvm_ctx),
                            (unsigned long long)literal.as.int16_value, true);

    case XVR_LITERAL_INT32:
        return LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx),
                            (unsigned long long)literal.as.int32_value, true);

    case XVR_LITERAL_INT64:
        return LLVMConstInt(LLVMInt64TypeInContext(llvm_ctx),
                            (unsigned long long)literal.as.int64_value, true);

    case XVR_LITERAL_UINT8:
        return LLVMConstInt(LLVMInt8TypeInContext(llvm_ctx),
                            (unsigned long long)literal.as.uint8_value, false);

    case XVR_LITERAL_UINT16:
        return LLVMConstInt(LLVMInt16TypeInContext(llvm_ctx),
                            (unsigned long long)literal.as.uint16_value, false);

    case XVR_LITERAL_UINT32:
        return LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx),
                            (unsigned long long)literal.as.uint32_value, false);

    case XVR_LITERAL_UINT64:
        return LLVMConstInt(LLVMInt64TypeInContext(llvm_ctx),
                            (unsigned long long)literal.as.uint64_value, false);

    case XVR_LITERAL_FLOAT:
        /* Float: 32-bit floating point */
        return LLVMConstReal(LLVMFloatTypeInContext(llvm_ctx),
                             literal.as.number);

    case XVR_LITERAL_FLOAT16:
        /* Float16: 16-bit floating point (stored as bits) */
        return LLVMConstInt(LLVMInt16TypeInContext(llvm_ctx),
                            literal.as.float16_bits, false);

    case XVR_LITERAL_FLOAT32:
        /* Float32: 32-bit floating point */
        return LLVMConstReal(LLVMFloatTypeInContext(llvm_ctx),
                             literal.as.float32_value);

    case XVR_LITERAL_FLOAT64:
        /* Float64: 64-bit floating point (double) */
        return LLVMConstReal(LLVMDoubleTypeInContext(llvm_ctx),
                             literal.as.float64_value);

    case XVR_LITERAL_STRING: {
        /* String: create global string constant */
        const char* str_data = "";
        if (literal.as.string.ptr != NULL) {
            str_data = literal.as.string.ptr->data;
        }
        return LLVMBuildGlobalStringPtr(builder, str_data, "str_literal");
    }

    case XVR_LITERAL_IDENTIFIER:
        /* Identifier: look up variable */
        return Xvr_LLVMExpressionEmitterEmitIdentifier(emitter, literal);

    default:
        /* Unknown type: return null pointer */
        return LLVMConstNull(LLVMInt8TypeInContext(llvm_ctx));
    }
}

LLVMValueRef Xvr_LLVMExpressionEmitterEmitLiteral(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_NodeLiteral* literal) {
    if (!emitter || !literal) {
        return NULL;
    }
    return emit_literal_value(emitter, literal->literal);
}

static bool is_pointer_type(LLVMValueRef value) {
    if (!value) return false;
    LLVMTypeKind kind = LLVMGetTypeKind(LLVMTypeOf(value));
    return kind == LLVMPointerTypeKind;
}

static bool get_constant_string_data(LLVMValueRef value, const char** out_str,
                                     size_t* out_len) {
    if (!value) {
        return false;
    }

    LLVMValueKind kind = LLVMGetValueKind(value);

    if (kind == LLVMGlobalVariableValueKind) {
        LLVMValueRef initializer = LLVMGetInitializer(value);
        if (!initializer) {
            return false;
        }
        kind = LLVMGetValueKind(initializer);
    }

    if (kind != LLVMConstantDataArrayValueKind) {
        return false;
    }

    LLVMValueRef init = value;
    if (kind == LLVMGlobalVariableValueKind) {
        init = LLVMGetInitializer(value);
        if (!init || !LLVMIsConstant(init)) {
            return false;
        }
    }

    LLVMTypeRef type = LLVMTypeOf(init);

    if (LLVMGetTypeKind(type) != LLVMArrayTypeKind) {
        return false;
    }

    LLVMTypeRef elem_type = LLVMGetElementType(type);
    if (LLVMGetTypeKind(elem_type) != LLVMIntegerTypeKind) {
        return false;
    }

    unsigned bit_width = LLVMGetIntTypeWidth(elem_type);
    if (bit_width != 8) {
        return false;
    }

    size_t len = LLVMGetArrayLength(type);
    if (len == 0 || len > 65536) {
        return false;
    }

    if (out_len) {
        *out_len = len;
    }

    return true;
}

static bool extract_constant_string(LLVMValueRef value, char* buffer,
                                    size_t buffer_size) {
    if (!value || !buffer || buffer_size == 0) {
        return false;
    }

    LLVMValueRef init = value;
    LLVMValueKind kind = LLVMGetValueKind(value);

    if (kind == LLVMGlobalVariableValueKind) {
        init = LLVMGetInitializer(value);
        if (!init) {
            return false;
        }
    }

    if (LLVMGetValueKind(init) != LLVMConstantDataArrayValueKind) {
        return false;
    }

    for (unsigned i = 0; i < (unsigned)LLVMGetNumOperands(init) &&
                         i < (unsigned)(buffer_size - 1);
         i++) {
        LLVMValueRef operand = LLVMGetOperand(init, i);
        if (!operand) {
            return false;
        }
        buffer[i] = (char)LLVMConstIntGetSExtValue(operand);
    }

    return true;
}

static LLVMValueRef emit_string_concat(Xvr_LLVMExpressionEmitter* emitter,
                                       LLVMValueRef lhs, LLVMValueRef rhs) {
    size_t lhs_len = 0;
    size_t rhs_len = 0;

    bool lhs_is_str = get_constant_string_data(lhs, NULL, &lhs_len);
    bool rhs_is_str = get_constant_string_data(rhs, NULL, &rhs_len);

    if (lhs_is_str && rhs_is_str) {
        LLVMContextRef ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
        size_t total_len = lhs_len + rhs_len - 1;

        char* lhs_buf = (char*)malloc(lhs_len);
        char* rhs_buf = (char*)malloc(rhs_len);
        char* combined = (char*)malloc(total_len + 1);

        if (!lhs_buf || !rhs_buf || !combined) {
            free(lhs_buf);
            free(rhs_buf);
            free(combined);
            return NULL;
        }

        if (!extract_constant_string(lhs, lhs_buf, lhs_len) ||
            !extract_constant_string(rhs, rhs_buf, rhs_len)) {
            free(lhs_buf);
            free(rhs_buf);
            free(combined);
            return NULL;
        }

        memcpy(combined, lhs_buf, lhs_len - 1);
        memcpy(combined + lhs_len - 1, rhs_buf, rhs_len);
        combined[total_len] = '\0';

        free(lhs_buf);
        free(rhs_buf);

        LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);
        LLVMValueRef global_str = LLVMAddGlobal(
            module, LLVMArrayType(LLVMInt8TypeInContext(ctx), total_len + 1),
            "");
        LLVMSetLinkage(global_str, LLVMInternalLinkage);
        LLVMSetUnnamedAddr(global_str, true);
        LLVMSetInitializer(global_str, LLVMConstStringInContext(
                                           ctx, combined, total_len + 1, true));
        LLVMSetGlobalConstant(global_str, true);

        free(combined);

        return LLVMBuildPointerCast(
            Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder), global_str,
            LLVMPointerType(LLVMInt8TypeInContext(ctx), 0), "const_str_concat");
    }

    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);
    LLVMBuilderRef llvm_builder =
        Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);

    LLVMTypeRef fn_type = Xvr_LLVMModuleManagerGetFunctionType(
        emitter->module, "xvr_string_concat");

    if (!fn_type) {
        LLVMContextRef ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
        LLVMTypeRef i8_ptr = LLVMPointerType(LLVMInt8TypeInContext(ctx), 0);
        fn_type =
            LLVMFunctionType(i8_ptr, (LLVMTypeRef[]){i8_ptr, i8_ptr}, 2, false);
        Xvr_LLVMModuleManagerRegisterFunctionType(emitter->module,
                                                  "xvr_string_concat", fn_type);
    }

    LLVMValueRef concat_fn = LLVMGetNamedFunction(module, "xvr_string_concat");
    if (!concat_fn) {
        concat_fn = LLVMAddFunction(module, "xvr_string_concat", fn_type);
    }

    LLVMValueRef args[] = {lhs, rhs};
    return LLVMBuildCall2(llvm_builder, fn_type, concat_fn, args, 2,
                          "str_concat");
}

static LLVMValueRef emit_binary_op(Xvr_LLVMExpressionEmitter* emitter,
                                   Xvr_Opcode opcode, LLVMValueRef lhs,
                                   LLVMValueRef rhs) {
    Xvr_LLVMIRBuilder* builder = emitter->builder;
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);

    // Check if operands are floats
    LLVMTypeKind lhsKind = LLVMGetTypeKind(LLVMTypeOf(lhs));
    LLVMTypeKind rhsKind = LLVMGetTypeKind(LLVMTypeOf(rhs));
    bool isFloat =
        (lhsKind == LLVMFloatTypeKind || lhsKind == LLVMDoubleTypeKind ||
         rhsKind == LLVMFloatTypeKind || rhsKind == LLVMDoubleTypeKind);

    switch (opcode) {
    /* Arithmetic operations */
    case XVR_OP_ADDITION:
        if (isFloat) {
            return LLVMBuildFAdd(llvm_builder, lhs, rhs, "fadd");
        }
        if (is_pointer_type(lhs) && is_pointer_type(rhs)) {
            return emit_string_concat(emitter, lhs, rhs);
        }
        return Xvr_LLVMIRBuilderCreateAdd(emitter->builder, lhs, rhs, "add");

    case XVR_OP_SUBTRACTION:
        if (isFloat) {
            return LLVMBuildFSub(llvm_builder, lhs, rhs, "fsub");
        }
        return Xvr_LLVMIRBuilderCreateSub(emitter->builder, lhs, rhs, "sub");

    case XVR_OP_MULTIPLICATION:
        if (isFloat) {
            return LLVMBuildFMul(llvm_builder, lhs, rhs, "fmul");
        }
        return Xvr_LLVMIRBuilderCreateMul(emitter->builder, lhs, rhs, "mul");

    case XVR_OP_DIVISION:
        if (isFloat) {
            return LLVMBuildFDiv(llvm_builder, lhs, rhs, "fdiv");
        }
        return Xvr_LLVMIRBuilderCreateSDiv(emitter->builder, lhs, rhs, "sdiv");

    case XVR_OP_MODULO:
        if (isFloat) {
            return LLVMBuildFRem(llvm_builder, lhs, rhs, "frem");
        }
        return Xvr_LLVMIRBuilderCreateSRem(emitter->builder, lhs, rhs, "srem");

    /* Logical operations */
    case XVR_OP_AND: {
        LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
        return LLVMBuildAnd(llvm_builder, lhs, rhs, "and");
    }

    case XVR_OP_OR: {
        LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
        return LLVMBuildOr(llvm_builder, lhs, rhs, "or");
    }

    /* Comparison operations (signed) */
    case XVR_OP_COMPARE_LESS:
        return Xvr_LLVMIRBuilderCreateICmpSLT(builder, lhs, rhs, "lt");

    case XVR_OP_COMPARE_LESS_EQUAL:
        return Xvr_LLVMIRBuilderCreateICmpSLE(builder, lhs, rhs, "le");

    case XVR_OP_COMPARE_GREATER:
        return Xvr_LLVMIRBuilderCreateICmpSGT(builder, lhs, rhs, "gt");

    case XVR_OP_COMPARE_GREATER_EQUAL:
        return Xvr_LLVMIRBuilderCreateICmpSGE(builder, lhs, rhs, "ge");

    case XVR_OP_COMPARE_EQUAL:
        return Xvr_LLVMIRBuilderCreateICmpEQ(builder, lhs, rhs, "eq");

    case XVR_OP_COMPARE_NOT_EQUAL:
        return Xvr_LLVMIRBuilderCreateICmpNE(builder, lhs, rhs, "ne");

    default:
        return NULL;
    }
}

static LLVMValueRef emit_printf(Xvr_LLVMExpressionEmitter* emitter,
                                Xvr_ASTNode* args);
static LLVMValueRef emit_printfln(Xvr_LLVMExpressionEmitter* emitter,
                                  Xvr_ASTNode* args);
static LLVMValueRef emit_max(Xvr_LLVMExpressionEmitter* emitter,
                             Xvr_ASTNode* args);
static LLVMValueRef lookup_var_with_type(Xvr_LLVMExpressionEmitter* emitter,
                                         const char* name,
                                         Xvr_LiteralType* out_type);

LLVMValueRef Xvr_LLVMExpressionEmitterEmitBinary(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_NodeBinary* binary) {
    if (!emitter || !binary) {
        return NULL;
    }

    /* Handle PRINTF specially - it's a variadic function call */
    if (binary->opcode == XVR_OP_PRINTF) {
        return emit_printf(emitter, binary->left);
    }

    /* Error: standalone print() is not supported, use std::print() instead */
    if (binary->opcode == XVR_OP_FN_CALL) {
        if (binary->left && binary->left->type == XVR_AST_NODE_LITERAL &&
            binary->left->atomic.literal.type == XVR_LITERAL_IDENTIFIER) {
            const char* fn_name =
                (const char*)binary->left->atomic.literal.as.string.ptr->data;
            if (fn_name && strcmp(fn_name, "print") == 0) {
                Xvr_LLVMContextSetError(
                    emitter->context,
                    "print() is not supported, use std::print() instead");
                return NULL;
            }
            if (fn_name && strcmp(fn_name, "max") == 0) {
                Xvr_LLVMContextSetError(
                    emitter->context,
                    "max() is not supported, use std::max() instead");
                return NULL;
            }

            if (fn_name && strcmp(fn_name, "sizeof") == 0) {
                if (binary->right &&
                    binary->right->type == XVR_AST_NODE_FN_CALL) {
                    Xvr_ASTNode* args_node = binary->right->fnCall.arguments;
                    if (args_node &&
                        args_node->type == XVR_AST_NODE_FN_COLLECTION &&
                        args_node->fnCollection.count == 1) {
                        Xvr_ASTNode* type_arg =
                            &args_node->fnCollection.nodes[0];
                        if (type_arg->type == XVR_AST_NODE_LITERAL &&
                            type_arg->atomic.literal.type == XVR_LITERAL_TYPE) {
                            Xvr_LiteralType type_val =
                                XVR_AS_TYPE(type_arg->atomic.literal).typeOf;
                            int size_bits = 0;
                            switch (type_val) {
                            case XVR_LITERAL_VOID:
                                size_bits = 0;
                                break;
                            case XVR_LITERAL_BOOLEAN:
                            case XVR_LITERAL_INT8:
                            case XVR_LITERAL_UINT8:
                                size_bits = 8;
                                break;
                            case XVR_LITERAL_INT16:
                            case XVR_LITERAL_UINT16:
                                size_bits = 16;
                                break;
                            case XVR_LITERAL_INTEGER:
                            case XVR_LITERAL_FLOAT:
                            case XVR_LITERAL_INT32:
                            case XVR_LITERAL_UINT32:
                            case XVR_LITERAL_FLOAT32:
                                size_bits = 32;
                                break;
                            case XVR_LITERAL_INT64:
                            case XVR_LITERAL_UINT64:
                            case XVR_LITERAL_FLOAT64:
                                size_bits = 64;
                                break;
                            case XVR_LITERAL_FLOAT16:
                                size_bits = 16;
                                break;
                            default:
                                size_bits = 0;
                                break;
                            }
                            LLVMContextRef ctx =
                                Xvr_LLVMContextGetLLVMContext(emitter->context);
                            return LLVMConstInt(LLVMInt32TypeInContext(ctx),
                                                size_bits, false);
                        }
                    }
                }
                Xvr_LLVMContextSetError(emitter->context,
                                        "sizeof expects a type argument");
                return NULL;
            }

            if (fn_name && strcmp(fn_name, "len") == 0) {
                if (binary->right &&
                    binary->right->type == XVR_AST_NODE_FN_CALL) {
                    Xvr_ASTNode* args_node = binary->right->fnCall.arguments;
                    if (args_node &&
                        args_node->type == XVR_AST_NODE_FN_COLLECTION &&
                        args_node->fnCollection.count >= 1) {
                        Xvr_ASTNode* target = &args_node->fnCollection.nodes[0];

                        /* Check if target is an identifier - look up its type
                         */
                        Xvr_LiteralType var_type_xvr = XVR_LITERAL_ANY;
                        if (target->type == XVR_AST_NODE_LITERAL &&
                            target->atomic.literal.type ==
                                XVR_LITERAL_IDENTIFIER) {
                            const char* var_name =
                                (const char*)
                                    target->atomic.literal.as.string.ptr->data;
                            LLVMValueRef var_ptr = lookup_var_with_type(
                                emitter, var_name, &var_type_xvr);

                            if (var_ptr && var_type_xvr == XVR_LITERAL_ARRAY) {
                                /* Try to get array count from local_vars */
                                Xvr_LLVMFunctionEmitter* fn_emitter =
                                    (Xvr_LLVMFunctionEmitter*)
                                        emitter->fn_emitter;
                                if (fn_emitter) {
                                    int array_count =
                                        Xvr_LLVMFunctionEmitterLookupVarArrayCount(
                                            fn_emitter, var_name);
                                    if (array_count > 0) {
                                        LLVMContextRef ctx =
                                            Xvr_LLVMContextGetLLVMContext(
                                                emitter->context);
                                        return LLVMConstInt(
                                            LLVMInt32TypeInContext(ctx),
                                            array_count, false);
                                    }
                                }
                                /* Fallback to LLVM types */
                                LLVMTypeRef ptr_type = LLVMTypeOf(var_ptr);
                                if (ptr_type && LLVMGetTypeKind(ptr_type) ==
                                                    LLVMPointerTypeKind) {
                                    LLVMTypeRef elem_type =
                                        LLVMGetElementType(ptr_type);
                                    if (elem_type &&
                                        LLVMGetTypeKind(elem_type) ==
                                            LLVMArrayTypeKind) {
                                        unsigned elem_count =
                                            LLVMGetArrayLength(elem_type);
                                        LLVMContextRef ctx =
                                            Xvr_LLVMContextGetLLVMContext(
                                                emitter->context);
                                        return LLVMConstInt(
                                            LLVMInt32TypeInContext(ctx),
                                            elem_count, false);
                                    }
                                    LLVMTypeRef alloc_type =
                                        LLVMGetAllocatedType(var_ptr);
                                    if (alloc_type &&
                                        LLVMGetTypeKind(alloc_type) ==
                                            LLVMArrayTypeKind) {
                                        unsigned elem_count =
                                            LLVMGetArrayLength(alloc_type);
                                        LLVMContextRef ctx =
                                            Xvr_LLVMContextGetLLVMContext(
                                                emitter->context);
                                        return LLVMConstInt(
                                            LLVMInt32TypeInContext(ctx),
                                            elem_count, false);
                                    }
                                }
                            }
                        }

                        /* For non-identifiers or other types, emit normally */
                        LLVMValueRef target_value =
                            Xvr_LLVMExpressionEmitterEmit(emitter, target);
                        if (!target_value) {
                            return NULL;
                        }

                        LLVMContextRef ctx =
                            Xvr_LLVMContextGetLLVMContext(emitter->context);
                        LLVMTypeRef target_type = LLVMTypeOf(target_value);
                        LLVMTypeKind kind = LLVMGetTypeKind(target_type);

                        if (kind == LLVMPointerTypeKind) {
                            LLVMTypeRef elem_type =
                                LLVMGetElementType(target_type);
                            if (elem_type && LLVMGetTypeKind(elem_type) ==
                                                 LLVMArrayTypeKind) {
                                unsigned elem_count =
                                    LLVMGetArrayLength(elem_type);
                                return LLVMConstInt(LLVMInt32TypeInContext(ctx),
                                                    elem_count, false);
                            }
                            LLVMModuleRef m =
                                Xvr_LLVMModuleManagerGetModule(emitter->module);
                            LLVMBuilderRef b = Xvr_LLVMIRBuilderGetLLVMBuilder(
                                emitter->builder);
                            const char* len_fn_name = "xvr_str_len";
                            LLVMTypeRef len_fn_type = LLVMFunctionType(
                                LLVMInt32TypeInContext(ctx),
                                (LLVMTypeRef[]){LLVMInt8TypeInContext(ctx)}, 1,
                                false);
                            LLVMValueRef len_fn =
                                LLVMGetNamedFunction(m, len_fn_name);
                            if (!len_fn) {
                                len_fn = LLVMAddFunction(m, len_fn_name,
                                                         len_fn_type);
                            }
                            return LLVMBuildCall2(b, len_fn_type, len_fn,
                                                  &target_value, 1, "");
                        } else if (kind == LLVMArrayTypeKind) {
                            unsigned elem_count =
                                LLVMGetArrayLength(target_type);
                            return LLVMConstInt(LLVMInt32TypeInContext(ctx),
                                                elem_count, false);
                        } else {
                            Xvr_LLVMContextSetError(
                                emitter->context,
                                "len() only works on strings and arrays");
                            return NULL;
                        }
                    }
                }
                Xvr_LLVMContextSetError(emitter->context,
                                        "len() expects one argument");
                return NULL;
            }

            LLVMModuleRef module =
                Xvr_LLVMModuleManagerGetModule(emitter->module);
            LLVMValueRef callee = LLVMGetNamedFunction(module, fn_name);

            if (callee) {
                LLVMBuilderRef llvm_builder =
                    Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);

                int arg_count = 0;
                LLVMValueRef* args = NULL;

                if (binary->right) {
                    Xvr_ASTNode* args_node = NULL;
                    if (binary->right->type == XVR_AST_NODE_COMPOUND) {
                        arg_count = binary->right->compound.count;
                        if (arg_count > 0) {
                            args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) *
                                                         arg_count);
                            for (int i = 0; i < arg_count; i++) {
                                Xvr_ASTNode* arg =
                                    &binary->right->compound.nodes[i];
                                args[i] =
                                    Xvr_LLVMExpressionEmitterEmit(emitter, arg);
                                if (!args[i]) {
                                    args[i] = LLVMConstInt(
                                        LLVMInt32TypeInContext(
                                            Xvr_LLVMContextGetLLVMContext(
                                                emitter->context)),
                                        0, false);
                                }
                            }
                        }
                    } else if (binary->right->type == XVR_AST_NODE_FN_CALL) {
                        args_node = binary->right->fnCall.arguments;
                        if (args_node &&
                            args_node->type == XVR_AST_NODE_FN_COLLECTION) {
                            arg_count = args_node->fnCollection.count;
                            if (arg_count > 0) {
                                args = (LLVMValueRef*)malloc(
                                    sizeof(LLVMValueRef) * arg_count);
                                for (int i = 0; i < arg_count; i++) {
                                    Xvr_ASTNode* arg =
                                        &args_node->fnCollection.nodes[i];
                                    args[i] = Xvr_LLVMExpressionEmitterEmit(
                                        emitter, arg);
                                    if (!args[i]) {
                                        args[i] = LLVMConstInt(
                                            LLVMInt32TypeInContext(
                                                Xvr_LLVMContextGetLLVMContext(
                                                    emitter->context)),
                                            0, false);
                                    }
                                }
                            }
                        }
                    }
                }

                LLVMTypeRef fn_type = Xvr_LLVMModuleManagerGetFunctionType(
                    emitter->module, fn_name);
                if (!fn_type) {
                    fn_type = LLVMTypeOf(callee);
                }
                LLVMValueRef result = LLVMBuildCall2(
                    llvm_builder, fn_type, callee, args, arg_count, "");

                if (args) {
                    free(args);
                }

                return result;
            }
        }
        return NULL;
    }

    /* Handle namespace::function calls like std::print */
    if (binary->opcode == XVR_OP_DOT) {
        /* Check if this is std::print */
        if (binary->left && binary->left->type == XVR_AST_NODE_LITERAL &&
            binary->left->atomic.literal.type == XVR_LITERAL_IDENTIFIER) {
            const char* namespace_name =
                (const char*)binary->left->atomic.literal.as.string.ptr->data;

            /* Check if it's std:: and the right side is a function call */
            if (namespace_name && strcmp(namespace_name, "std") == 0) {
                /* Right side should be a function call binary node */
                if (binary->right &&
                    binary->right->type == XVR_AST_NODE_BINARY &&
                    (binary->right->binary.opcode == XVR_OP_FN_CALL ||
                     binary->right->binary.opcode == XVR_OP_DOT)) {
                    // Get the function name
                    const char* fn_name = NULL;
                    if (binary->right->binary.left &&
                        binary->right->binary.left->type ==
                            XVR_AST_NODE_LITERAL &&
                        binary->right->binary.left->atomic.literal.type ==
                            XVR_LITERAL_IDENTIFIER) {
                        fn_name = (const char*)binary->right->binary.left
                                      ->atomic.literal.as.string.ptr->data;
                    }

                    // The function call is stored as XVR_AST_NODE_FN_CALL in
                    // binary.right For now, check if we can find the FN_CALL
                    // node
                    Xvr_ASTNode* fn_call_node = NULL;

                    // Try to find the FN_CALL node - it could be in different
                    // places
                    if (binary->right->binary.right &&
                        binary->right->binary.right->type ==
                            XVR_AST_NODE_FN_CALL) {
                        fn_call_node = binary->right->binary.right;
                    }

                    if (fn_call_node) {
                        if (fn_name && strcmp(fn_name, "print") == 0) {
                            return emit_printf(emitter,
                                               fn_call_node->fnCall.arguments);
                        }
                        if (fn_name && strcmp(fn_name, "println") == 0) {
                            return emit_printfln(
                                emitter, fn_call_node->fnCall.arguments);
                        }
                        if (fn_name && strcmp(fn_name, "max") == 0) {
                            return emit_max(emitter,
                                            fn_call_node->fnCall.arguments);
                        }
                    }
                }
            }
        }
        return NULL;
    }

    /* Handle variable assignment: x = expr, x += expr, etc. */
    if (binary->opcode == XVR_OP_VAR_ASSIGN ||
        binary->opcode == XVR_OP_VAR_ADDITION_ASSIGN ||
        binary->opcode == XVR_OP_VAR_SUBTRACTION_ASSIGN ||
        binary->opcode == XVR_OP_VAR_MULTIPLICATION_ASSIGN ||
        binary->opcode == XVR_OP_VAR_DIVISION_ASSIGN ||
        binary->opcode == XVR_OP_VAR_MODULO_ASSIGN) {
        /* Get the variable pointer for the left side */
        if (binary->left && binary->left->type == XVR_AST_NODE_LITERAL &&
            binary->left->atomic.literal.type == XVR_LITERAL_IDENTIFIER) {
            const char* var_name =
                (const char*)binary->left->atomic.literal.as.string.ptr->data;

            /* Look up the variable */
            LLVMValueRef var_ptr = lookup_var(emitter, var_name);
            if (!var_ptr) {
                return NULL;
            }

            /* Evaluate the right side */
            LLVMValueRef rhs =
                Xvr_LLVMExpressionEmitterEmit(emitter, binary->right);
            if (!rhs) {
                return NULL;
            }

            /* Handle compound assignments (+=, -=, etc.) */
            LLVMValueRef value_to_store = rhs;
            if (binary->opcode != XVR_OP_VAR_ASSIGN) {
                /* Load current value and compute with rhs */
                LLVMTypeRef var_type = LLVMGetElementType(LLVMTypeOf(var_ptr));
                if (!var_type) {
                    /* Opaque pointer - default to i32 */
                    var_type = LLVMInt32TypeInContext(
                        Xvr_LLVMContextGetLLVMContext(emitter->context));
                }
                LLVMBuilderRef llvm_builder =
                    Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);
                LLVMValueRef current =
                    LLVMBuildLoad2(llvm_builder, var_type, var_ptr, var_name);

                switch (binary->opcode) {
                case XVR_OP_VAR_ADDITION_ASSIGN:
                    value_to_store = Xvr_LLVMIRBuilderCreateAdd(
                        emitter->builder, current, rhs, "add");
                    break;
                case XVR_OP_VAR_SUBTRACTION_ASSIGN:
                    value_to_store = Xvr_LLVMIRBuilderCreateSub(
                        emitter->builder, current, rhs, "sub");
                    break;
                case XVR_OP_VAR_MULTIPLICATION_ASSIGN:
                    value_to_store = Xvr_LLVMIRBuilderCreateMul(
                        emitter->builder, current, rhs, "mul");
                    break;
                case XVR_OP_VAR_DIVISION_ASSIGN:
                    value_to_store = Xvr_LLVMIRBuilderCreateSDiv(
                        emitter->builder, current, rhs, "sdiv");
                    break;
                case XVR_OP_VAR_MODULO_ASSIGN:
                    value_to_store = Xvr_LLVMIRBuilderCreateSRem(
                        emitter->builder, current, rhs, "mod");
                    break;
                default:
                    break;
                }
            }

            /* Store the value */
            Xvr_LLVMIRBuilderCreateStore(emitter->builder, value_to_store,
                                         var_ptr);
            return value_to_store;
        }
        /* Handle array index assignment: arr[i] = value */
        if (binary->left && binary->left->type == XVR_AST_NODE_INDEX) {
            Xvr_NodeIndex* index_node = &binary->left->index;

            /* Get the array variable */
            if (index_node->first &&
                index_node->first->type == XVR_AST_NODE_LITERAL &&
                index_node->first->atomic.literal.type ==
                    XVR_LITERAL_IDENTIFIER) {
                const char* var_name =
                    (const char*)
                        index_node->first->atomic.literal.as.string.ptr->data;
                LLVMValueRef var_ptr = lookup_var(emitter, var_name);
                if (!var_ptr) {
                    return NULL;
                }

                /* Emit the value to store */
                LLVMValueRef value =
                    Xvr_LLVMExpressionEmitterEmit(emitter, binary->right);
                if (!value) {
                    return NULL;
                }

                /* Emit the index */
                LLVMValueRef index_val = NULL;
                if (index_node->second) {
                    index_val = Xvr_LLVMExpressionEmitterEmit(
                        emitter, index_node->second);
                }
                if (!index_val) {
                    index_val = LLVMConstInt(
                        LLVMInt32TypeInContext(
                            Xvr_LLVMContextGetLLVMContext(emitter->context)),
                        0, false);
                }

                /* Get array type and create gep */
                LLVMTypeRef ptr_type = LLVMTypeOf(var_ptr);
                LLVMTypeRef elem_type = LLVMGetElementType(ptr_type);

                LLVMBuilderRef llvm_builder =
                    Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);
                LLVMValueRef indices[] = {
                    LLVMConstInt(
                        LLVMInt32TypeInContext(
                            Xvr_LLVMContextGetLLVMContext(emitter->context)),
                        0, false),
                    index_val};
                LLVMValueRef elem_ptr = LLVMBuildInBoundsGEP2(
                    llvm_builder, elem_type, var_ptr, indices, 2, "array_idx");
                Xvr_LLVMIRBuilderCreateStore(emitter->builder, value, elem_ptr);
                return value;
            }
        }
        return NULL;
    }

    /* Recursively emit left operand */
    LLVMValueRef lhs = Xvr_LLVMExpressionEmitterEmit(emitter, binary->left);
    if (!lhs) {
        return NULL;
    }

    /* Recursively emit right operand */
    LLVMValueRef rhs = Xvr_LLVMExpressionEmitterEmit(emitter, binary->right);
    if (!rhs) {
        return NULL;
    }

    /* Generate binary operation */
    return emit_binary_op(emitter, binary->opcode, lhs, rhs);
}

/* Forward declaration for emit_array_print */
static LLVMValueRef emit_array_print(Xvr_LLVMExpressionEmitter* emitter,
                                     LLVMValueRef array_ptr, int array_count,
                                     LLVMTypeRef elem_type,
                                     const char* format_str) {
    (void)emitter;
    (void)array_ptr;
    (void)array_count;
    (void)elem_type;
    (void)format_str;
    return NULL;
}

static LLVMValueRef emit_printf(Xvr_LLVMExpressionEmitter* emitter,
                                Xvr_ASTNode* args) {
    if (!emitter || !args) {
        return NULL;
    }

    Xvr_LLVMIRBuilder* builder = emitter->builder;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);

    LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");
    if (!printf_fn) {
        LLVMTypeRef printf_type =
            LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx), NULL, 0, true);
        printf_fn = LLVMAddFunction(module, "printf", printf_type);
    }

    if ((args->type == XVR_AST_NODE_COMPOUND && args->compound.count >= 1) ||
        (args->type == XVR_AST_NODE_FN_COLLECTION &&
         args->fnCollection.count >= 1)) {
        const char* format_str = NULL;
        LLVMValueRef* call_args = NULL;
        int arg_count = 0;
        bool has_format_string = false;

        Xvr_ASTNode* format_node = NULL;
        if (args->type == XVR_AST_NODE_COMPOUND) {
            format_node = &args->compound.nodes[0];
            arg_count = args->compound.count - 1;
        } else {
            format_node = &args->fnCollection.nodes[0];
            arg_count = args->fnCollection.count - 1;
        }

        if (format_node && format_node->type == XVR_AST_NODE_LITERAL &&
            format_node->atomic.literal.type == XVR_LITERAL_STRING) {
            has_format_string = true;
            format_str =
                (const char*)format_node->atomic.literal.as.string.ptr->data;
        }

        int total_args = (args->type == XVR_AST_NODE_COMPOUND)
                             ? args->compound.count
                             : args->fnCollection.count;

        if (!has_format_string && total_args == 1 && format_node) {
            Xvr_ASTNode* array_node = NULL;
            if (args->type == XVR_AST_NODE_COMPOUND &&
                args->compound.literalType == XVR_LITERAL_ARRAY) {
                array_node = args;
            } else if (args->type == XVR_AST_NODE_FN_COLLECTION &&
                       args->fnCollection.count > 0 &&
                       args->fnCollection.nodes[0].type ==
                           XVR_AST_NODE_COMPOUND &&
                       args->fnCollection.nodes[0].compound.literalType ==
                           XVR_LITERAL_ARRAY) {
                array_node = &args->fnCollection.nodes[0];
            }

            if (array_node) {
                Xvr_LLVMContextSetError(
                    emitter->context,
                    "print: cannot print array directly. Use format string: "
                    "std::print(\"{}\", array)");
                return NULL;
            }

            LLVMValueRef arg_val =
                Xvr_LLVMExpressionEmitterEmit(emitter, format_node);
            if (arg_val) {
                LLVMTypeRef arg_type = LLVMTypeOf(arg_val);
                const char* fmt_str = "%s";
                LLVMValueRef print_arg = arg_val;
                LLVMTypeKind kind = LLVMGetTypeKind(arg_type);
                if (kind == LLVMIntegerTypeKind) {
                    Xvr_LLVMContextSetError(
                        emitter->context,
                        "print: use format string for non-string types: "
                        "std::print(\"{}\", value)");
                    return NULL;
                } else if (kind == LLVMFloatTypeKind) {
                    Xvr_LLVMContextSetError(
                        emitter->context,
                        "print: use format string for non-string types: "
                        "std::print(\"{}\", value)");
                    return NULL;
                } else if (kind == LLVMDoubleTypeKind) {
                    Xvr_LLVMContextSetError(
                        emitter->context,
                        "print: use format string for non-string types: "
                        "std::print(\"{}\", value)");
                    return NULL;
                } else if (kind == LLVMArrayTypeKind) {
                    Xvr_LLVMContextSetError(emitter->context,
                                            "print: cannot print array "
                                            "directly. Use format string: "
                                            "std::print(\"{}\", array)");
                    return NULL;
                } else if (kind == LLVMPointerTypeKind) {
                    fmt_str = "%s";
                }
                LLVMValueRef fmt_global =
                    LLVMBuildGlobalStringPtr(llvm_builder, fmt_str, "fmt_str");
                LLVMValueRef all_args[2] = {fmt_global, print_arg};
                LLVMTypeRef printf_type = LLVMFunctionType(
                    LLVMInt32TypeInContext(llvm_ctx),
                    (LLVMTypeRef[]){
                        LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0),
                        arg_type},
                    2, true);
                LLVMBuildCall2(llvm_builder, printf_type, printf_fn, all_args,
                               2, "printf_call");
            }
            return NULL;
        }

        if (arg_count > 0) {
            call_args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * arg_count);
            for (int i = 0; i < arg_count; i++) {
                Xvr_ASTNode* arg = NULL;
                if (args->type == XVR_AST_NODE_COMPOUND) {
                    arg = &args->compound.nodes[i + 1];
                } else {
                    arg = &args->fnCollection.nodes[i + 1];
                }
                LLVMValueRef arg_val =
                    Xvr_LLVMExpressionEmitterEmit(emitter, arg);
                if (arg_val) {
                    call_args[i] = arg_val;
                } else {
                    call_args[i] = LLVMConstInt(
                        LLVMInt32TypeInContext(llvm_ctx), 0, false);
                }
            }
        }

        if (has_format_string && arg_count > 0) {
            XvrFormatString* fmt = XvrFormatStringParse(format_str, arg_count);
            if (fmt && fmt->is_valid) {
                XvrFormatArgType* arg_types = (XvrFormatArgType*)malloc(
                    sizeof(XvrFormatArgType) * arg_count);
                for (int i = 0; i < arg_count; i++) {
                    LLVMValueRef arg_val = call_args[i];
                    LLVMTypeRef arg_type = LLVMTypeOf(arg_val);
                    LLVMTypeKind kind = LLVMGetTypeKind(arg_type);
                    if (kind == LLVMIntegerTypeKind) {
                        arg_types[i] = XVR_FORMAT_ARG_INT;
                    } else if (kind == LLVMFloatTypeKind) {
                        arg_types[i] = XVR_FORMAT_ARG_FLOAT;
                    } else if (kind == LLVMDoubleTypeKind) {
                        arg_types[i] = XVR_FORMAT_ARG_DOUBLE;
                    } else if (kind == LLVMPointerTypeKind) {
                        arg_types[i] = XVR_FORMAT_ARG_STRING;
                    } else if (kind == LLVMArrayTypeKind) {
                        Xvr_LLVMContextSetError(
                            emitter->context,
                            "print: cannot use array with format string. "
                            "Use std::print(array) without format string, or "
                            "print elements individually");
                        free(arg_types);
                        XvrFormatStringFree(fmt);
                        return NULL;
                    } else {
                        arg_types[i] = XVR_FORMAT_ARG_STRING;
                    }
                }

                char* printf_fmt =
                    XvrFormatStringBuildPrintfFormat(fmt, arg_types, arg_count);

                LLVMValueRef fmt_global = LLVMBuildGlobalStringPtr(
                    llvm_builder, printf_fmt, "fmt_str");

                LLVMValueRef* all_args = (LLVMValueRef*)malloc(
                    sizeof(LLVMValueRef) * (arg_count + 1));
                all_args[0] = fmt_global;
                for (int i = 0; i < arg_count; i++) {
                    LLVMValueRef arg = call_args[i];
                    LLVMTypeRef arg_type = LLVMTypeOf(arg);
                    if (LLVMGetTypeKind(arg_type) == LLVMFloatTypeKind) {
                        all_args[i + 1] =
                            LLVMBuildFPExt(llvm_builder, arg,
                                           LLVMDoubleTypeInContext(llvm_ctx),
                                           "float_to_double");
                    } else {
                        all_args[i + 1] = arg;
                    }
                }

                LLVMTypeRef* param_types =
                    (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * (arg_count + 1));
                param_types[0] =
                    LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0);
                for (int i = 0; i < arg_count; i++) {
                    LLVMValueRef arg = call_args[i];
                    LLVMTypeRef arg_type = LLVMTypeOf(arg);
                    if (LLVMGetTypeKind(arg_type) == LLVMFloatTypeKind) {
                        param_types[i + 1] = LLVMDoubleTypeInContext(llvm_ctx);
                    } else {
                        param_types[i + 1] = arg_type;
                    }
                }

                LLVMTypeRef printf_type =
                    LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                                     param_types, arg_count + 1, true);
                LLVMBuildCall2(llvm_builder, printf_type, printf_fn, all_args,
                               arg_count + 1, "printf_call");

                free(all_args);
                free(param_types);
                free(printf_fmt);
                free(arg_types);
                XvrFormatStringFree(fmt);
            } else {
                LLVMValueRef fmt_global =
                    LLVMBuildGlobalStringPtr(llvm_builder, "%s", "fmt_str");
                LLVMTypeRef printf_type = LLVMFunctionType(
                    LLVMInt32TypeInContext(llvm_ctx), NULL, 0, true);
                LLVMBuildCall2(llvm_builder, printf_type, printf_fn,
                               &fmt_global, 1, "printf_call");
                if (fmt) {
                    XvrFormatStringFree(fmt);
                }
            }
        } else {
            if (arg_count > 0) {
                LLVMValueRef fmt_global =
                    LLVMBuildGlobalStringPtr(llvm_builder, "%s", "fmt_str");
                LLVMValueRef* all_args = (LLVMValueRef*)malloc(
                    sizeof(LLVMValueRef) * (arg_count + 1));
                all_args[0] = fmt_global;
                for (int i = 0; i < arg_count; i++) {
                    LLVMValueRef arg = call_args[i];
                    LLVMTypeRef arg_type = LLVMTypeOf(arg);
                    if (LLVMGetTypeKind(arg_type) == LLVMFloatTypeKind) {
                        all_args[i + 1] =
                            LLVMBuildFPExt(llvm_builder, arg,
                                           LLVMDoubleTypeInContext(llvm_ctx),
                                           "float_to_double");
                    } else {
                        all_args[i + 1] = arg;
                    }
                }

                LLVMTypeRef* param_types =
                    (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * (arg_count + 1));
                param_types[0] =
                    LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0);
                for (int i = 0; i < arg_count; i++) {
                    LLVMValueRef arg = call_args[i];
                    LLVMTypeRef arg_type = LLVMTypeOf(arg);
                    if (LLVMGetTypeKind(arg_type) == LLVMFloatTypeKind) {
                        param_types[i + 1] = LLVMDoubleTypeInContext(llvm_ctx);
                    } else {
                        param_types[i + 1] = arg_type;
                    }
                }

                LLVMTypeRef printf_type =
                    LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                                     param_types, arg_count + 1, true);
                LLVMBuildCall2(llvm_builder, printf_type, printf_fn, all_args,
                               arg_count + 1, "printf_call");

                free(param_types);
                free(all_args);
            } else {
                LLVMValueRef fmt_global = LLVMBuildGlobalStringPtr(
                    llvm_builder, format_str ? format_str : "%s", "fmt_str");
                if (has_format_string || !format_node) {
                    LLVMTypeRef printf_type = LLVMFunctionType(
                        LLVMInt32TypeInContext(llvm_ctx), NULL, 0, true);
                    LLVMBuildCall2(llvm_builder, printf_type, printf_fn,
                                   &fmt_global, 1, "printf_call");
                } else {
                    LLVMValueRef arg_val =
                        Xvr_LLVMExpressionEmitterEmit(emitter, format_node);
                    if (arg_val) {
                        LLVMValueRef all_args[2] = {fmt_global, arg_val};
                        LLVMTypeRef printf_type = LLVMFunctionType(
                            LLVMInt32TypeInContext(llvm_ctx),
                            (LLVMTypeRef[]){
                                LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx),
                                                0),
                                LLVMTypeOf(arg_val)},
                            2, true);
                        LLVMBuildCall2(llvm_builder, printf_type, printf_fn,
                                       all_args, 2, "printf_call");
                    }
                }
            }
        }

        if (call_args) {
            free(call_args);
        }
    }

    return NULL;
}

static LLVMValueRef emit_printfln(Xvr_LLVMExpressionEmitter* emitter,
                                  Xvr_ASTNode* args) {
    if (!emitter || !args) {
        return NULL;
    }

    if (args->type != XVR_AST_NODE_COMPOUND &&
        args->type != XVR_AST_NODE_FN_COLLECTION) {
        Xvr_LLVMContextSetError(emitter->context,
                                "println: invalid argument type");
        return NULL;
    }

    int total_nodes = (args->type == XVR_AST_NODE_COMPOUND)
                          ? args->compound.count
                          : args->fnCollection.count;

    if (total_nodes < 1) {
        Xvr_LLVMContextSetError(emitter->context,
                                "println: requires at least one argument");
        return NULL;
    }

    Xvr_LLVMIRBuilder* builder = emitter->builder;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);

    LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");
    if (!printf_fn) {
        LLVMTypeRef printf_type =
            LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx), NULL, 0, true);
        printf_fn = LLVMAddFunction(module, "printf", printf_type);
    }

    const char* format_str = NULL;
    LLVMValueRef* call_args = NULL;
    int arg_count = 0;
    bool has_format_string = false;

    Xvr_ASTNode* format_node = NULL;
    if (args->type == XVR_AST_NODE_COMPOUND) {
        format_node = &args->compound.nodes[0];
        arg_count = args->compound.count - 1;
    } else {
        format_node = &args->fnCollection.nodes[0];
        arg_count = args->fnCollection.count - 1;
    }

    if (format_node && format_node->type == XVR_AST_NODE_LITERAL &&
        format_node->atomic.literal.type == XVR_LITERAL_STRING) {
        has_format_string = true;
        format_str =
            (const char*)format_node->atomic.literal.as.string.ptr->data;
    }

    int total_args = (args->type == XVR_AST_NODE_COMPOUND)
                         ? args->compound.count
                         : args->fnCollection.count;

    if (!has_format_string && total_args == 1 && format_node) {
        Xvr_ASTNode* array_node = NULL;
        if (args->type == XVR_AST_NODE_COMPOUND &&
            args->compound.literalType == XVR_LITERAL_ARRAY) {
            array_node = args;
        } else if (args->type == XVR_AST_NODE_FN_COLLECTION &&
                   args->fnCollection.count > 0 &&
                   args->fnCollection.nodes[0].type == XVR_AST_NODE_COMPOUND &&
                   args->fnCollection.nodes[0].compound.literalType ==
                       XVR_LITERAL_ARRAY) {
            array_node = &args->fnCollection.nodes[0];
        }

        if (array_node) {
            Xvr_LLVMContextSetError(
                emitter->context,
                "println: cannot print array directly. Use format string: "
                "std::println(\"{}\", array)");
            return NULL;
        }

        LLVMValueRef arg_val =
            Xvr_LLVMExpressionEmitterEmit(emitter, format_node);
        if (arg_val) {
            LLVMTypeRef arg_type = LLVMTypeOf(arg_val);
            const char* fmt_str = "%s\n";
            LLVMValueRef print_arg = arg_val;
            LLVMTypeKind kind = LLVMGetTypeKind(arg_type);
            if (kind == LLVMIntegerTypeKind) {
                Xvr_LLVMContextSetError(
                    emitter->context,
                    "println: use format string for non-string types: "
                    "std::println(\"{}\", value)");
                return NULL;
            } else if (kind == LLVMFloatTypeKind) {
                Xvr_LLVMContextSetError(
                    emitter->context,
                    "println: use format string for non-string types: "
                    "std::println(\"{}\", value)");
                return NULL;
            } else if (kind == LLVMDoubleTypeKind) {
                Xvr_LLVMContextSetError(
                    emitter->context,
                    "println: use format string for non-string types: "
                    "std::println(\"{}\", value)");
                return NULL;
            } else if (kind == LLVMArrayTypeKind) {
                Xvr_LLVMContextSetError(
                    emitter->context,
                    "println: cannot print array directly. Use format string: "
                    "std::println(\"{}\", array)");
                return NULL;
            } else if (kind == LLVMPointerTypeKind) {
                fmt_str = "%s\n";
            }
            LLVMValueRef fmt_global =
                LLVMBuildGlobalStringPtr(llvm_builder, fmt_str, "fmt_str");
            LLVMValueRef all_args[2] = {fmt_global, print_arg};
            LLVMTypeRef printf_type = LLVMFunctionType(
                LLVMInt32TypeInContext(llvm_ctx),
                (LLVMTypeRef[]){
                    LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0),
                    arg_type},
                2, true);
            LLVMBuildCall2(llvm_builder, printf_type, printf_fn, all_args, 2,
                           "printf_call");
        }
        return NULL;
    }

    if (arg_count > 0) {
        call_args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * arg_count);
        if (!call_args) {
            Xvr_LLVMContextSetError(emitter->context,
                                    "println: memory allocation failed");
            return NULL;
        }
        for (int i = 0; i < arg_count; i++) {
            Xvr_ASTNode* arg = NULL;
            if (args->type == XVR_AST_NODE_COMPOUND) {
                arg = &args->compound.nodes[i + 1];
            } else {
                arg = &args->fnCollection.nodes[i + 1];
            }
            LLVMValueRef arg_val = Xvr_LLVMExpressionEmitterEmit(emitter, arg);
            if (arg_val) {
                call_args[i] = arg_val;
            } else {
                call_args[i] =
                    LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, false);
            }
        }
    }

    if (has_format_string && arg_count > 0) {
        XvrFormatString* fmt = XvrFormatStringParse(format_str, arg_count);
        if (!fmt) {
            Xvr_LLVMContextSetError(emitter->context,
                                    "println: failed to parse format string");
            free(call_args);
            return NULL;
        }

        if (!fmt->is_valid) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                     "println: invalid format string: %s",
                     fmt->error_message ? fmt->error_message : "unknown error");
            Xvr_LLVMContextSetError(emitter->context, error_msg);
            XvrFormatStringFree(fmt);
            free(call_args);
            return NULL;
        }

        if (fmt->placeholder_count != (size_t)arg_count) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg),
                     "println: format placeholder count (%u) does not "
                     "match argument count (%d)",
                     (unsigned int)fmt->placeholder_count, arg_count);
            Xvr_LLVMContextSetError(emitter->context, error_msg);
            XvrFormatStringFree(fmt);
            free(call_args);
            return NULL;
        }

        char* printf_fmt_with_newline = NULL;
        if (asprintf(&printf_fmt_with_newline, "%s\n",
                     fmt->printf_format ? fmt->printf_format : "%s") == -1) {
            Xvr_LLVMContextSetError(emitter->context,
                                    "println: memory allocation failed");
            XvrFormatStringFree(fmt);
            free(call_args);
            return NULL;
        }

        XvrFormatArgType* arg_types =
            (XvrFormatArgType*)malloc(sizeof(XvrFormatArgType) * arg_count);
        if (!arg_types) {
            Xvr_LLVMContextSetError(emitter->context,
                                    "println: memory allocation failed");
            free(printf_fmt_with_newline);
            XvrFormatStringFree(fmt);
            free(call_args);
            return NULL;
        }

        for (int i = 0; i < arg_count; i++) {
            LLVMValueRef arg_val = call_args[i];
            LLVMTypeRef arg_type = LLVMTypeOf(arg_val);
            LLVMTypeKind kind = LLVMGetTypeKind(arg_type);
            if (kind == LLVMIntegerTypeKind) {
                arg_types[i] = XVR_FORMAT_ARG_INT;
            } else if (kind == LLVMFloatTypeKind) {
                arg_types[i] = XVR_FORMAT_ARG_FLOAT;
            } else if (kind == LLVMDoubleTypeKind) {
                arg_types[i] = XVR_FORMAT_ARG_DOUBLE;
            } else if (kind == LLVMPointerTypeKind) {
                arg_types[i] = XVR_FORMAT_ARG_STRING;
            } else if (kind == LLVMArrayTypeKind) {
                Xvr_LLVMContextSetError(
                    emitter->context,
                    "println: cannot use array with format string. "
                    "Use std::println(array) without format string, or "
                    "print elements individually");
                free(arg_types);
                free(printf_fmt_with_newline);
                XvrFormatStringFree(fmt);
                free(call_args);
                return NULL;
            } else {
                arg_types[i] = XVR_FORMAT_ARG_STRING;
            }
        }

        char* printf_fmt =
            XvrFormatStringBuildPrintfFormat(fmt, arg_types, arg_count);
        char* printf_fmt_final = NULL;
        if (asprintf(&printf_fmt_final, "%s\n", printf_fmt) == -1) {
            Xvr_LLVMContextSetError(emitter->context,
                                    "println: memory allocation failed");
            free(printf_fmt_with_newline);
            free(arg_types);
            XvrFormatStringFree(fmt);
            free(call_args);
            return NULL;
        }

        LLVMValueRef fmt_global =
            LLVMBuildGlobalStringPtr(llvm_builder, printf_fmt_final, "fmt_str");

        LLVMValueRef* all_args =
            (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * (arg_count + 1));
        if (!all_args) {
            Xvr_LLVMContextSetError(emitter->context,
                                    "println: memory allocation failed");
            free(printf_fmt_final);
            free(printf_fmt_with_newline);
            free(arg_types);
            XvrFormatStringFree(fmt);
            free(call_args);
            return NULL;
        }

        all_args[0] = fmt_global;
        for (int i = 0; i < arg_count; i++) {
            LLVMValueRef arg = call_args[i];
            LLVMTypeRef arg_type = LLVMTypeOf(arg);
            if (LLVMGetTypeKind(arg_type) == LLVMFloatTypeKind) {
                all_args[i + 1] = LLVMBuildFPExt(
                    llvm_builder, arg, LLVMDoubleTypeInContext(llvm_ctx),
                    "float_to_double");
            } else {
                all_args[i + 1] = arg;
            }
        }

        LLVMTypeRef* param_types =
            (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * (arg_count + 1));
        if (!param_types) {
            Xvr_LLVMContextSetError(emitter->context,
                                    "println: memory allocation failed");
            free(all_args);
            free(printf_fmt_final);
            free(printf_fmt_with_newline);
            free(arg_types);
            XvrFormatStringFree(fmt);
            free(call_args);
            return NULL;
        }

        param_types[0] = LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0);
        for (int i = 0; i < arg_count; i++) {
            LLVMValueRef arg = call_args[i];
            LLVMTypeRef arg_type = LLVMTypeOf(arg);
            if (LLVMGetTypeKind(arg_type) == LLVMFloatTypeKind) {
                param_types[i + 1] = LLVMDoubleTypeInContext(llvm_ctx);
            } else {
                param_types[i + 1] = arg_type;
            }
        }

        LLVMTypeRef printf_type = LLVMFunctionType(
            LLVMInt32TypeInContext(llvm_ctx), param_types, arg_count + 1, true);
        LLVMBuildCall2(llvm_builder, printf_type, printf_fn, all_args,
                       arg_count + 1, "printf_call");

        free(all_args);
        free(param_types);
        free(printf_fmt);
        free(printf_fmt_final);
        free(printf_fmt_with_newline);
        free(arg_types);
        XvrFormatStringFree(fmt);
    } else if (!has_format_string) {
        Xvr_LLVMContextSetError(emitter->context,
                                "println: format string required");
        if (call_args) free(call_args);
        return NULL;
    } else {
        char* final_fmt = NULL;
        if (format_str) {
            if (asprintf(&final_fmt, "%s\n", format_str) == -1) {
                Xvr_LLVMContextSetError(emitter->context,
                                        "println: memory allocation failed");
                free(call_args);
                return NULL;
            }
        } else {
            final_fmt = strdup("%s\n");
            if (!final_fmt) {
                Xvr_LLVMContextSetError(emitter->context,
                                        "println: memory allocation failed");
                free(call_args);
                return NULL;
            }
        }
        LLVMValueRef fmt_global =
            LLVMBuildGlobalStringPtr(llvm_builder, final_fmt, "fmt_str");
        LLVMTypeRef printf_type =
            LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx), NULL, 0, true);
        LLVMBuildCall2(llvm_builder, printf_type, printf_fn, &fmt_global, 1,
                       "printf_call");
        free(final_fmt);
    }

    if (call_args) {
        free(call_args);
    }

    return NULL;
}

static LLVMValueRef emit_max(Xvr_LLVMExpressionEmitter* emitter,
                             Xvr_ASTNode* args) {
    if (!emitter || !args) {
        return NULL;
    }

    Xvr_LLVMIRBuilder* builder = emitter->builder;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);

    int arg_count = 0;
    LLVMValueRef args_list[8] = {NULL};

    if (args->type == XVR_AST_NODE_COMPOUND) {
        arg_count = args->compound.count;
        if (arg_count < 1 || arg_count > 8) {
            Xvr_LLVMContextSetError(emitter->context,
                                    "max: requires 1-8 arguments");
            return NULL;
        }
        for (int i = 0; i < arg_count; i++) {
            args_list[i] = Xvr_LLVMExpressionEmitterEmit(
                emitter, &args->compound.nodes[i]);
            if (!args_list[i]) {
                Xvr_LLVMContextSetError(emitter->context,
                                        "max: invalid argument");
                return NULL;
            }
        }
    } else if (args->type == XVR_AST_NODE_FN_COLLECTION) {
        arg_count = args->fnCollection.count;
        if (arg_count < 1 || arg_count > 8) {
            Xvr_LLVMContextSetError(emitter->context,
                                    "max: requires 1-8 arguments");
            return NULL;
        }
        for (int i = 0; i < arg_count; i++) {
            args_list[i] = Xvr_LLVMExpressionEmitterEmit(
                emitter, &args->fnCollection.nodes[i]);
            if (!args_list[i]) {
                Xvr_LLVMContextSetError(emitter->context,
                                        "max: invalid argument");
                return NULL;
            }
        }
    } else {
        Xvr_LLVMContextSetError(emitter->context, "max: invalid arguments");
        return NULL;
    }

    LLVMTypeRef type0 = LLVMTypeOf(args_list[0]);
    LLVMTypeKind kind0 = LLVMGetTypeKind(type0);

    if (kind0 == LLVMIntegerTypeKind) {
        switch (arg_count) {
        case 1:
            return args_list[0];
        case 2: {
            LLVMValueRef cond =
                LLVMBuildICmp(llvm_builder, LLVMIntSGT, args_list[0],
                              args_list[1], "max_cmp");
            return LLVMBuildSelect(llvm_builder, cond, args_list[0],
                                   args_list[1], "max_result");
        }
        case 3: {
            LLVMValueRef tmp = args_list[0];
            LLVMValueRef c1 = LLVMBuildICmp(llvm_builder, LLVMIntSGT, tmp,
                                            args_list[1], "max_cmp1");
            tmp = LLVMBuildSelect(llvm_builder, c1, tmp, args_list[1],
                                  "max_tmp1");
            LLVMValueRef c2 = LLVMBuildICmp(llvm_builder, LLVMIntSGT, tmp,
                                            args_list[2], "max_cmp2");
            return LLVMBuildSelect(llvm_builder, c2, tmp, args_list[2],
                                   "max_result");
        }
        case 4: {
            LLVMValueRef tmp = args_list[0];
            LLVMValueRef c1 = LLVMBuildICmp(llvm_builder, LLVMIntSGT, tmp,
                                            args_list[1], "max_cmp1");
            tmp = LLVMBuildSelect(llvm_builder, c1, tmp, args_list[1],
                                  "max_tmp1");
            LLVMValueRef c2 = LLVMBuildICmp(llvm_builder, LLVMIntSGT, tmp,
                                            args_list[2], "max_cmp2");
            tmp = LLVMBuildSelect(llvm_builder, c2, tmp, args_list[2],
                                  "max_tmp2");
            LLVMValueRef c3 = LLVMBuildICmp(llvm_builder, LLVMIntSGT, tmp,
                                            args_list[3], "max_cmp3");
            return LLVMBuildSelect(llvm_builder, c3, tmp, args_list[3],
                                   "max_result");
        }
        default: {
            LLVMValueRef max_val = args_list[0];
            for (int i = 1; i < arg_count; i++) {
                LLVMValueRef cond = LLVMBuildICmp(
                    llvm_builder, LLVMIntSGT, max_val, args_list[i], "max_cmp");
                max_val = LLVMBuildSelect(llvm_builder, cond, max_val,
                                          args_list[i], "max_result");
            }
            return max_val;
        }
        }
    }

    if (kind0 == LLVMFloatTypeKind || kind0 == LLVMDoubleTypeKind) {
        switch (arg_count) {
        case 1:
            return args_list[0];
        case 2: {
            LLVMValueRef cond =
                LLVMBuildFCmp(llvm_builder, LLVMRealOGT, args_list[0],
                              args_list[1], "max_cmp");
            return LLVMBuildSelect(llvm_builder, cond, args_list[0],
                                   args_list[1], "max_result");
        }
        case 3: {
            LLVMValueRef tmp = args_list[0];
            LLVMValueRef c1 = LLVMBuildFCmp(llvm_builder, LLVMRealOGT, tmp,
                                            args_list[1], "max_cmp1");
            tmp = LLVMBuildSelect(llvm_builder, c1, tmp, args_list[1],
                                  "max_tmp1");
            LLVMValueRef c2 = LLVMBuildFCmp(llvm_builder, LLVMRealOGT, tmp,
                                            args_list[2], "max_cmp2");
            return LLVMBuildSelect(llvm_builder, c2, tmp, args_list[2],
                                   "max_result");
        }
        case 4: {
            LLVMValueRef tmp = args_list[0];
            LLVMValueRef c1 = LLVMBuildFCmp(llvm_builder, LLVMRealOGT, tmp,
                                            args_list[1], "max_cmp1");
            tmp = LLVMBuildSelect(llvm_builder, c1, tmp, args_list[1],
                                  "max_tmp1");
            LLVMValueRef c2 = LLVMBuildFCmp(llvm_builder, LLVMRealOGT, tmp,
                                            args_list[2], "max_cmp2");
            tmp = LLVMBuildSelect(llvm_builder, c2, tmp, args_list[2],
                                  "max_tmp2");
            LLVMValueRef c3 = LLVMBuildFCmp(llvm_builder, LLVMRealOGT, tmp,
                                            args_list[3], "max_cmp3");
            return LLVMBuildSelect(llvm_builder, c3, tmp, args_list[3],
                                   "max_result");
        }
        default: {
            LLVMValueRef max_val = args_list[0];
            for (int i = 1; i < arg_count; i++) {
                LLVMValueRef cond =
                    LLVMBuildFCmp(llvm_builder, LLVMRealOGT, max_val,
                                  args_list[i], "max_cmp");
                max_val = LLVMBuildSelect(llvm_builder, cond, max_val,
                                          args_list[i], "max_result");
            }
            return max_val;
        }
        }
    }

    Xvr_LLVMContextSetError(
        emitter->context,
        "max: unsupported type - only int and float are supported");
    return NULL;
}

/**
 * @brief Maps XVR unary opcode to LLVM instruction
 * @param emitter Expression emitter
 * @param opcode XVR opcode
 * @param operand Operand value
 * @return LLVM value, or NULL if unsupported
 *
 * Supports:
 * - NOT/INVERT: boolean negation
 * - NEGATE: arithmetic negation
 */
static LLVMValueRef emit_unary_op(Xvr_LLVMExpressionEmitter* emitter,
                                  Xvr_Opcode opcode, LLVMValueRef operand) {
    Xvr_LLVMIRBuilder* builder = emitter->builder;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);

    switch (opcode) {
    case XVR_OP_INVERT: {
        /* Boolean NOT: compare operand to zero */
        LLVMValueRef zero = LLVMConstNull(LLVMInt1TypeInContext(llvm_ctx));
        return Xvr_LLVMIRBuilderCreateICmpNE(builder, operand, zero, "not");
    }

    case XVR_OP_NEGATE: {
        /* Arithmetic negation: 0 - operand */
        LLVMValueRef zero = LLVMConstNull(LLVMInt32TypeInContext(llvm_ctx));
        return Xvr_LLVMIRBuilderCreateSub(builder, zero, operand, "neg");
    }

    case XVR_OP_PRINT: {
        /* For print statements, emit a call to printf function with format
         * string */
        if (!operand) {
            return NULL;
        }
        LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);
        LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
        LLVMContextRef llvm_ctx =
            Xvr_LLVMContextGetLLVMContext(emitter->context);

        /* Get or create printf function */
        LLVMValueRef callee = LLVMGetNamedFunction(module, "printf");
        LLVMTypeRef printf_type =
            LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                             (LLVMTypeRef[]){LLVMPointerType(
                                 LLVMInt8TypeInContext(llvm_ctx), 0)},
                             1, true);
        if (!callee) {
            callee = LLVMAddFunction(module, "printf", printf_type);
        }

        /* Create format string based on operand type */
        LLVMTypeRef operand_type = LLVMTypeOf(operand);
        const char* format_str;
        LLVMTypeKind type_kind = LLVMGetTypeKind(operand_type);

        if (type_kind == LLVMIntegerTypeKind) {
            format_str = "%d";
        } else if (type_kind == LLVMFloatTypeKind) {
            format_str = "%f";
        } else if (type_kind == LLVMDoubleTypeKind) {
            format_str = "%lf";
        } else if (type_kind == LLVMArrayTypeKind) {
            int array_count = LLVMGetArrayLength(operand_type);
            LLVMTypeRef elem_type = LLVMGetElementType(operand_type);
            LLVMTypeKind elem_kind = LLVMGetTypeKind(elem_type);
            const char* arr_fmt = "%d ";
            if (elem_kind == LLVMFloatTypeKind) {
                arr_fmt = "%f ";
            } else if (elem_kind == LLVMDoubleTypeKind) {
                arr_fmt = "%lf ";
            }
            return emit_array_print(emitter, operand, array_count, elem_type,
                                    arr_fmt);
        } else if (type_kind == LLVMPointerTypeKind) {
            LLVMTypeRef pointee_type = LLVMGetElementType(operand_type);
            LLVMTypeKind pointee_kind =
                pointee_type ? LLVMGetTypeKind(pointee_type) : 0;
            if (!pointee_type) {
                /* Try using the variable's allocated type instead */
                LLVMTypeRef operand_val_type = LLVMGetAllocatedType(operand);
                if (operand_val_type &&
                    LLVMGetTypeKind(operand_val_type) == LLVMArrayTypeKind) {
                    int array_count = LLVMGetArrayLength(operand_val_type);
                    LLVMTypeRef elem_type =
                        LLVMGetElementType(operand_val_type);
                    LLVMTypeKind elem_kind = LLVMGetTypeKind(elem_type);
                    const char* arr_fmt = "%d ";
                    if (elem_kind == LLVMFloatTypeKind) {
                        arr_fmt = "%f ";
                    } else if (elem_kind == LLVMDoubleTypeKind) {
                        arr_fmt = "%lf ";
                    }
                    return emit_array_print(emitter, operand, array_count,
                                            elem_type, arr_fmt);
                }
                format_str = "%s";
            } else {
                LLVMTypeKind pointee_kind = LLVMGetTypeKind(pointee_type);
                if (pointee_kind == LLVMArrayTypeKind) {
                    int array_count = LLVMGetArrayLength(pointee_type);
                    LLVMTypeRef elem_type = LLVMGetElementType(pointee_type);
                    LLVMTypeKind elem_kind = LLVMGetTypeKind(elem_type);
                    const char* arr_fmt = "%d ";
                    if (elem_kind == LLVMFloatTypeKind) {
                        arr_fmt = "%f ";
                    } else if (elem_kind == LLVMDoubleTypeKind) {
                        arr_fmt = "%lf ";
                    }
                    return emit_array_print(emitter, operand, array_count,
                                            elem_type, arr_fmt);
                } else if (pointee_kind == LLVMIntegerTypeKind) {
                    unsigned elem_bits = LLVMGetIntTypeWidth(pointee_type);
                    if (elem_bits == 8) {
                        format_str = "%s";
                    } else {
                        format_str = "%p";
                    }
                } else {
                    format_str = "%p";
                }
            }
        } else {
            format_str = "%p";
        }

        /* Create global string for format */
        LLVMValueRef format_global =
            LLVMBuildGlobalStringPtr(llvm_builder, format_str, "print_fmt");

        /* Call printf with format string and operand */
        LLVMValueRef args[] = {format_global, operand};
        return LLVMBuildCall2(llvm_builder, printf_type, callee, args, 2,
                              "print_call");
    }

    default:
        return NULL;
    }
}

LLVMValueRef Xvr_LLVMExpressionEmitterEmitUnary(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_NodeUnary* unary) {
    if (!emitter || !unary) {
        return NULL;
    }

    /* Recursively emit operand */
    LLVMValueRef operand = Xvr_LLVMExpressionEmitterEmit(emitter, unary->child);
    if (!operand) {
        return NULL;
    }

    /* Generate unary operation */
    return emit_unary_op(emitter, unary->opcode, operand);
}

static LLVMValueRef lookup_var(Xvr_LLVMExpressionEmitter* emitter,
                               const char* name) {
    if (!emitter || !emitter->fn_emitter) {
        return NULL;
    }
    Xvr_LLVMFunctionEmitter* fn_emitter =
        (Xvr_LLVMFunctionEmitter*)emitter->fn_emitter;
    return Xvr_LLVMFunctionEmitterLookupVar(fn_emitter, name);
}

static LLVMValueRef lookup_var_with_type(Xvr_LLVMExpressionEmitter* emitter,
                                         const char* name,
                                         Xvr_LiteralType* out_type) {
    if (!emitter || !emitter->fn_emitter || !out_type) {
        return NULL;
    }
    Xvr_LLVMFunctionEmitter* fn_emitter =
        (Xvr_LLVMFunctionEmitter*)emitter->fn_emitter;
    return Xvr_LLVMFunctionEmitterLookupVarWithType(fn_emitter, name, out_type);
}

LLVMValueRef Xvr_LLVMExpressionEmitterEmitIdentifier(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_Literal identifier) {
    if (!emitter || identifier.type != XVR_LITERAL_IDENTIFIER) {
        return NULL;
    }

    const char* var_name = NULL;
    if (identifier.as.string.ptr) {
        var_name = (const char*)identifier.as.string.ptr->data;
    }

    if (!var_name) {
        return NULL;
    }

    Xvr_LiteralType var_type_xvr = XVR_LITERAL_ANY;
    LLVMValueRef var_ptr =
        lookup_var_with_type(emitter, var_name, &var_type_xvr);
    if (!var_ptr) {
        return NULL;
    }

    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
    Xvr_LLVMTypeMapper* type_mapper = emitter->type_mapper;
    LLVMTypeRef var_type = NULL;

    /* For arrays, we should NOT use the type mapper as it returns pointer
     * types. Instead, get the type from the alloca directly. */
    if (type_mapper && var_type_xvr != XVR_LITERAL_ANY &&
        var_type_xvr != XVR_LITERAL_ARRAY) {
        var_type = Xvr_LLVMTypeMapperGetType(type_mapper, var_type_xvr);
    }

    if (!var_type) {
        LLVMTypeRef ptr_type = LLVMTypeOf(var_ptr);

        /* Try LLVMGetAllocatedType for allocas */
        LLVMTypeRef alloc_type = LLVMGetAllocatedType(var_ptr);

        if (alloc_type) {
            var_type = alloc_type;
        } else {
            var_type = LLVMGetElementType(ptr_type);
        }
    }

    if (!var_type) {
        var_type = LLVMInt32TypeInContext(llvm_ctx);
    }

    /* For arrays, don't load - return the pointer instead so GEP works */
    if (var_type && LLVMGetTypeKind(var_type) == LLVMArrayTypeKind) {
        return var_ptr;
    }

    LLVMBuilderRef builder = Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);
    return LLVMBuildLoad2(builder, var_type, var_ptr, var_name);
}

LLVMValueRef Xvr_LLVMExpressionEmitterEmitFnCall(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_NodeFnCall* fn_call) {
    if (!emitter || !fn_call) {
        return NULL;
    }

    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);
    LLVMBuilderRef llvm_builder =
        Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);

    Xvr_ASTNode* args_node = fn_call->arguments;
    if (!args_node) {
        return NULL;
    }

    const char* fn_name = "unknown";
    LLVMValueRef callee = NULL;

    if (args_node->type == XVR_AST_NODE_COMPOUND &&
        args_node->compound.count > 0) {
        Xvr_ASTNode* first = &args_node->compound.nodes[0];
        if (first->type == XVR_AST_NODE_BINARY &&
            first->binary.opcode == XVR_OP_FN_CALL) {
            Xvr_ASTNode* fn_identifier = first->binary.left;
            if (fn_identifier->type == XVR_AST_NODE_LITERAL &&
                fn_identifier->atomic.literal.type == XVR_LITERAL_IDENTIFIER) {
                fn_name = (const char*)
                              fn_identifier->atomic.literal.as.string.ptr->data;
            }
        }
    }

    callee = LLVMGetNamedFunction(module, fn_name);
    if (!callee) {
        return NULL;
    }

    int arg_count = 0;
    LLVMValueRef* args = NULL;

    if (args_node->type == XVR_AST_NODE_COMPOUND) {
        arg_count = args_node->compound.count;
        if (arg_count > 0) {
            args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * arg_count);
            for (int i = 0; i < arg_count; i++) {
                Xvr_ASTNode* arg = &args_node->compound.nodes[i];
                if (arg->type == XVR_AST_NODE_BINARY &&
                    arg->binary.opcode == XVR_OP_FN_CALL) {
                    args[i] = LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0,
                                           false);
                } else {
                    args[i] = Xvr_LLVMExpressionEmitterEmit(emitter, arg);
                    if (!args[i]) {
                        args[i] = LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx),
                                               0, false);
                    }
                }
            }
        }
    }

    LLVMValueRef result = NULL;
    if (callee) {
        LLVMTypeRef callee_type = LLVMTypeOf(callee);
        result = LLVMBuildCall2(llvm_builder, callee_type, callee, args,
                                arg_count, "");
    }

    if (args) {
        free(args);
    }

    return result;
}

/**
 * @brief Helper to get node type safely
 * @param node AST node
 * @return Node type, or ERROR if NULL
 */
static Xvr_ASTNodeType get_node_type(Xvr_ASTNode* node) {
    if (!node) {
        return XVR_AST_NODE_ERROR;
    }
    return node->type;
}

LLVMValueRef Xvr_LLVMExpressionEmitterEmit(Xvr_LLVMExpressionEmitter* emitter,
                                           Xvr_ASTNode* node) {
    if (!emitter || !node) {
        return NULL;
    }

    Xvr_ASTNodeType node_type = get_node_type(node);

    switch (node_type) {
    case XVR_AST_NODE_LITERAL:
        return Xvr_LLVMExpressionEmitterEmitLiteral(emitter, &node->atomic);

    case XVR_AST_NODE_BINARY:
        return Xvr_LLVMExpressionEmitterEmitBinary(emitter, &node->binary);

    case XVR_AST_NODE_UNARY:
        return Xvr_LLVMExpressionEmitterEmitUnary(emitter, &node->unary);

    case XVR_AST_NODE_FN_CALL:
        return Xvr_LLVMExpressionEmitterEmitFnCall(emitter, &node->fnCall);

    case XVR_AST_NODE_CAST: {
        Xvr_ASTNode* expr = node->cast.expression;
        Xvr_Literal target_type_literal = node->cast.targetType;

        LLVMValueRef value = Xvr_LLVMExpressionEmitterEmit(emitter, expr);
        if (!value || !emitter->cast_emitter) {
            return NULL;
        }

        LLVMTypeRef llvm_type = LLVMTypeOf(value);
        Xvr_LiteralType source_literal_type = XVR_LITERAL_INTEGER;
        LLVMTypeKind kind = LLVMGetTypeKind(llvm_type);
        if (kind == LLVMIntegerTypeKind) {
            unsigned bits = LLVMGetIntTypeWidth(llvm_type);
            if (bits == 1) {
                source_literal_type = XVR_LITERAL_BOOLEAN;
            } else if (bits == 8) {
                source_literal_type = XVR_LITERAL_INT8;
            } else if (bits == 16) {
                source_literal_type = XVR_LITERAL_INT16;
            } else if (bits == 32) {
                source_literal_type = XVR_LITERAL_INT32;
            } else if (bits == 64) {
                source_literal_type = XVR_LITERAL_INT64;
            }
        } else if (kind == LLVMFloatTypeKind) {
            source_literal_type = XVR_LITERAL_FLOAT32;
        } else if (kind == LLVMDoubleTypeKind) {
            source_literal_type = XVR_LITERAL_FLOAT64;
        } else if (kind == LLVMPointerTypeKind) {
            source_literal_type = XVR_LITERAL_STRING;
        }

        Xvr_Type* from_type = Xvr_TypeGetFromLiteral(source_literal_type);

        Xvr_LiteralType target_type;
        if (target_type_literal.type == XVR_LITERAL_TYPE) {
            target_type = target_type_literal.as.type.typeOf;
        } else {
            target_type = target_type_literal.type;
        }
        Xvr_Type* to_type = Xvr_TypeGetFromLiteral(target_type);

        if (!from_type || !to_type) {
            return NULL;
        }

        return Xvr_EmitCast(emitter->cast_emitter, value, from_type, to_type,
                            "cast");
    }

    case XVR_AST_NODE_IF:
        if (emitter->control_flow) {
            Xvr_LLVMControlFlowEmitIf(emitter->control_flow, &node->pathIf);
        }
        return NULL;

    case XVR_AST_NODE_WHILE:
        if (emitter->control_flow) {
            Xvr_LLVMControlFlowEmitWhile(emitter->control_flow,
                                         &node->pathWhile);
        }
        return NULL;

    case XVR_AST_NODE_FOR:
        if (emitter->control_flow) {
            Xvr_LLVMControlFlowEmitFor(emitter->control_flow, &node->pathFor);
        }
        return NULL;

    case XVR_AST_NODE_BREAK:
        if (emitter->control_flow) {
            Xvr_LLVMControlFlowEmitBreak(emitter->control_flow);
        }
        return NULL;

    case XVR_AST_NODE_CONTINUE:
        if (emitter->control_flow) {
            Xvr_LLVMControlFlowEmitContinue(emitter->control_flow);
        }
        return NULL;

    case XVR_AST_NODE_BLOCK:
        if (emitter->control_flow && node->block.nodes &&
            node->block.count > 0) {
            Xvr_LLVMFunctionEmitter* fn_emitter =
                (Xvr_LLVMFunctionEmitter*)emitter->fn_emitter;
            if (fn_emitter) {
                Xvr_LLVMFunctionEmitterEnterScope(fn_emitter);
            }
            for (int i = 0; i < node->block.count; i++) {
                Xvr_LLVMExpressionEmitterEmit(emitter, &node->block.nodes[i]);
            }
            if (fn_emitter) {
                Xvr_LLVMFunctionEmitterExitScope(fn_emitter);
            }
        }
        return NULL;

    case XVR_AST_NODE_VAR_DECL: {
        /* Handle variable declaration in blocks */
        Xvr_NodeVarDecl* varDecl = &node->varDecl;
        const char* var_name = NULL;
        if (varDecl->identifier.type == XVR_LITERAL_IDENTIFIER &&
            varDecl->identifier.as.string.ptr) {
            var_name = (const char*)varDecl->identifier.as.string.ptr->data;
        }

        if (var_name && varDecl->expression) {
            LLVMValueRef init_value =
                Xvr_LLVMExpressionEmitterEmit(emitter, varDecl->expression);
            if (init_value) {
                LLVMTypeRef var_type = LLVMTypeOf(init_value);
                LLVMValueRef alloca = Xvr_LLVMIRBuilderCreateAlloca(
                    emitter->builder, var_type, var_name);
                Xvr_LLVMIRBuilderCreateStore(emitter->builder, init_value,
                                             alloca);
                Xvr_LiteralType varType = XVR_LITERAL_INTEGER;
                int array_count = 0;
                if (varDecl->typeLiteral.type == XVR_LITERAL_TYPE) {
                    varType = XVR_AS_TYPE(varDecl->typeLiteral).typeOf;
                }
                /* Detect array type from expression */
                if (varDecl->expression->type == XVR_AST_NODE_COMPOUND) {
                    Xvr_NodeCompound* comp = &varDecl->expression->compound;
                    if (comp->literalType == XVR_LITERAL_ARRAY ||
                        comp->literalType == XVR_LITERAL_ANY) {
                        varType = XVR_LITERAL_ARRAY;
                        array_count = comp->count;
                    }
                }
                /* Add to function emitter's variable table */
                Xvr_LLVMFunctionEmitter* fn_emitter =
                    (Xvr_LLVMFunctionEmitter*)emitter->fn_emitter;
                if (fn_emitter) {
                    Xvr_LLVMFunctionEmitterAddLocalVar(
                        fn_emitter, var_name, alloca, varType, array_count);
                }
            }
        }
        return NULL;
    }

    case XVR_AST_NODE_COMPOUND: {
        Xvr_NodeCompound* compound = &node->compound;
        if (compound->count == 0) {
            return NULL;
        }
        LLVMContextRef llvm_ctx =
            Xvr_LLVMContextGetLLVMContext(emitter->context);

        if (compound->literalType == XVR_LITERAL_ARRAY ||
            compound->literalType == XVR_LITERAL_ANY) {
            LLVMValueRef first_elem =
                Xvr_LLVMExpressionEmitterEmit(emitter, &compound->nodes[0]);
            if (!first_elem) {
                return NULL;
            }
            LLVMTypeRef elem_type = LLVMTypeOf(first_elem);
            LLVMTypeKind elem_kind = LLVMGetTypeKind(elem_type);

            if (elem_kind == LLVMArrayTypeKind) {
                LLVMValueRef* values =
                    malloc(sizeof(LLVMValueRef) * compound->count);
                values[0] = first_elem;
                for (int i = 1; i < compound->count; i++) {
                    LLVMValueRef elem = Xvr_LLVMExpressionEmitterEmit(
                        emitter, &compound->nodes[i]);
                    if (!elem) {
                        free(values);
                        return NULL;
                    }
                    values[i] = elem;
                }
                LLVMValueRef array =
                    LLVMConstArray(elem_type, values, compound->count);
                free(values);
                return array;
            } else {
                LLVMTypeRef array_type =
                    LLVMArrayType(elem_type, compound->count);
                LLVMValueRef* values =
                    malloc(sizeof(LLVMValueRef) * compound->count);
                values[0] = first_elem;
                for (int i = 1; i < compound->count; i++) {
                    LLVMValueRef elem = Xvr_LLVMExpressionEmitterEmit(
                        emitter, &compound->nodes[i]);
                    if (!elem) {
                        free(values);
                        return NULL;
                    }
                    values[i] = elem;
                }
                LLVMValueRef array =
                    LLVMConstArray(elem_type, values, compound->count);
                free(values);
                return array;
            }
        }
        return NULL;
    }

    /* TODO: Consider using LLVMBuildSelect for constant conditions in ternary
     * to avoid generating unnecessary basic blocks when the condition
     * is a compile-time constant (e.g., true ? x : y becomes just x).
     */
    case XVR_AST_NODE_TERNARY: {
        Xvr_NodeTernary* ternary = &node->ternary;
        if (!ternary->condition || !ternary->thenPath || !ternary->elsePath) {
            return NULL;
        }

        LLVMValueRef condition =
            Xvr_LLVMExpressionEmitterEmit(emitter, ternary->condition);
        if (!condition) {
            return NULL;
        }
        LLVMTypeRef cond_type = LLVMTypeOf(condition);
        if (LLVMGetTypeKind(cond_type) != LLVMIntegerTypeKind ||
            LLVMGetIntTypeWidth(cond_type) != 1) {
            return NULL;
        }

        LLVMValueRef then_val =
            Xvr_LLVMExpressionEmitterEmit(emitter, ternary->thenPath);
        LLVMValueRef else_val =
            Xvr_LLVMExpressionEmitterEmit(emitter, ternary->elsePath);

        if (!then_val || !else_val) {
            return NULL;
        }

        LLVMValueRef current_fn =
            Xvr_LLVMExpressionEmitterGetCurrentFunction(emitter);
        if (!current_fn) return NULL;

        LLVMContextRef llvm_ctx =
            Xvr_LLVMContextGetLLVMContext(emitter->context);
        LLVMBuilderRef llvm_builder =
            Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);

        LLVMBasicBlockRef then_block =
            LLVMAppendBasicBlockInContext(llvm_ctx, current_fn, "ternary_then");
        LLVMBasicBlockRef else_block =
            LLVMAppendBasicBlockInContext(llvm_ctx, current_fn, "ternary_else");
        LLVMBasicBlockRef merge_block =
            LLVMAppendBasicBlockInContext(llvm_ctx, current_fn, "ternary_cont");

        LLVMBuildCondBr(llvm_builder, condition, then_block, else_block);

        LLVMPositionBuilderAtEnd(llvm_builder, then_block);
        LLVMBuildBr(llvm_builder, merge_block);

        LLVMPositionBuilderAtEnd(llvm_builder, else_block);
        LLVMBuildBr(llvm_builder, merge_block);

        LLVMPositionBuilderAtEnd(llvm_builder, merge_block);

        LLVMTypeRef result_type = LLVMTypeOf(then_val);
        LLVMValueRef phi =
            LLVMBuildPhi(llvm_builder, result_type, "ternary_result");

        LLVMPositionBuilderAtEnd(llvm_builder, then_block);
        LLVMAddIncoming(phi, &then_val, &then_block, 1);

        LLVMPositionBuilderAtEnd(llvm_builder, else_block);
        LLVMAddIncoming(phi, &else_val, &else_block, 1);

        LLVMPositionBuilderAtEnd(llvm_builder, merge_block);

        return phi;
    }

    /* Grouping expression - unwrap and emit the child */
    case XVR_AST_NODE_GROUPING:
        if (node->grouping.child) {
            return Xvr_LLVMExpressionEmitterEmit(emitter, node->grouping.child);
        }
        return NULL;

    /* Array index access */
    case XVR_AST_NODE_INDEX: {
        Xvr_NodeIndex* index_node = &node->index;
        if (!index_node->first || !index_node->second) {
            return NULL;
        }

        /* Handle array indexing for identifiers like arr[0] */
        if (index_node->first->type == XVR_AST_NODE_LITERAL &&
            index_node->first->atomic.literal.type == XVR_LITERAL_IDENTIFIER) {
            const char* var_name =
                (const char*)
                    index_node->first->atomic.literal.as.string.ptr->data;

            Xvr_LiteralType var_type_xvr = XVR_LITERAL_ANY;
            LLVMValueRef var_ptr =
                lookup_var_with_type(emitter, var_name, &var_type_xvr);
            if (!var_ptr) {
                return NULL;
            }

            /* Verify this is an array type */
            if (var_type_xvr != XVR_LITERAL_ARRAY) {
                return NULL;
            }

            LLVMValueRef index =
                Xvr_LLVMExpressionEmitterEmit(emitter, index_node->second);
            if (!index) {
                return NULL;
            }

            LLVMBuilderRef llvm_builder =
                Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);
            LLVMTypeRef ptr_type = LLVMTypeOf(var_ptr);
            if (!ptr_type || LLVMGetTypeKind(ptr_type) != LLVMPointerTypeKind) {
                return NULL;
            }

            /* For alloca, use LLVMGetAllocatedType to get the array type */
            LLVMTypeRef elem_type = LLVMGetAllocatedType(var_ptr);
            if (!elem_type) {
                /* Fallback to LLVMGetElementType */
                elem_type = LLVMGetElementType(ptr_type);
            }

            if (!elem_type) {
                return NULL;
            }

            LLVMValueRef indices[] = {
                LLVMConstInt(
                    LLVMInt32TypeInContext(
                        Xvr_LLVMContextGetLLVMContext(emitter->context)),
                    0, false),
                index};
            LLVMValueRef elem_ptr = LLVMBuildInBoundsGEP2(
                llvm_builder, elem_type, var_ptr, indices, 2, "array_idx");
            LLVMTypeRef elem_elem_type = LLVMGetElementType(elem_type);
            return Xvr_LLVMIRBuilderCreateLoad(emitter->builder, elem_elem_type,
                                               elem_ptr, "array_elem");
        }

        /* Handle general index expressions (e.g., matrix[0][0]) */
        LLVMValueRef base =
            Xvr_LLVMExpressionEmitterEmit(emitter, index_node->first);
        if (!base) {
            return NULL;
        }
        LLVMValueRef index =
            Xvr_LLVMExpressionEmitterEmit(emitter, index_node->second);
        if (!index) {
            return NULL;
        }
        LLVMBuilderRef llvm_builder =
            Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);
        LLVMTypeRef base_type = LLVMTypeOf(base);

        /* Check if base is a pointer or an array (loaded from first index) */
        if (LLVMGetTypeKind(base_type) == LLVMPointerTypeKind) {
            LLVMTypeRef elem_type = LLVMGetElementType(base_type);
            if (!elem_type) {
                return NULL;
            }

            LLVMValueRef indices[] = {
                LLVMConstInt(
                    LLVMInt32TypeInContext(
                        Xvr_LLVMContextGetLLVMContext(emitter->context)),
                    0, false),
                index};
            LLVMValueRef elem_ptr = LLVMBuildInBoundsGEP2(
                llvm_builder, elem_type, base, indices, 2, "array_idx");

            /* Get element type of the array for loading */
            LLVMTypeRef elem_elem_type = LLVMGetElementType(elem_type);
            if (!elem_elem_type) {
                /* Single element (not an array), load with elem_type */
                return Xvr_LLVMIRBuilderCreateLoad(emitter->builder, elem_type,
                                                   elem_ptr, "array_elem");
            }
            return Xvr_LLVMIRBuilderCreateLoad(emitter->builder, elem_elem_type,
                                               elem_ptr, "array_elem");
        } else {
            /* Base is an array type (already loaded from first index like
             * matrix[0]) - need to store to stack to create pointer */
            LLVMTypeRef elem_type = base_type;
            LLVMTypeRef elem_elem_type = LLVMGetElementType(elem_type);
            if (!elem_elem_type) {
                return NULL;
            }

            /* Allocate stack space for the array */
            LLVMValueRef array_alloca =
                LLVMBuildAlloca(llvm_builder, elem_type, "temp_array");
            LLVMBuildStore(llvm_builder, base, array_alloca);

            /* Now use GEP to index into the array */
            LLVMValueRef zero = LLVMConstInt(
                LLVMInt32TypeInContext(
                    Xvr_LLVMContextGetLLVMContext(emitter->context)),
                0, false);
            LLVMValueRef elem_ptr =
                LLVMBuildInBoundsGEP2(llvm_builder, elem_elem_type,
                                      array_alloca, &index, 1, "array_idx");
            return Xvr_LLVMIRBuilderCreateLoad(emitter->builder, elem_elem_type,
                                               elem_ptr, "array_elem");
        }
    }

    /* Unsupported expression types */
    case XVR_AST_NODE_ERROR:
    case XVR_AST_NODE_PAIR:
    case XVR_AST_NODE_FN_DECL:
    case XVR_AST_NODE_FN_COLLECTION:
    case XVR_AST_NODE_FN_RETURN:
    case XVR_AST_NODE_PREFIX_INCREMENT:
    case XVR_AST_NODE_POSTFIX_INCREMENT:
    case XVR_AST_NODE_PREFIX_DECREMENT:
    case XVR_AST_NODE_POSTFIX_DECREMENT:
    case XVR_AST_NODE_IMPORT:
    case XVR_AST_NODE_PASS:
    default:
        return NULL;
    }
}
