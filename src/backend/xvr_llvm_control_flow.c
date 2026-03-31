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

THE SOFTWARE IS "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "xvr_llvm_control_flow.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_ast_node.h"
#include "xvr_console_colors.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_expression_emitter.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_type_mapper.h"

#define MAX_LOOP_NESTING 64
#define MAX_ERROR_MESSAGE 512

struct Xvr_LLVMControlFlow {
    Xvr_LLVMContext* context;
    Xvr_LLVMModuleManager* module;
    Xvr_LLVMIRBuilder* builder;
    Xvr_LLVMTypeMapper* type_mapper;
    Xvr_LLVMExpressionEmitter* expr_emitter;

    LLVMBasicBlockRef loop_end_stack[MAX_LOOP_NESTING];
    LLVMBasicBlockRef loop_cond_stack[MAX_LOOP_NESTING];
    int loop_stack_depth;

    LLVMValueRef last_expression_result;

    bool has_error;
    char error_message[MAX_ERROR_MESSAGE];
    char error_hint[MAX_ERROR_MESSAGE];
};

static void emit_error(Xvr_LLVMControlFlow* cf, const char* fmt, ...) {
    if (!cf) return;

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(cf->error_message, MAX_ERROR_MESSAGE, fmt, args);
        va_end(args);
    }

    cf->has_error = true;

    fprintf(stderr, "\n");
    fprintf(stderr, XVR_CC_FONT_RED "error" XVR_CC_RESET ": %s\n",
            cf->error_message);

    if (cf->error_hint[0] != '\0') {
        fprintf(stderr, XVR_CC_NOTICE "help" XVR_CC_RESET ": %s\n",
                cf->error_hint);
    }
    fprintf(stderr, "\n");
}

static void set_error(Xvr_LLVMControlFlow* cf, const char* fmt, ...) {
    if (!cf) return;

    va_list args;
    va_start(args, fmt);
    vsnprintf(cf->error_message, MAX_ERROR_MESSAGE, fmt, args);
    va_end(args);

    cf->has_error = true;
    cf->error_hint[0] = '\0';
}

static void set_error_hint(Xvr_LLVMControlFlow* cf, const char* error,
                           const char* hint, ...) {
    if (!cf) return;

    if (error) {
        va_list args;
        va_start(args, hint);
        vsnprintf(cf->error_message, MAX_ERROR_MESSAGE, error, args);
        va_end(args);
    }

    if (hint) {
        va_list args;
        va_start(args, hint);
        vsnprintf(cf->error_hint, MAX_ERROR_MESSAGE, hint, args);
        va_end(args);
    } else {
        cf->error_hint[0] = '\0';
    }

    cf->has_error = true;
}

static void print_error(Xvr_LLVMControlFlow* cf) { emit_error(cf, NULL); }

void Xvr_LLVMControlFlowPrintError(Xvr_LLVMControlFlow* cf) { print_error(cf); }

Xvr_LLVMControlFlow* Xvr_LLVMControlFlowCreate(
    Xvr_LLVMContext* ctx, Xvr_LLVMModuleManager* module,
    Xvr_LLVMIRBuilder* builder, Xvr_LLVMTypeMapper* type_mapper,
    Xvr_LLVMExpressionEmitter* expr_emitter) {
    if (!ctx || !module || !builder || !type_mapper || !expr_emitter) {
        return NULL;
    }

    Xvr_LLVMControlFlow* cf = calloc(1, sizeof(Xvr_LLVMControlFlow));
    if (!cf) {
        return NULL;
    }

    cf->context = ctx;
    cf->module = module;
    cf->builder = builder;
    cf->type_mapper = type_mapper;
    cf->expr_emitter = expr_emitter;
    cf->last_expression_result = NULL;

    return cf;
}

void Xvr_LLVMControlFlowDestroy(Xvr_LLVMControlFlow* cf) { free(cf); }

bool Xvr_LLVMControlFlowHasError(Xvr_LLVMControlFlow* cf) {
    return cf && cf->has_error;
}

const char* Xvr_LLVMControlFlowGetError(Xvr_LLVMControlFlow* cf) {
    if (!cf || !cf->has_error) {
        return NULL;
    }
    return cf->error_message;
}

void Xvr_LLVMControlFlowClearError(Xvr_LLVMControlFlow* cf) {
    if (!cf) return;
    cf->has_error = false;
    cf->error_message[0] = '\0';
    cf->error_hint[0] = '\0';
}

LLVMValueRef Xvr_LLVMControlFlowGetLastExpressionResult(
    Xvr_LLVMControlFlow* cf) {
    if (!cf) return NULL;
    LLVMValueRef result = cf->last_expression_result;
    cf->last_expression_result = NULL;
    return result;
}

static bool is_boolean_type(LLVMValueRef value) {
    if (!value) return false;
    LLVMTypeRef type = LLVMTypeOf(value);
    return LLVMGetTypeKind(type) == LLVMIntegerTypeKind &&
           LLVMGetIntTypeWidth(type) == 1;
}

static void emit_block_statements(Xvr_LLVMControlFlow* cf,
                                  Xvr_ASTNode* block_node) {
    if (!block_node) return;

    if (block_node->type == XVR_AST_NODE_BLOCK) {
        Xvr_LLVMExpressionEmitter* expr_emitter = cf->expr_emitter;
        Xvr_NodeBlock* block = &block_node->block;

        if (!block->nodes || block->count == 0) return;

        for (int i = 0; i < block->count; i++) {
            Xvr_LLVMExpressionEmitterEmit(expr_emitter, &block->nodes[i]);
        }
    } else if (block_node->type == XVR_AST_NODE_IF) {
        Xvr_LLVMControlFlowEmitIf(cf, &block_node->pathIf);
    } else if (block_node->type == XVR_AST_NODE_WHILE) {
        Xvr_LLVMControlFlowEmitWhile(cf, &block_node->pathWhile);
    } else if (block_node->type == XVR_AST_NODE_FOR) {
        Xvr_LLVMControlFlowEmitFor(cf, &block_node->pathFor);
    } else {
        Xvr_LLVMExpressionEmitter* expr_emitter = cf->expr_emitter;
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, block_node);
    }
}

static LLVMValueRef emit_if_expression(Xvr_LLVMControlFlow* cf,
                                       Xvr_NodeIf* if_node) {
    if (!cf || !if_node) return NULL;

    Xvr_LLVMIRBuilder* builder = cf->builder;
    Xvr_LLVMExpressionEmitter* expr_emitter = cf->expr_emitter;
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);

    LLVMValueRef current_fn =
        Xvr_LLVMExpressionEmitterGetCurrentFunction(expr_emitter);
    if (!current_fn) return NULL;

    LLVMValueRef condition =
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, if_node->condition);
    if (!condition) {
        set_error(cf, "failed to evaluate condition of if expression");
        return NULL;
    }

    if (!is_boolean_type(condition)) {
        set_error_hint(cf,
                       "condition of if expression must be boolean, got '%s'",
                       "use a comparison operator (e.g., 'x > 0') or wrap the "
                       "condition with 'bool()'",
                       LLVMPrintTypeToString(LLVMTypeOf(condition)));
        return NULL;
    }

    LLVMTypeRef result_type =
        Xvr_LLVMTypeMapperGetType(cf->type_mapper, if_node->returnType);

    LLVMBasicBlockRef then_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "then");
    LLVMBasicBlockRef else_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "else");
    LLVMBasicBlockRef merge_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "ifcont");

    Xvr_LLVMIRBuilderCreateCondBr(builder, condition, then_block, else_block);

    LLVMValueRef then_val = NULL;
    LLVMBasicBlockRef then_end_block = then_block;

    Xvr_LLVMIRBuilderSetInsertPoint(builder, then_block);
    if (if_node->thenPath) {
        Xvr_ASTNode* then_node = if_node->thenPath;
        if (then_node->type == XVR_AST_NODE_BLOCK && then_node->block.nodes &&
            then_node->block.count > 0) {
            for (int i = 0; i < then_node->block.count - 1; i++) {
                Xvr_LLVMExpressionEmitterEmit(expr_emitter,
                                              &then_node->block.nodes[i]);
            }
            then_val = Xvr_LLVMExpressionEmitterEmit(
                expr_emitter,
                &then_node->block.nodes[then_node->block.count - 1]);
        } else {
            then_val = Xvr_LLVMExpressionEmitterEmit(expr_emitter, then_node);
        }
    }
    Xvr_LLVMIRBuilderCreateBr(builder, merge_block);
    then_end_block = LLVMGetInsertBlock(llvm_builder);

    LLVMValueRef else_val = NULL;
    LLVMBasicBlockRef else_end_block = else_block;

    Xvr_LLVMIRBuilderSetInsertPoint(builder, else_block);
    if (if_node->elsePath) {
        Xvr_ASTNode* else_node = if_node->elsePath;
        if (else_node->type == XVR_AST_NODE_IF) {
            else_val = emit_if_expression(cf, &else_node->pathIf);
        } else if (else_node->type == XVR_AST_NODE_BLOCK &&
                   else_node->block.nodes && else_node->block.count > 0) {
            for (int i = 0; i < else_node->block.count - 1; i++) {
                Xvr_LLVMExpressionEmitterEmit(expr_emitter,
                                              &else_node->block.nodes[i]);
            }
            else_val = Xvr_LLVMExpressionEmitterEmit(
                expr_emitter,
                &else_node->block.nodes[else_node->block.count - 1]);
        } else {
            else_val = Xvr_LLVMExpressionEmitterEmit(expr_emitter, else_node);
        }
    }
    Xvr_LLVMIRBuilderCreateBr(builder, merge_block);
    else_end_block = LLVMGetInsertBlock(llvm_builder);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, merge_block);

    if (!then_val) {
        then_val = LLVMConstNull(result_type);
    }
    if (!else_val) {
        else_val = LLVMConstNull(result_type);
    }

    LLVMValueRef phi = LLVMBuildPhi(llvm_builder, result_type, "if_result");
    LLVMAddIncoming(phi, &then_val, &then_end_block, 1);
    LLVMAddIncoming(phi, &else_val, &else_end_block, 1);

    cf->last_expression_result = phi;
    return phi;
}

bool Xvr_LLVMControlFlowEmitIf(Xvr_LLVMControlFlow* cf, Xvr_NodeIf* if_node) {
    if (!cf || !if_node) {
        set_error(cf, "internal error: null pointer passed to EmitIf");
        return false;
    }

    Xvr_LLVMControlFlowClearError(cf);

    if (if_node->isExpression) {
        if (!emit_if_expression(cf, if_node)) {
            return false;
        }
        return true;
    }

    Xvr_LLVMIRBuilder* builder = cf->builder;
    Xvr_LLVMExpressionEmitter* expr_emitter = cf->expr_emitter;

    LLVMValueRef current_fn =
        Xvr_LLVMExpressionEmitterGetCurrentFunction(expr_emitter);
    if (!current_fn) {
        set_error(cf, "internal error: no current function for if statement");
        return false;
    }

    LLVMValueRef condition =
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, if_node->condition);
    if (!condition) {
        set_error(cf, "failed to evaluate condition of if statement");
        return false;
    }

    if (!is_boolean_type(condition)) {
        char* type_str = LLVMPrintTypeToString(LLVMTypeOf(condition));
        set_error_hint(cf,
                       "condition of if statement must be boolean, got '%s'",
                       "use a comparison operator (e.g., 'x > 0') or wrap the "
                       "condition with 'bool()'",
                       type_str);
        LLVMDisposeMessage(type_str);
        return false;
    }

    LLVMBasicBlockRef then_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "then");
    LLVMBasicBlockRef else_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "else");
    LLVMBasicBlockRef merge_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "ifcont");

    Xvr_LLVMIRBuilderCreateCondBr(builder, condition, then_block, else_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, then_block);
    if (if_node->thenPath) {
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, if_node->thenPath);
    }
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
    if (LLVMGetInsertBlock(llvm_builder)) {
        Xvr_LLVMIRBuilderCreateBr(builder, merge_block);
    }

    Xvr_LLVMIRBuilderSetInsertPoint(builder, else_block);
    if (if_node->elsePath) {
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, if_node->elsePath);
    }
    llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
    if (LLVMGetInsertBlock(llvm_builder)) {
        Xvr_LLVMIRBuilderCreateBr(builder, merge_block);
    }

    Xvr_LLVMIRBuilderSetInsertPoint(builder, merge_block);

    return true;
}

bool Xvr_LLVMControlFlowEmitWhile(Xvr_LLVMControlFlow* cf,
                                  Xvr_NodeWhile* while_node) {
    if (!cf || !while_node) {
        set_error(cf, "internal error: null pointer passed to EmitWhile");
        return false;
    }

    Xvr_LLVMControlFlowClearError(cf);

    Xvr_LLVMIRBuilder* builder = cf->builder;
    Xvr_LLVMExpressionEmitter* expr_emitter = cf->expr_emitter;

    LLVMValueRef current_fn =
        Xvr_LLVMExpressionEmitterGetCurrentFunction(expr_emitter);
    if (!current_fn) {
        set_error(cf,
                  "internal error: no current function for while statement");
        return false;
    }

    if (cf->loop_stack_depth >= MAX_LOOP_NESTING) {
        set_error(cf, "maximum loop nesting depth (%d) exceeded",
                  MAX_LOOP_NESTING);
        return false;
    }

    LLVMBasicBlockRef cond_block = Xvr_LLVMIRBuilderCreateBlockInFunction(
        builder, current_fn, "while_cond");
    LLVMBasicBlockRef body_block = Xvr_LLVMIRBuilderCreateBlockInFunction(
        builder, current_fn, "while_body");
    LLVMBasicBlockRef end_block = Xvr_LLVMIRBuilderCreateBlockInFunction(
        builder, current_fn, "while_end");

    cf->loop_end_stack[cf->loop_stack_depth] = end_block;
    cf->loop_cond_stack[cf->loop_stack_depth] = cond_block;
    cf->loop_stack_depth++;

    Xvr_LLVMIRBuilderCreateBr(builder, cond_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, cond_block);
    LLVMValueRef condition =
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, while_node->condition);

    if (!condition) {
        set_error(cf, "failed to evaluate condition of while statement");
        cf->loop_stack_depth--;
        return false;
    }

    if (!is_boolean_type(condition)) {
        const char* type_str = LLVMPrintTypeToString(LLVMTypeOf(condition));
        set_error_hint(cf,
                       "condition of while statement must be boolean, got '%s'",
                       "use a comparison operator (e.g., 'x > 0') or wrap the "
                       "condition with 'bool()'",
                       type_str);
        cf->loop_stack_depth--;
        return false;
    }
    Xvr_LLVMIRBuilderCreateCondBr(builder, condition, body_block, end_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, body_block);
    if (while_node->thenPath) {
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, while_node->thenPath);
    }
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
    if (LLVMGetInsertBlock(llvm_builder)) {
        Xvr_LLVMIRBuilderCreateBr(builder, cond_block);
    }

    Xvr_LLVMIRBuilderSetInsertPoint(builder, end_block);

    cf->loop_stack_depth--;

    return true;
}

bool Xvr_LLVMControlFlowEmitFor(Xvr_LLVMControlFlow* cf,
                                Xvr_NodeFor* for_node) {
    if (!cf || !for_node) {
        set_error(cf, "internal error: null pointer passed to EmitFor");
        return false;
    }

    Xvr_LLVMControlFlowClearError(cf);

    Xvr_LLVMIRBuilder* builder = cf->builder;
    Xvr_LLVMExpressionEmitter* expr_emitter = cf->expr_emitter;

    LLVMValueRef current_fn =
        Xvr_LLVMExpressionEmitterGetCurrentFunction(expr_emitter);
    if (!current_fn) {
        set_error(cf, "internal error: no current function for for statement");
        return false;
    }

    if (cf->loop_stack_depth >= MAX_LOOP_NESTING) {
        set_error(cf, "maximum loop nesting depth (%d) exceeded",
                  MAX_LOOP_NESTING);
        return false;
    }

    LLVMBasicBlockRef cond_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "for_cond");
    LLVMBasicBlockRef body_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "for_body");
    LLVMBasicBlockRef inc_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "for_inc");
    LLVMBasicBlockRef end_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "for_end");

    cf->loop_end_stack[cf->loop_stack_depth] = end_block;
    cf->loop_cond_stack[cf->loop_stack_depth] = inc_block;
    cf->loop_stack_depth++;

    /* Emit preClause (initialization) - may create its own block */
    if (for_node->preClause) {
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, for_node->preClause);
    }

    /* Branch from current location to condition block */
    Xvr_LLVMIRBuilderCreateBr(builder, cond_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, cond_block);
    LLVMValueRef condition = NULL;
    if (for_node->condition) {
        condition =
            Xvr_LLVMExpressionEmitterEmit(expr_emitter, for_node->condition);
        if (condition && !is_boolean_type(condition)) {
            set_error_hint(
                cf, "condition of for statement must be boolean, got '%s'",
                "use a comparison operator (e.g., 'i < 10') or wrap the "
                "condition with 'bool()'",
                LLVMPrintTypeToString(LLVMTypeOf(condition)));
            cf->loop_stack_depth--;
            return false;
        }
    }
    if (!condition) {
        LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(cf->context);
        condition = LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx), 1, false);
    }
    Xvr_LLVMIRBuilderCreateCondBr(builder, condition, body_block, end_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, body_block);
    if (for_node->thenPath) {
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, for_node->thenPath);
    }
    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
    if (LLVMGetInsertBlock(llvm_builder)) {
        Xvr_LLVMIRBuilderCreateBr(builder, inc_block);
    }

    Xvr_LLVMIRBuilderSetInsertPoint(builder, inc_block);
    if (for_node->postClause) {
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, for_node->postClause);
    }
    llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
    if (LLVMGetInsertBlock(llvm_builder)) {
        Xvr_LLVMIRBuilderCreateBr(builder, cond_block);
    }

    Xvr_LLVMIRBuilderSetInsertPoint(builder, end_block);

    cf->loop_stack_depth--;

    return true;
}

bool Xvr_LLVMControlFlowEmitBreak(Xvr_LLVMControlFlow* cf) {
    if (!cf) {
        set_error(cf, "internal error: null pointer passed to EmitBreak");
        return false;
    }

    Xvr_LLVMControlFlowClearError(cf);

    if (cf->loop_stack_depth == 0) {
        set_error_hint(
            cf, "break statement must be inside a loop",
            "place the 'break' statement inside a 'while' or 'for' loop");
        return false;
    }

    LLVMBasicBlockRef end_block = cf->loop_end_stack[cf->loop_stack_depth - 1];
    Xvr_LLVMIRBuilderCreateBr(cf->builder, end_block);
    return true;
}

bool Xvr_LLVMControlFlowEmitContinue(Xvr_LLVMControlFlow* cf) {
    if (!cf) {
        set_error(cf, "internal error: null pointer passed to EmitContinue");
        return false;
    }

    Xvr_LLVMControlFlowClearError(cf);

    if (cf->loop_stack_depth == 0) {
        set_error_hint(
            cf, "continue statement must be inside a loop",
            "place the 'continue' statement inside a 'while' or 'for' loop");
        return false;
    }

    LLVMBasicBlockRef cond_block =
        cf->loop_cond_stack[cf->loop_stack_depth - 1];
    Xvr_LLVMIRBuilderCreateBr(cf->builder, cond_block);
    return true;
}

void Xvr_LLVMControlFlowPushLoopTarget(Xvr_LLVMControlFlow* cf,
                                       LLVMBasicBlockRef break_block,
                                       LLVMBasicBlockRef continue_block) {
    if (!cf || cf->loop_stack_depth >= MAX_LOOP_NESTING) return;

    cf->loop_end_stack[cf->loop_stack_depth] = break_block;
    cf->loop_cond_stack[cf->loop_stack_depth] = continue_block;
    cf->loop_stack_depth++;
}

void Xvr_LLVMControlFlowPopLoopTarget(Xvr_LLVMControlFlow* cf) {
    if (!cf || cf->loop_stack_depth <= 0) return;
    cf->loop_stack_depth--;
}
