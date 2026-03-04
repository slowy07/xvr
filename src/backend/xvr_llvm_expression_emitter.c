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
        return LLVMConstNull(LLVMInt8TypeInContext(llvm_ctx));

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
        return Xvr_LLVMIRBuilderCreateSDiv(emitter->builder, lhs, rhs, "srem");

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
                                Xvr_ASTNode* args) {
    if (!emitter || !args) {
        return NULL;
    }

    Xvr_LLVMIRBuilder* builder = emitter->builder;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(emitter->context);
    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);

    /* Get or create printf function */
    LLVMValueRef printf_fn = LLVMGetNamedFunction(module, "printf");
    if (!printf_fn) {
        LLVMTypeRef printf_type =
            LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                             (LLVMTypeRef[]){LLVMPointerType(
                                 LLVMInt8TypeInContext(llvm_ctx), 0)},
                             1, true);
        printf_fn = LLVMAddFunction(module, "printf", printf_type);
    }

    /* Count arguments */
    int arg_count = 0;
    if (args->type == XVR_AST_NODE_COMPOUND) {
        arg_count = args->compound.count;
    }

    if (arg_count == 0) {
        return NULL;
    }

    /* Emit format string (first arg) */
    LLVMValueRef format_str =
        Xvr_LLVMExpressionEmitterEmit(emitter, &args->compound.nodes[0]);
    if (!format_str) {
        return NULL;
    }

    LLVMValueRef* call_args = NULL;
    int num_extra_args = arg_count - 1;
    if (num_extra_args > 0) {
        call_args = malloc(sizeof(LLVMValueRef) * (arg_count));
        call_args[0] = format_str;
        for (int i = 1; i < arg_count; i++) {
            call_args[i] = Xvr_LLVMExpressionEmitterEmit(
                emitter, &args->compound.nodes[i]);
        }
    }

    LLVMTypeRef printf_type = LLVMFunctionType(
        LLVMInt32TypeInContext(llvm_ctx),
        (LLVMTypeRef[]){LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0)}, 1,
        true);

    LLVMValueRef result =
        LLVMBuildCall2(llvm_builder, printf_type, printf_fn, call_args,
                       num_extra_args > 0 ? arg_count : 0, "printf_call");

    if (call_args) {
        free(call_args);
    }

    return result;
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
        /* For print statements, emit a call to printf function */
        LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(emitter->module);
        LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
        LLVMValueRef callee = LLVMGetNamedFunction(module, "printf");
        LLVMTypeRef printf_type =
            LLVMFunctionType(LLVMInt32TypeInContext(llvm_ctx),
                             (LLVMTypeRef[]){LLVMPointerType(
                                 LLVMInt8TypeInContext(llvm_ctx), 0)},
                             1, true);
        if (!callee) {
            callee = LLVMAddFunction(module, "printf", printf_type);
        }
        return LLVMBuildCall2(llvm_builder, printf_type, callee, &operand, 1,
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

    LLVMValueRef var_ptr = lookup_var(emitter, var_name);
    if (!var_ptr) {
        return NULL;
    }

    LLVMTypeRef var_type = LLVMGetElementType(LLVMTypeOf(var_ptr));
    LLVMBuilderRef builder = Xvr_LLVMIRBuilderGetLLVMBuilder(emitter->builder);
    return LLVMBuildLoad2(builder, var_type, var_ptr, var_name);
}

LLVMValueRef Xvr_LLVMExpressionEmitterEmitFnCall(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_NodeFnCall* fn_call) {
    if (!emitter || !fn_call) {
        return NULL;
    }

    /* TODO: Implement proper function call emission
     * For now, just return NULL to not break the build
     * The function body will need to be emitted as a call to runtime
     */
    (void)fn_call;
    return NULL;
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
            for (int i = 0; i < node->block.count; i++) {
                Xvr_LLVMExpressionEmitterEmit(emitter, &node->block.nodes[i]);
            }
        }
        return NULL;

    /* Unsupported expression types */
    case XVR_AST_NODE_VAR_DECL:
    case XVR_AST_NODE_GROUPING:
    case XVR_AST_NODE_INDEX:
    case XVR_AST_NODE_ERROR:
    case XVR_AST_NODE_COMPOUND:
    case XVR_AST_NODE_PAIR:
    case XVR_AST_NODE_TERNARY:
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
