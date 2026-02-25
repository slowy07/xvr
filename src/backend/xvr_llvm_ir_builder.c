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

#include "xvr_llvm_ir_builder.h"

#include <stdlib.h>

#include "xvr_llvm_context.h"

struct Xvr_LLVMIRBuilder {
    Xvr_LLVMContext* context;
    LLVMBuilderRef builder;
};

Xvr_LLVMIRBuilder* Xvr_LLVMIRBuilderCreate(Xvr_LLVMContext* ctx,
                                           LLVMModuleRef module) {
    if (!ctx || !module) {
        return NULL;
    }

    Xvr_LLVMIRBuilder* builder = calloc(1, sizeof(Xvr_LLVMIRBuilder));
    if (!builder) {
        return NULL;
    }

    builder->context = ctx;
    builder->builder =
        LLVMCreateBuilderInContext(Xvr_LLVMContextGetLLVMContext(ctx));

    return builder;
}

void Xvr_LLVMIRBuilderDestroy(Xvr_LLVMIRBuilder* builder) {
    if (!builder) {
        return;
    }
    if (builder->builder) {
        LLVMDisposeBuilder(builder->builder);
    }
    free(builder);
}

LLVMBuilderRef Xvr_LLVMIRBuilderGetLLVMBuilder(Xvr_LLVMIRBuilder* builder) {
    if (!builder) {
        return NULL;
    }
    return builder->builder;
}

LLVMBasicBlockRef Xvr_LLVMIRBuilderGetInsertBlock(Xvr_LLVMIRBuilder* builder) {
    if (!builder) {
        return NULL;
    }
    return LLVMGetInsertBlock(builder->builder);
}

void Xvr_LLVMIRBuilderSetInsertPoint(Xvr_LLVMIRBuilder* builder,
                                     LLVMBasicBlockRef block) {
    if (!builder || !block) {
        return;
    }
    LLVMPositionBuilderAtEnd(builder->builder, block);
}

void Xvr_LLVMIRBuilderSetInsertPointAtEnd(Xvr_LLVMIRBuilder* builder,
                                          LLVMBasicBlockRef block) {
    Xvr_LLVMIRBuilderSetInsertPoint(builder, block);
}

LLVMValueRef Xvr_LLVMIRBuilderCreateRet(Xvr_LLVMIRBuilder* builder,
                                        LLVMValueRef value) {
    if (!builder || !value) {
        return NULL;
    }
    return LLVMBuildRet(builder->builder, value);
}

LLVMValueRef Xvr_LLVMIRBuilderCreateRetVoid(Xvr_LLVMIRBuilder* builder) {
    if (!builder) {
        return NULL;
    }
    return LLVMBuildRetVoid(builder->builder);
}

LLVMValueRef Xvr_LLVMIRBuilderCreateBr(Xvr_LLVMIRBuilder* builder,
                                       LLVMBasicBlockRef dest) {
    if (!builder || !dest) {
        return NULL;
    }
    return LLVMBuildBr(builder->builder, dest);
}

LLVMValueRef Xvr_LLVMIRBuilderCreateCondBr(Xvr_LLVMIRBuilder* builder,
                                           LLVMValueRef condition,
                                           LLVMBasicBlockRef then_block,
                                           LLVMBasicBlockRef else_block) {
    if (!builder || !condition || !then_block || !else_block) {
        return NULL;
    }
    return LLVMBuildCondBr(builder->builder, condition, then_block, else_block);
}

LLVMValueRef Xvr_LLVMIRBuilderCreateAdd(Xvr_LLVMIRBuilder* builder,
                                        LLVMValueRef lhs, LLVMValueRef rhs,
                                        const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildAdd(builder->builder, lhs, rhs, name ? name : "add_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateSub(Xvr_LLVMIRBuilder* builder,
                                        LLVMValueRef lhs, LLVMValueRef rhs,
                                        const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildSub(builder->builder, lhs, rhs, name ? name : "sub_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateMul(Xvr_LLVMIRBuilder* builder,
                                        LLVMValueRef lhs, LLVMValueRef rhs,
                                        const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildMul(builder->builder, lhs, rhs, name ? name : "mul_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateSDiv(Xvr_LLVMIRBuilder* builder,
                                         LLVMValueRef lhs, LLVMValueRef rhs,
                                         const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildSDiv(builder->builder, lhs, rhs, name ? name : "sdiv_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateUDiv(Xvr_LLVMIRBuilder* builder,
                                         LLVMValueRef lhs, LLVMValueRef rhs,
                                         const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildUDiv(builder->builder, lhs, rhs, name ? name : "udiv_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateICmpEQ(Xvr_LLVMIRBuilder* builder,
                                           LLVMValueRef lhs, LLVMValueRef rhs,
                                           const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildICmp(builder->builder, LLVMIntEQ, lhs, rhs,
                         name ? name : "icmp_eq_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateICmpNE(Xvr_LLVMIRBuilder* builder,
                                           LLVMValueRef lhs, LLVMValueRef rhs,
                                           const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildICmp(builder->builder, LLVMIntNE, lhs, rhs,
                         name ? name : "icmp_ne_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateICmpSLT(Xvr_LLVMIRBuilder* builder,
                                            LLVMValueRef lhs, LLVMValueRef rhs,
                                            const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildICmp(builder->builder, LLVMIntSLT, lhs, rhs,
                         name ? name : "icmp_slt_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateICmpSLE(Xvr_LLVMIRBuilder* builder,
                                            LLVMValueRef lhs, LLVMValueRef rhs,
                                            const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildICmp(builder->builder, LLVMIntSLE, lhs, rhs,
                         name ? name : "icmp_sle_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateICmpSGT(Xvr_LLVMIRBuilder* builder,
                                            LLVMValueRef lhs, LLVMValueRef rhs,
                                            const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildICmp(builder->builder, LLVMIntSGT, lhs, rhs,
                         name ? name : "icmp_sgt_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateICmpSGE(Xvr_LLVMIRBuilder* builder,
                                            LLVMValueRef lhs, LLVMValueRef rhs,
                                            const char* name) {
    if (!builder || !lhs || !rhs) {
        return NULL;
    }
    return LLVMBuildICmp(builder->builder, LLVMIntSGE, lhs, rhs,
                         name ? name : "icmp_sge_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateAlloca(Xvr_LLVMIRBuilder* builder,
                                           LLVMTypeRef type, const char* name) {
    if (!builder || !type) {
        return NULL;
    }
    return LLVMBuildAlloca(builder->builder, type, name ? name : "alloca_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateLoad(Xvr_LLVMIRBuilder* builder,
                                         LLVMTypeRef type, LLVMValueRef ptr,
                                         const char* name) {
    if (!builder || !type || !ptr) {
        return NULL;
    }
    return LLVMBuildLoad2(builder->builder, type, ptr,
                          name ? name : "load_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateStore(Xvr_LLVMIRBuilder* builder,
                                          LLVMValueRef value,
                                          LLVMValueRef ptr) {
    if (!builder || !value || !ptr) {
        return NULL;
    }
    return LLVMBuildStore(builder->builder, value, ptr);
}

LLVMValueRef Xvr_LLVMIRBuilderCreateCall(Xvr_LLVMIRBuilder* builder,
                                         LLVMValueRef function,
                                         LLVMValueRef* args, unsigned arg_count,
                                         const char* name) {
    if (!builder || !function || !args) {
        return NULL;
    }
    return LLVMBuildCall2(builder->builder,
                          LLVMGetElementType(LLVMTypeOf(function)), function,
                          args, arg_count, name ? name : "call_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreateCall0(Xvr_LLVMIRBuilder* builder,
                                          LLVMValueRef function,
                                          const char* name) {
    return Xvr_LLVMIRBuilderCreateCall(builder, function, NULL, 0, name);
}

LLVMValueRef Xvr_LLVMIRBuilderCreateCall1(Xvr_LLVMIRBuilder* builder,
                                          LLVMValueRef function,
                                          LLVMValueRef arg0, const char* name) {
    return Xvr_LLVMIRBuilderCreateCall(builder, function, &arg0, 1, name);
}

LLVMValueRef Xvr_LLVMIRBuilderCreateCall2(Xvr_LLVMIRBuilder* builder,
                                          LLVMValueRef function,
                                          LLVMValueRef arg0, LLVMValueRef arg1,
                                          const char* name) {
    LLVMValueRef args[] = {arg0, arg1};
    return Xvr_LLVMIRBuilderCreateCall(builder, function, args, 2, name);
}

LLVMValueRef Xvr_LLVMIRBuilderCreateSelect(Xvr_LLVMIRBuilder* builder,
                                           LLVMValueRef condition,
                                           LLVMValueRef then_val,
                                           LLVMValueRef else_val,
                                           const char* name) {
    if (!builder || !condition || !then_val || !else_val) {
        return NULL;
    }
    return LLVMBuildSelect(builder->builder, condition, then_val, else_val,
                           name ? name : "select_tmp");
}

LLVMValueRef Xvr_LLVMIRBuilderCreatePHI(Xvr_LLVMIRBuilder* builder,
                                        LLVMTypeRef type, const char* name) {
    if (!builder || !type) {
        return NULL;
    }
    return LLVMBuildPhi(builder->builder, type, name ? name : "phi_tmp");
}

LLVMBasicBlockRef Xvr_LLVMIRBuilderCreateBlock(Xvr_LLVMIRBuilder* builder,
                                               const char* name) {
    if (!builder) {
        return NULL;
    }
    return LLVMAppendBasicBlockInContext(
        Xvr_LLVMContextGetLLVMContext(builder->context), NULL,
        name ? name : "block");
}

LLVMBasicBlockRef Xvr_LLVMIRBuilderCreateBlockInFunction(
    Xvr_LLVMIRBuilder* builder, LLVMValueRef function, const char* name) {
    if (!builder || !function) {
        return NULL;
    }
    return LLVMAppendBasicBlock(function, name ? name : "block");
}
