/**
 * MIT License
 * Copyright (c) 2025 arfy slowy
 *
 * @file xvr_cast_emit.h
 * @brief LLVM cast lowering - generates correct LLVM IR for type conversions
 */

#ifndef XVR_CAST_EMIT_H
#define XVR_CAST_EMIT_H

#include <llvm-c/Core.h>

#include "xvr_type.h"

typedef struct Xvr_LLVMCastEmitter Xvr_LLVMCastEmitter;

Xvr_LLVMCastEmitter* Xvr_LLVMCastEmitterCreate(LLVMModuleRef module,
                                               LLVMBuilderRef builder);
void Xvr_LLVMCastEmitterDestroy(Xvr_LLVMCastEmitter* emitter);

typedef enum {
    XVR_LLVM_CAST_INT_TO_INT,
    XVR_LLVM_CAST_INT_TO_FLOAT,
    XVR_LLVM_CAST_FLOAT_TO_INT,
    XVR_LLVM_CAST_FLOAT_TO_FLOAT,
    XVR_LLVM_CAST_INT_TO_PTR,
    XVR_LLVM_CAST_PTR_TO_INT,
    XVR_LLVM_CAST_PTR_TO_PTR,
    XVR_LLVM_CAST_BOOL_TO_INT,
    XVR_LLVM_CAST_INT_TO_BOOL,
    XVR_LLVM_CAST_BOOL_TO_FLOAT,
    XVR_LLVM_CAST_FLOAT_TO_BOOL,
    XVR_LLVM_CAST_INVALID,
} Xvr_LLVMCastType;

Xvr_LLVMCastType Xvr_CastEmitterClassify(Xvr_Type* from, Xvr_Type* to);

LLVMValueRef Xvr_EmitCast(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                          Xvr_Type* from, Xvr_Type* to, const char* name);

LLVMValueRef Xvr_EmitIntToInt(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name);
LLVMValueRef Xvr_EmitIntToFloat(Xvr_LLVMCastEmitter* emitter,
                                LLVMValueRef value, Xvr_Type* from,
                                Xvr_Type* to, const char* name);
LLVMValueRef Xvr_EmitFloatToInt(Xvr_LLVMCastEmitter* emitter,
                                LLVMValueRef value, Xvr_Type* from,
                                Xvr_Type* to, const char* name);
LLVMValueRef Xvr_EmitFloatToFloat(Xvr_LLVMCastEmitter* emitter,
                                  LLVMValueRef value, Xvr_Type* from,
                                  Xvr_Type* to, const char* name);
LLVMValueRef Xvr_EmitIntToPtr(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name);
LLVMValueRef Xvr_EmitPtrToInt(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name);
LLVMValueRef Xvr_EmitPtrToPtr(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name);
LLVMValueRef Xvr_EmitBoolCast(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name);

bool Xvr_CastEmitterCheckSafety(Xvr_LLVMCastEmitter* emitter,
                                Xvr_LLVMCastType cast_type,
                                bool enable_runtime_checks);

const char* Xvr_LLVMCastTypeToString(Xvr_LLVMCastType type);

#endif
