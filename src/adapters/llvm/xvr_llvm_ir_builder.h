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

#ifndef XVR_LLVM_IR_BUILDER_H
#define XVR_LLVM_IR_BUILDER_H

#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

#include "xvr_llvm_context.h"
#include "xvr_llvm_module_manager.h"

typedef struct Xvr_LLVMIRBuilder Xvr_LLVMIRBuilder;

Xvr_LLVMIRBuilder* Xvr_LLVMIRBuilderCreate(Xvr_LLVMContext* ctx,
                                           LLVMModuleRef module);
void Xvr_LLVMIRBuilderDestroy(Xvr_LLVMIRBuilder* builder);
LLVMBuilderRef Xvr_LLVMIRBuilderGetLLVMBuilder(Xvr_LLVMIRBuilder* builder);

LLVMBasicBlockRef Xvr_LLVMIRBuilderGetInsertBlock(Xvr_LLVMIRBuilder* builder);
void Xvr_LLVMIRBuilderSetInsertPoint(Xvr_LLVMIRBuilder* builder,
                                     LLVMBasicBlockRef block);
void Xvr_LLVMIRBuilderSetInsertPointAtEnd(Xvr_LLVMIRBuilder* builder,
                                          LLVMBasicBlockRef block);

LLVMValueRef Xvr_LLVMIRBuilderCreateRet(Xvr_LLVMIRBuilder* builder,
                                        LLVMValueRef value);
LLVMValueRef Xvr_LLVMIRBuilderCreateRetVoid(Xvr_LLVMIRBuilder* builder);

LLVMValueRef Xvr_LLVMIRBuilderCreateBr(Xvr_LLVMIRBuilder* builder,
                                       LLVMBasicBlockRef dest);
LLVMValueRef Xvr_LLVMIRBuilderCreateCondBr(Xvr_LLVMIRBuilder* builder,
                                           LLVMValueRef condition,
                                           LLVMBasicBlockRef then_block,
                                           LLVMBasicBlockRef else_block);

LLVMValueRef Xvr_LLVMIRBuilderCreateAdd(Xvr_LLVMIRBuilder* builder,
                                        LLVMValueRef lhs, LLVMValueRef rhs,
                                        const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateSub(Xvr_LLVMIRBuilder* builder,
                                        LLVMValueRef lhs, LLVMValueRef rhs,
                                        const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateMul(Xvr_LLVMIRBuilder* builder,
                                        LLVMValueRef lhs, LLVMValueRef rhs,
                                        const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateSDiv(Xvr_LLVMIRBuilder* builder,
                                         LLVMValueRef lhs, LLVMValueRef rhs,
                                         const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateSRem(Xvr_LLVMIRBuilder* builder,
                                         LLVMValueRef lhs, LLVMValueRef rhs,
                                         const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateUDiv(Xvr_LLVMIRBuilder* builder,
                                         LLVMValueRef lhs, LLVMValueRef rhs,
                                         const char* name);

LLVMValueRef Xvr_LLVMIRBuilderCreateICmpEQ(Xvr_LLVMIRBuilder* builder,
                                           LLVMValueRef lhs, LLVMValueRef rhs,
                                           const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateICmpNE(Xvr_LLVMIRBuilder* builder,
                                           LLVMValueRef lhs, LLVMValueRef rhs,
                                           const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateICmpSLT(Xvr_LLVMIRBuilder* builder,
                                            LLVMValueRef lhs, LLVMValueRef rhs,
                                            const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateICmpSLE(Xvr_LLVMIRBuilder* builder,
                                            LLVMValueRef lhs, LLVMValueRef rhs,
                                            const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateICmpSGT(Xvr_LLVMIRBuilder* builder,
                                            LLVMValueRef lhs, LLVMValueRef rhs,
                                            const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateICmpSGE(Xvr_LLVMIRBuilder* builder,
                                            LLVMValueRef lhs, LLVMValueRef rhs,
                                            const char* name);

LLVMValueRef Xvr_LLVMIRBuilderCreateAlloca(Xvr_LLVMIRBuilder* builder,
                                           LLVMTypeRef type, const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateLoad(Xvr_LLVMIRBuilder* builder,
                                         LLVMTypeRef type, LLVMValueRef ptr,
                                         const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateStore(Xvr_LLVMIRBuilder* builder,
                                          LLVMValueRef value, LLVMValueRef ptr);

LLVMValueRef Xvr_LLVMIRBuilderCreateCall(Xvr_LLVMIRBuilder* builder,
                                         LLVMValueRef function,
                                         LLVMValueRef* args, unsigned arg_count,
                                         const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateCall0(Xvr_LLVMIRBuilder* builder,
                                          LLVMValueRef function,
                                          const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateCall1(Xvr_LLVMIRBuilder* builder,
                                          LLVMValueRef function,
                                          LLVMValueRef arg0, const char* name);
LLVMValueRef Xvr_LLVMIRBuilderCreateCall2(Xvr_LLVMIRBuilder* builder,
                                          LLVMValueRef function,
                                          LLVMValueRef arg0, LLVMValueRef arg1,
                                          const char* name);

LLVMValueRef Xvr_LLVMIRBuilderCreateSelect(Xvr_LLVMIRBuilder* builder,
                                           LLVMValueRef condition,
                                           LLVMValueRef then_val,
                                           LLVMValueRef else_val,
                                           const char* name);

LLVMValueRef Xvr_LLVMIRBuilderCreatePHI(Xvr_LLVMIRBuilder* builder,
                                        LLVMTypeRef type, const char* name);

LLVMBasicBlockRef Xvr_LLVMIRBuilderCreateBlock(Xvr_LLVMIRBuilder* builder,
                                               const char* name);

LLVMBasicBlockRef Xvr_LLVMIRBuilderCreateBlockInFunction(
    Xvr_LLVMIRBuilder* builder, LLVMValueRef function, const char* name);

#endif
