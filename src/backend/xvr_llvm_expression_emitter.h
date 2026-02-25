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

#ifndef XVR_LLVM_EXPRESSION_EMITTER_H
#define XVR_LLVM_EXPRESSION_EMITTER_H

#include <llvm-c/Core.h>

#include "xvr_ast_node.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_type_mapper.h"

/**
 * @brief Opaque structure for expression emitter
 *
 * Manages expression-to-IR translation with references
 * to context, module, builder, and type mapper.
 */
typedef struct Xvr_LLVMExpressionEmitter Xvr_LLVMExpressionEmitter;

/**
 * @brief Creates a new expression emitter
 * @param ctx LLVM context
 * @param module Module manager
 * @param builder IR builder
 * @param type_mapper Type mapper
 * @return New emitter, or NULL on failure
 */
Xvr_LLVMExpressionEmitter* Xvr_LLVMExpressionEmitterCreate(
    Xvr_LLVMContext* ctx, Xvr_LLVMModuleManager* module,
    Xvr_LLVMIRBuilder* builder, Xvr_LLVMTypeMapper* type_mapper);

/**
 * @brief Destroys an expression emitter
 * @param emitter Emitter to destroy
 */
void Xvr_LLVMExpressionEmitterDestroy(Xvr_LLVMExpressionEmitter* emitter);

/**
 * @brief Emits IR for any AST node that is an expression
 * @param emitter Expression emitter
 * @param node AST node to emit
 * @return LLVM value, or NULL on failure
 */
LLVMValueRef Xvr_LLVMExpressionEmitterEmit(Xvr_LLVMExpressionEmitter* emitter,
                                           Xvr_ASTNode* node);

/**
 * @brief Emits IR for a literal node
 * @param emitter Expression emitter
 * @param literal Literal node
 * @return LLVM value, or NULL on failure
 */
LLVMValueRef Xvr_LLVMExpressionEmitterEmitLiteral(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_NodeLiteral* literal);

/**
 * @brief Emits IR for a binary expression
 * @param emitter Expression emitter
 * @param binary Binary node
 * @return LLVM value, or NULL on failure
 */
LLVMValueRef Xvr_LLVMExpressionEmitterEmitBinary(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_NodeBinary* binary);

/**
 * @brief Emits IR for a unary expression
 * @param emitter Expression emitter
 * @param unary Unary node
 * @return LLVM value, or NULL on failure
 */
LLVMValueRef Xvr_LLVMExpressionEmitterEmitUnary(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_NodeUnary* unary);

/**
 * @brief Emits IR for a function call
 * @param emitter Expression emitter
 * @param fn_call Function call node
 * @return LLVM value, or NULL on failure
 */
LLVMValueRef Xvr_LLVMExpressionEmitterEmitFnCall(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_NodeFnCall* fn_call);

/**
 * @brief Emits IR for an identifier (variable lookup)
 * @param emitter Expression emitter
 * @param identifier Identifier literal
 * @return LLVM value, or NULL on failure
 */
LLVMValueRef Xvr_LLVMExpressionEmitterEmitIdentifier(
    Xvr_LLVMExpressionEmitter* emitter, Xvr_Literal identifier);

#endif
