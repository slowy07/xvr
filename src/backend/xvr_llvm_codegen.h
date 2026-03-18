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

#ifndef XVR_LLVM_CODEGEN_H
#define XVR_LLVM_CODEGEN_H

#include <stdbool.h>
#include <stddef.h>

#include "xvr_ast_node.h"
#include "xvr_common.h"
#include "xvr_llvm_context.h"
#include "xvr_llvm_control_flow.h"
#include "xvr_llvm_expression_emitter.h"
#include "xvr_llvm_function_emitter.h"
#include "xvr_llvm_ir_builder.h"
#include "xvr_llvm_module_manager.h"
#include "xvr_llvm_optimizer.h"
#include "xvr_llvm_target.h"
#include "xvr_llvm_type_mapper.h"

typedef struct Xvr_LLVMCodegen Xvr_LLVMCodegen;

Xvr_LLVMCodegen* Xvr_LLVMCodegenCreate(const char* module_name);
void Xvr_LLVMCodegenDestroy(Xvr_LLVMCodegen* codegen);

bool Xvr_LLVMCodegenSetOptimizationLevel(Xvr_LLVMCodegen* codegen,
                                         Xvr_LLVMOptimizationLevel level);

bool Xvr_LLVMCodegenRunOptimizer(Xvr_LLVMCodegen* codegen);

bool Xvr_LLVMCodegenSetTargetTriple(Xvr_LLVMCodegen* codegen,
                                    const char* triple);

bool Xvr_LLVMCodegenSetTargetCPU(Xvr_LLVMCodegen* codegen, const char* cpu);

bool Xvr_LLVMCodegenEmitAST(Xvr_LLVMCodegen* codegen, Xvr_ASTNode* ast);

char* Xvr_LLVMCodegenPrintIR(Xvr_LLVMCodegen* codegen, size_t* out_len);

bool Xvr_LLVMCodegenWriteBitcode(Xvr_LLVMCodegen* codegen,
                                 const char* filepath);

bool Xvr_LLVMCodegenWriteObjectFile(Xvr_LLVMCodegen* codegen,
                                    const char* filepath);

bool Xvr_LLVMCodegenExecuteJIT(Xvr_LLVMCodegen* codegen);

bool Xvr_LLVMCodegenHasError(Xvr_LLVMCodegen* codegen);
const char* Xvr_LLVMCodegenGetError(Xvr_LLVMCodegen* codegen);

#endif
