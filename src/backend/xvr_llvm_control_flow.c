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

#include "xvr_llvm_control_flow.h"

#include <stdio.h>
#include <stdlib.h>

#include "xvr_ast_node.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_expression_emitter.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_type_mapper.h"

struct Xvr_LLVMControlFlow {
    Xvr_LLVMContext* context;
    Xvr_LLVMModuleManager* module;
    Xvr_LLVMIRBuilder* builder;
    Xvr_LLVMTypeMapper* type_mapper;
    Xvr_LLVMExpressionEmitter* expr_emitter;

    /* Stack for tracking current loop context (for break/continue) */
    LLVMBasicBlockRef loop_end_stack[100];
    LLVMBasicBlockRef loop_cond_stack[100];
    int loop_stack_depth;
};

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
    cf->loop_stack_depth = 0;

    return cf;
}

void Xvr_LLVMControlFlowDestroy(Xvr_LLVMControlFlow* cf) { free(cf); }

bool Xvr_LLVMControlFlowEmitIf(Xvr_LLVMControlFlow* cf, Xvr_NodeIf* if_node) {
    if (!cf || !if_node) {
        return false;
    }

    Xvr_LLVMIRBuilder* builder = cf->builder;
    Xvr_LLVMExpressionEmitter* expr_emitter = cf->expr_emitter;

    LLVMValueRef current_fn =
        Xvr_LLVMExpressionEmitterGetCurrentFunction(expr_emitter);
    if (!current_fn) {
        return false;
    }

    LLVMValueRef condition =
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, if_node->condition);
    if (!condition) {
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
    Xvr_LLVMIRBuilderCreateBr(builder, merge_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, else_block);
    if (if_node->elsePath) {
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, if_node->elsePath);
    }
    Xvr_LLVMIRBuilderCreateBr(builder, merge_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, merge_block);

    return true;
}

bool Xvr_LLVMControlFlowEmitWhile(Xvr_LLVMControlFlow* cf,
                                  Xvr_NodeWhile* while_node) {
    if (!cf || !while_node) {
        return false;
    }

    Xvr_LLVMIRBuilder* builder = cf->builder;
    Xvr_LLVMExpressionEmitter* expr_emitter = cf->expr_emitter;

    LLVMValueRef current_fn =
        Xvr_LLVMExpressionEmitterGetCurrentFunction(expr_emitter);
    if (!current_fn) {
        return false;
    }

    LLVMBasicBlockRef cond_block = Xvr_LLVMIRBuilderCreateBlockInFunction(
        builder, current_fn, "while_cond");
    LLVMBasicBlockRef body_block = Xvr_LLVMIRBuilderCreateBlockInFunction(
        builder, current_fn, "while_body");
    LLVMBasicBlockRef end_block = Xvr_LLVMIRBuilderCreateBlockInFunction(
        builder, current_fn, "while_end");
    LLVMBasicBlockRef after_block = Xvr_LLVMIRBuilderCreateBlockInFunction(
        builder, current_fn, "while_after");

    /* Push loop context for break/continue */
    if (cf->loop_stack_depth < 100) {
        cf->loop_end_stack[cf->loop_stack_depth] = end_block;
        cf->loop_cond_stack[cf->loop_stack_depth] = cond_block;
        cf->loop_stack_depth++;
    }

    Xvr_LLVMIRBuilderCreateBr(builder, cond_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, cond_block);
    LLVMValueRef condition =
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, while_node->condition);

    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(cf->context);
    if (!condition) {
        condition = LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx), 0, false);
    }
    Xvr_LLVMIRBuilderCreateCondBr(builder, condition, body_block, end_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, body_block);
    if (while_node->thenPath) {
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, while_node->thenPath);
    }
    Xvr_LLVMIRBuilderCreateBr(builder, cond_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, end_block);
    Xvr_LLVMIRBuilderCreateBr(builder, after_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, after_block);

    /* Pop loop context */
    if (cf->loop_stack_depth > 0) {
        cf->loop_stack_depth--;
    }

    return true;
}

bool Xvr_LLVMControlFlowEmitFor(Xvr_LLVMControlFlow* cf,
                                Xvr_NodeFor* for_node) {
    if (!cf || !for_node) {
        return false;
    }

    Xvr_LLVMIRBuilder* builder = cf->builder;
    Xvr_LLVMExpressionEmitter* expr_emitter = cf->expr_emitter;

    LLVMValueRef current_fn =
        Xvr_LLVMExpressionEmitterGetCurrentFunction(expr_emitter);
    if (!current_fn) {
        return false;
    }

    if (for_node->preClause) {
    }

    LLVMBasicBlockRef cond_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "for_cond");
    LLVMBasicBlockRef body_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "for_body");
    LLVMBasicBlockRef inc_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "for_inc");
    LLVMBasicBlockRef end_block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, current_fn, "for_end");

    Xvr_LLVMIRBuilderCreateBr(builder, cond_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, cond_block);
    LLVMValueRef condition = NULL;
    if (for_node->condition) {
        condition =
            Xvr_LLVMExpressionEmitterEmit(expr_emitter, for_node->condition);
    } else {
        LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(cf->context);
        condition = LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx), 1, false);
    }
    Xvr_LLVMIRBuilderCreateCondBr(builder, condition, body_block, end_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, body_block);
    if (for_node->thenPath) {
        Xvr_LLVMExpressionEmitterEmit(expr_emitter, for_node->thenPath);
    }
    Xvr_LLVMIRBuilderCreateBr(builder, inc_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, inc_block);
    if (for_node->postClause) {
    }
    Xvr_LLVMIRBuilderCreateBr(builder, cond_block);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, end_block);

    return true;
}

bool Xvr_LLVMControlFlowEmitBreak(Xvr_LLVMControlFlow* cf) {
    if (!cf || cf->loop_stack_depth == 0) {
        return false;
    }

    /* Branch to the end block of the current loop */
    LLVMBasicBlockRef end_block = cf->loop_end_stack[cf->loop_stack_depth - 1];
    Xvr_LLVMIRBuilderCreateBr(cf->builder, end_block);
    return true;
}

bool Xvr_LLVMControlFlowEmitContinue(Xvr_LLVMControlFlow* cf) {
    if (!cf || cf->loop_stack_depth == 0) {
        return false;
    }

    /* Branch to the condition block of the current loop */
    LLVMBasicBlockRef cond_block =
        cf->loop_cond_stack[cf->loop_stack_depth - 1];
    Xvr_LLVMIRBuilderCreateBr(cf->builder, cond_block);
    return true;
}
