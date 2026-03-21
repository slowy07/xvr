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
    /* Note: We don't destroy the referenced objects as we don't own them */
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

/**
 * @brief Maps XVR binary opcode to LLVM instruction
 * @param emitter Expression emitter
 * @param opcode XVR opcode
 * @param lhs Left-hand side value
 * @param rhs Right-hand side value
 * @return LLVM value, or NULL if unsupported
 *
 * Supports:
 * - Arithmetic: +, -, *, /, %
 * - Comparison: <, <=, >, >=, ==, !=
 */
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

/* Forward declaration for emit_array_print */
static LLVMValueRef emit_array_print(Xvr_LLVMExpressionEmitter* emitter,
                                     LLVMValueRef array_ptr, int array_count,
                                     LLVMTypeRef elem_type,
                                     const char* format_str);

static LLVMValueRef emit_printf(Xvr_LLVMExpressionEmitter* emitter,
                                Xvr_ASTNode* args) {
    if (!emitter || !args) {
        return NULL;
    }

    Xvr_LLVMIRBuilder* builder = emitter->builder;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);

    /* Count arguments */
    int arg_count = 0;
    if (args->type == XVR_AST_NODE_COMPOUND) {
        arg_count = args->compound.count;
    } else if (args->type == XVR_AST_NODE_FN_COLLECTION) {
        arg_count = args->fnCollection.count;
    }

    if (arg_count == 0) {
        return NULL;
    }

    /* Get the format string (first arg) - must be a literal for security */
    Xvr_ASTNode* format_arg = NULL;
    if (args->type == XVR_AST_NODE_COMPOUND) {
        format_arg = &args->compound.nodes[0];
    } else if (args->type == XVR_AST_NODE_FN_COLLECTION) {
        format_arg = &args->fnCollection.nodes[0];
    }
    const char* format_literal = NULL;

    if (format_arg->type == XVR_AST_NODE_LITERAL &&
        format_arg->atomic.literal.type == XVR_LITERAL_STRING &&
        format_arg->atomic.literal.as.string.ptr) {
        format_literal =
            (const char*)format_arg->atomic.literal.as.string.ptr->data;
    }

    /*
     * Security: Only parse format strings from LITERAL strings.
     * User-controlled strings are NEVER interpreted as format strings.
     * If the first argument is not a literal string, treat it as a regular
     * string and pass it directly to printf.
     */
    char* printf_format = NULL;
    uint32_t num_format_args = arg_count - 1;
    XvrFormatString* parsed = NULL;

    if (format_literal) {
        parsed = XvrFormatStringParse(format_literal, num_format_args);

        if (!parsed || !parsed->is_valid) {
            fprintf(stderr, "Format error: %s\n",
                    parsed && parsed->error_message ? parsed->error_message
                                                    : "unknown");
            if (parsed) {
                XvrFormatStringFree(parsed);
            }
            return NULL;
        }
    } else {
        LLVMValueRef format_str =
            Xvr_LLVMExpressionEmitterEmit(emitter, format_arg);
        if (!format_str) {
            return NULL;
        }

        LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");
        if (!printf_fn) {
            LLVMTypeRef printf_type =
                LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                                 (LLVMTypeRef[]){LLVMPointerType(
                                     LLVMInt8TypeInContext(llvm_ctx), 0)},
                                 1, true);
            printf_fn = LLVMAddFunction(module, "printf", printf_type);
        }

        int num_extra_args = arg_count - 1;
        LLVMValueRef* call_args = NULL;

        if (num_extra_args > 0) {
            call_args = malloc(sizeof(LLVMValueRef) * (num_extra_args + 1));
            call_args[0] = format_str;
            for (int i = 1; i < arg_count; i++) {
                call_args[i] = Xvr_LLVMExpressionEmitterEmit(
                    emitter, &args->compound.nodes[i]);
            }
        } else {
            call_args = &format_str;
        }

        LLVMTypeRef printf_type =
            LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                             (LLVMTypeRef[]){LLVMPointerType(
                                 LLVMInt8TypeInContext(llvm_ctx), 0)},
                             1, true);

        LLVMValueRef result =
            LLVMBuildCall2(llvm_builder, printf_type, printf_fn, call_args,
                           num_extra_args > 0 ? arg_count : 1, "printf_call");

        if (call_args && num_extra_args > 0) {
            free(call_args);
        }

        return result;
    }

    if (num_format_args == 0) {
        printf_format = XvrFormatStringGetPrintfFormat(parsed);
        LLVMValueRef format_global =
            LLVMBuildGlobalStringPtr(llvm_builder, printf_format, "fmt_str");
        free(printf_format);

        LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");
        if (!printf_fn) {
            LLVMTypeRef printf_type =
                LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                                 (LLVMTypeRef[]){LLVMPointerType(
                                     LLVMInt8TypeInContext(llvm_ctx), 0)},
                                 1, true);
            printf_fn = LLVMAddFunction(module, "printf", printf_type);
        }

        LLVMValueRef call_args[] = {format_global};
        LLVMTypeRef printf_type =
            LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                             (LLVMTypeRef[]){LLVMPointerType(
                                 LLVMInt8TypeInContext(llvm_ctx), 0)},
                             1, true);

        LLVMValueRef result = LLVMBuildCall2(
            llvm_builder, printf_type, printf_fn, call_args, 1, "printf_call");

        XvrFormatStringFree(parsed);
        return result;
    }

    LLVMValueRef* arg_values = malloc(sizeof(LLVMValueRef) * num_format_args);
    XvrFormatArgType* arg_types =
        malloc(sizeof(XvrFormatArgType) * num_format_args);

    for (uint32_t i = 0; i < num_format_args; i++) {
        arg_values[i] = Xvr_LLVMExpressionEmitterEmit(
            emitter, &args->compound.nodes[i + 1]);
        if (arg_values[i]) {
            LLVMTypeRef llvm_type = LLVMTypeOf(arg_values[i]);
            LLVMTypeKind kind = LLVMGetTypeKind(llvm_type);
            if (kind == LLVMPointerTypeKind) {
                arg_types[i] = XVR_FORMAT_ARG_STRING;
            } else if (kind == LLVMFloatTypeKind) {
                arg_types[i] = XVR_FORMAT_ARG_FLOAT;
                LLVMValueRef converted = LLVMBuildFPExt(
                    llvm_builder, arg_values[i],
                    LLVMDoubleTypeInContext(llvm_ctx), "float_to_double");
                arg_values[i] = converted;
            } else if (kind == LLVMDoubleTypeKind) {
                arg_types[i] = XVR_FORMAT_ARG_DOUBLE;
            } else if (kind == LLVMIntegerTypeKind) {
                unsigned bits = LLVMGetIntTypeWidth(llvm_type);
                if (bits == 1) {
                    arg_types[i] = XVR_FORMAT_ARG_BOOL;
                } else {
                    arg_types[i] = XVR_FORMAT_ARG_INT;
                }
            } else if (kind == LLVMArrayTypeKind) {
                /* For arrays in format strings, use %p (fallback) */
                arg_types[i] = XVR_FORMAT_ARG_POINTER;
            } else {
                arg_types[i] = XVR_FORMAT_ARG_INT;
            }
        } else {
            arg_types[i] = XVR_FORMAT_ARG_INT;
        }
    }

    printf_format =
        XvrFormatStringBuildPrintfFormat(parsed, arg_types, num_format_args);
    XvrFormatStringFree(parsed);

    LLVMValueRef format_global =
        LLVMBuildGlobalStringPtr(llvm_builder, printf_format, "fmt_str");
    free(printf_format);

    LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");
    if (!printf_fn) {
        LLVMTypeRef printf_type =
            LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                             (LLVMTypeRef[]){LLVMPointerType(
                                 LLVMInt8TypeInContext(llvm_ctx), 0)},
                             1, true);
        printf_fn = LLVMAddFunction(module, "printf", printf_type);
    }

    LLVMValueRef* call_args =
        malloc(sizeof(LLVMValueRef) * (num_format_args + 1));
    call_args[0] = format_global;
    for (uint32_t i = 0; i < num_format_args; i++) {
        call_args[i + 1] = arg_values[i];
    }

    LLVMTypeRef printf_type = LLVMFunctionType(
        LLVMInt32TypeInContext(llvm_ctx),
        (LLVMTypeRef[]){LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0)}, 1,
        true);

    LLVMValueRef result =
        LLVMBuildCall2(llvm_builder, printf_type, printf_fn, call_args,
                       num_format_args + 1, "printf_call");

    free(arg_values);
    free(arg_types);
    free(call_args);

    return result;
}

static LLVMValueRef emit_array_print(Xvr_LLVMExpressionEmitter* emitter,
                                     LLVMValueRef array_ptr, int array_count,
                                     LLVMTypeRef elem_type,
                                     const char* fmt_str) {
    if (!emitter || !array_ptr || array_count <= 0) {
        return NULL;
    }

    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
    LLVMBuilderRef builder = Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);
    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);

    LLVMTypeRef i32_type = LLVMInt32TypeInContext(llvm_ctx);
    LLVMTypeRef i8_type = LLVMInt8TypeInContext(llvm_ctx);
    LLVMTypeRef i8_ptr_type = LLVMPointerType(i8_type, 0);

    LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");
    if (!printf_fn) {
        LLVMTypeRef printf_type =
            LLVMFunctionType(i32_type, (LLVMTypeRef[]){i8_ptr_type}, 1, true);
        printf_fn = LLVMAddFunction(module, "printf", printf_type);
    }

    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
    LLVMValueRef func = LLVMGetBasicBlockParent(current_block);

    LLVMBasicBlockRef loop_cond =
        LLVMAppendBasicBlockInContext(llvm_ctx, func, "print_arr_cond");
    LLVMBasicBlockRef loop_body =
        LLVMAppendBasicBlockInContext(llvm_ctx, func, "print_arr_body");
    LLVMBasicBlockRef loop_end =
        LLVMAppendBasicBlockInContext(llvm_ctx, func, "print_arr_end");

    LLVMValueRef i = LLVMBuildAlloca(builder, i32_type, "i");
    LLVMBuildStore(builder, LLVMConstNull(i32_type), i);
    LLVMBuildBr(builder, loop_cond);

    LLVMPositionBuilderAtEnd(builder, loop_cond);
    LLVMValueRef i_val = LLVMBuildLoad2(builder, i32_type, i, "i_val");
    LLVMValueRef cmp =
        LLVMBuildICmp(builder, LLVMIntSLT, i_val,
                      LLVMConstInt(i32_type, array_count, false), "cmp");
    LLVMBuildCondBr(builder, cmp, loop_body, loop_end);

    LLVMPositionBuilderAtEnd(builder, loop_body);
    LLVMValueRef zero = LLVMConstNull(i32_type);
    LLVMValueRef indices[2] = {zero, i_val};
    LLVMTypeRef array_type = LLVMArrayType(elem_type, array_count);
    LLVMValueRef elem_ptr =
        LLVMBuildGEP2(builder, array_type, array_ptr, indices, 2, "elem_ptr");
    LLVMValueRef elem_val =
        LLVMBuildLoad2(builder, elem_type, elem_ptr, "elem_val");

    LLVMValueRef fmt = LLVMBuildGlobalStringPtr(builder, fmt_str, "fmt");

    LLVMTypeRef printf_arg_type = elem_type;
    LLVMValueRef printf_elem_val = elem_val;

    if (LLVMGetTypeKind(elem_type) == LLVMFloatTypeKind) {
        printf_arg_type = LLVMDoubleTypeInContext(llvm_ctx);
        printf_elem_val = LLVMBuildFPExt(builder, elem_val, printf_arg_type,
                                         "float_to_double");
    }

    LLVMBuildCall2(
        builder,
        LLVMFunctionType(
            i32_type, (LLVMTypeRef[]){i8_ptr_type, printf_arg_type}, 2, true),
        printf_fn, (LLVMValueRef[]){fmt, printf_elem_val}, 2, "print_elem");

    LLVMValueRef i_next = LLVMBuildAdd(
        builder, i_val, LLVMConstInt(i32_type, 1, false), "i_next");
    LLVMBuildStore(builder, i_next, i);
    LLVMBuildBr(builder, loop_cond);

    LLVMPositionBuilderAtEnd(builder, loop_end);

    return LLVMConstNull(i32_type);
}

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

            LLVMModuleRef module =
                Xvr_LLVMModuleManagerGetModule(emitter->module);
            LLVMValueRef callee = LLVMGetNamedFunction(module, fn_name);
            if (callee) {
                LLVMBuilderRef llvm_builder =
                    Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);

                int arg_count = 0;
                LLVMValueRef* args = NULL;

                if (binary->right) {
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
                    }
                }

                LLVMTypeRef callee_type = LLVMTypeOf(callee);
                LLVMValueRef result = LLVMBuildCall2(
                    llvm_builder, callee_type, callee, args, arg_count, "");

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
                            /* This is std::print - route to emit_printf */
                            return emit_printf(emitter,
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

                /* Get the index */
                LLVMValueRef index_val =
                    Xvr_LLVMExpressionEmitterEmit(emitter, index_node->second);
                if (!index_val) {
                    return NULL;
                }

                /* Evaluate the right side */
                LLVMValueRef rhs =
                    Xvr_LLVMExpressionEmitterEmit(emitter, binary->right);
                if (!rhs) {
                    return NULL;
                }

                /* Get element pointer using GEP */
                LLVMContextRef llvm_ctx =
                    Xvr_LLVMContextGetLLVMContext(emitter->context);
                LLVMBuilderRef builder =
                    Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);
                LLVMTypeRef alloc_type = LLVMGetAllocatedType(var_ptr);

                if (alloc_type &&
                    LLVMGetTypeKind(alloc_type) == LLVMArrayTypeKind) {
                    LLVMTypeRef elem_type = LLVMGetElementType(alloc_type);
                    LLVMValueRef zero =
                        LLVMConstNull(LLVMInt32TypeInContext(llvm_ctx));
                    LLVMValueRef indices[2] = {zero, index_val};
                    LLVMValueRef elem_ptr = LLVMBuildGEP2(
                        builder, alloc_type, var_ptr, indices, 2, "elem_ptr");
                    Xvr_LLVMIRBuilderCreateStore(emitter->builder, rhs,
                                                 elem_ptr);
                    return rhs;
                }
            }
            return NULL;
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

    case XVR_AST_NODE_IF:
        if (emitter->control_flow) {
            Xvr_LLVMControlFlowEmitIf(emitter->control_flow, &node->pathIf);
            return Xvr_LLVMControlFlowGetLastExpressionResult(
                emitter->control_flow);
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
                if (varDecl->typeLiteral.type == XVR_LITERAL_TYPE) {
                    varType = XVR_AS_TYPE(varDecl->typeLiteral).typeOf;
                }
                /* Add to function emitter's variable table */
                Xvr_LLVMFunctionEmitter* fn_emitter =
                    (Xvr_LLVMFunctionEmitter*)emitter->fn_emitter;
                if (fn_emitter) {
                    Xvr_LLVMFunctionEmitterAddLocalVar(fn_emitter, var_name,
                                                       alloca, varType, 0);
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

    /* Unsupported expression types */
    case XVR_AST_NODE_GROUPING:
    case XVR_AST_NODE_INDEX:
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
