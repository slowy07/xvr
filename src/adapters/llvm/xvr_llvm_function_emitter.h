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

#ifndef XVR_LLVM_FUNCTION_EMITTER_H
#define XVR_LLVM_FUNCTION_EMITTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <llvm-c/Core.h>
#include <stdbool.h>

#include "xvr_ast_node.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_expression_emitter.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_type_mapper.h"

typedef struct Xvr_LLVMFunctionEmitter Xvr_LLVMFunctionEmitter;

Xvr_LLVMFunctionEmitter* Xvr_LLVMFunctionEmitterCreate(
    Xvr_LLVMContext* ctx, Xvr_LLVMModuleManager* module,
    Xvr_LLVMIRBuilder* builder, Xvr_LLVMTypeMapper* type_mapper,
    Xvr_LLVMExpressionEmitter* expr_emitter);
void Xvr_LLVMFunctionEmitterDestroy(Xvr_LLVMFunctionEmitter* emitter);

bool Xvr_LLVMFunctionEmitterEmit(Xvr_LLVMFunctionEmitter* emitter,
                                 Xvr_ASTNode* fn_decl);
bool Xvr_LLVMFunctionEmitterEmitCollection(Xvr_LLVMFunctionEmitter* emitter,
                                           Xvr_ASTNode* fn_collection);

/**
 * @brief Looks up a local variable by name
 * @param emitter Function emitter
 * @param name Variable name
 * @return LLVM value (the alloca or parameter), or NULL if not found
 */
LLVMValueRef Xvr_LLVMFunctionEmitterLookupVar(Xvr_LLVMFunctionEmitter* emitter,
                                              const char* name);

/**
 * @brief Looks up a local variable by name and returns its type
 * @param emitter Function emitter
 * @param name Variable name
 * @param out_type Output parameter for the variable's XVR type
 * @return LLVM value (the alloca or parameter), or NULL if not found
 */
LLVMValueRef Xvr_LLVMFunctionEmitterLookupVarWithType(
    Xvr_LLVMFunctionEmitter* emitter, const char* name,
    Xvr_LiteralType* out_type);

/**
 * @brief Looks up a local variable by name and returns array count
 * @param emitter Function emitter
 * @param name Variable name
 * @return Array element count, or 0 if not an array or not found
 */
int Xvr_LLVMFunctionEmitterLookupVarArrayCount(Xvr_LLVMFunctionEmitter* emitter,
                                               const char* name);

/**
 * @brief Gets the current function being emitted
 * @param emitter Function emitter
 * @return Current LLVM function, or NULL if none
 */
LLVMValueRef Xvr_LLVMFunctionEmitterGetCurrentFunction(
    Xvr_LLVMFunctionEmitter* emitter);

/**
 * @brief Sets the current function being emitted
 * @param emitter Function emitter
 * @param function LLVM function to set as current
 */
void Xvr_LLVMFunctionEmitterSetCurrentFunction(Xvr_LLVMFunctionEmitter* emitter,
                                               LLVMValueRef function);

/**
 * @brief Add a local variable to the function emitter
 * @param emitter Function emitter
 * @param name Variable name
 * @param alloca LLVM alloca for the variable
 * @param type XVR literal type
 * @param array_count Number of elements if array, 0 otherwise
 */
void Xvr_LLVMFunctionEmitterAddLocalVar(Xvr_LLVMFunctionEmitter* emitter,
                                        const char* name, LLVMValueRef alloca,
                                        Xvr_LiteralType type, int array_count);

/**
 * @brief Enter a new scope (saves current variable count)
 * @param emitter Function emitter
 */
void Xvr_LLVMFunctionEmitterEnterScope(Xvr_LLVMFunctionEmitter* emitter);

/**
 * @brief Exit current scope (removes variables added in this scope)
 * @param emitter Function emitter
 */
void Xvr_LLVMFunctionEmitterExitScope(Xvr_LLVMFunctionEmitter* emitter);

#ifdef __cplusplus
}
#endif

#endif
