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

#ifndef XVR_LLVM_CONTROL_FLOW_H
#define XVR_LLVM_CONTROL_FLOW_H

#include <llvm-c/Core.h>
#include <stdbool.h>

#include "xvr_ast_node.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_type_mapper.h"

typedef struct Xvr_LLVMControlFlow Xvr_LLVMControlFlow;
typedef struct Xvr_LLVMExpressionEmitter Xvr_LLVMExpressionEmitter;

Xvr_LLVMControlFlow* Xvr_LLVMControlFlowCreate(
    Xvr_LLVMContext* ctx, Xvr_LLVMModuleManager* module,
    Xvr_LLVMIRBuilder* builder, Xvr_LLVMTypeMapper* type_mapper,
    Xvr_LLVMExpressionEmitter* expr_emitter);
void Xvr_LLVMControlFlowDestroy(Xvr_LLVMControlFlow* cf);

LLVMValueRef Xvr_LLVMControlFlowEmitIf(Xvr_LLVMControlFlow* cf,
                                       Xvr_NodeIf* if_node);
bool Xvr_LLVMControlFlowEmitWhile(Xvr_LLVMControlFlow* cf,
                                  Xvr_NodeWhile* while_node);
bool Xvr_LLVMControlFlowEmitFor(Xvr_LLVMControlFlow* cf, Xvr_NodeFor* for_node);
bool Xvr_LLVMControlFlowEmitBreak(Xvr_LLVMControlFlow* cf);
bool Xvr_LLVMControlFlowEmitContinue(Xvr_LLVMControlFlow* cf);

void Xvr_LLVMControlFlowPushLoopTarget(Xvr_LLVMControlFlow* cf,
                                       LLVMBasicBlockRef break_block,
                                       LLVMBasicBlockRef continue_block);
void Xvr_LLVMControlFlowPopLoopTarget(Xvr_LLVMControlFlow* cf);

#endif
