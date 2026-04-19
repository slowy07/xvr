/**
 * MIT License
 * Copyright (c) 2025 arfy slowy
 *
 * @file xvr_cast_emit.c
 * @brief LLVM cast lowering implementation
 */

#include "xvr_cast_emit.h"

#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>

#include "xvr_type.h"

struct Xvr_LLVMCastEmitter {
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    bool runtime_checks_enabled;
};

Xvr_LLVMCastEmitter* Xvr_LLVMCastEmitterCreate(LLVMModuleRef module,
                                               LLVMBuilderRef builder) {
    Xvr_LLVMCastEmitter* emitter = (Xvr_LLVMCastEmitter*)calloc(1, sizeof(Xvr_LLVMCastEmitter));
    if (!emitter) return NULL;

    emitter->module = module;
    emitter->builder = builder;
    emitter->runtime_checks_enabled = false;

    return emitter;
}

void Xvr_LLVMCastEmitterDestroy(Xvr_LLVMCastEmitter* emitter) { free(emitter); }

Xvr_LLVMCastType Xvr_CastEmitterClassify(Xvr_Type* from, Xvr_Type* to) {
    if (!from || !to) return XVR_LLVM_CAST_INVALID;

    bool from_bool = from->kind == XVR_KIND_BOOL;
    bool to_bool = to->kind == XVR_KIND_BOOL;
    bool from_int = Xvr_TypeIsInteger(from);
    bool to_int = Xvr_TypeIsInteger(to);
    bool from_float = Xvr_TypeIsFloat(from);
    bool to_float = Xvr_TypeIsFloat(to);
    bool from_ptr = Xvr_TypeIsPointer(from);
    bool to_ptr = Xvr_TypeIsPointer(to);

    if (from_bool && to_int) return XVR_LLVM_CAST_BOOL_TO_INT;
    if (from_bool && to_float) return XVR_LLVM_CAST_BOOL_TO_FLOAT;
    if (from_int && to_bool) return XVR_LLVM_CAST_INT_TO_BOOL;
    if (from_float && to_bool) return XVR_LLVM_CAST_FLOAT_TO_BOOL;

    if (from_int && to_int) return XVR_LLVM_CAST_INT_TO_INT;
    if (from_int && to_float) return XVR_LLVM_CAST_INT_TO_FLOAT;
    if (from_float && to_int) return XVR_LLVM_CAST_FLOAT_TO_INT;
    if (from_float && to_float) return XVR_LLVM_CAST_FLOAT_TO_FLOAT;

    if (from_int && to_ptr) return XVR_LLVM_CAST_INT_TO_PTR;
    if (from_ptr && to_int) return XVR_LLVM_CAST_PTR_TO_INT;
    if (from_ptr && to_ptr) return XVR_LLVM_CAST_PTR_TO_PTR;

    return XVR_LLVM_CAST_INVALID;
}

LLVMValueRef Xvr_EmitCast(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                          Xvr_Type* from, Xvr_Type* to, const char* name) {
    if (!emitter || !value || !from || !to) return NULL;

    Xvr_LLVMCastType cast_type = Xvr_CastEmitterClassify(from, to);

    if (cast_type == XVR_LLVM_CAST_INVALID) {
        fprintf(stderr, "Invalid cast from %s to %s\n", Xvr_TypeToString(from),
                Xvr_TypeToString(to));
        return NULL;
    }

    switch (cast_type) {
    case XVR_LLVM_CAST_BOOL_TO_INT:
    case XVR_LLVM_CAST_BOOL_TO_FLOAT:
        return Xvr_EmitBoolCast(emitter, value, from, to, name);
    case XVR_LLVM_CAST_INT_TO_BOOL:
    case XVR_LLVM_CAST_FLOAT_TO_BOOL:
        return Xvr_EmitBoolCast(emitter, value, from, to, name);
    case XVR_LLVM_CAST_INT_TO_INT:
        return Xvr_EmitIntToInt(emitter, value, from, to, name);
    case XVR_LLVM_CAST_INT_TO_FLOAT:
        return Xvr_EmitIntToFloat(emitter, value, from, to, name);
    case XVR_LLVM_CAST_FLOAT_TO_INT:
        return Xvr_EmitFloatToInt(emitter, value, from, to, name);
    case XVR_LLVM_CAST_FLOAT_TO_FLOAT:
        return Xvr_EmitFloatToFloat(emitter, value, from, to, name);
    case XVR_LLVM_CAST_INT_TO_PTR:
        return Xvr_EmitIntToPtr(emitter, value, from, to, name);
    case XVR_LLVM_CAST_PTR_TO_INT:
        return Xvr_EmitPtrToInt(emitter, value, from, to, name);
    case XVR_LLVM_CAST_PTR_TO_PTR:
        return Xvr_EmitPtrToPtr(emitter, value, from, to, name);
    default:
        return NULL;
    }
}

LLVMValueRef Xvr_EmitIntToInt(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name) {
    if (!emitter || !value || !from || !to) return NULL;

    LLVMBuilderRef builder = emitter->builder;
    int from_bits = Xvr_TypeGetSizeBits(from);
    int to_bits = Xvr_TypeGetSizeBits(to);

    if (from_bits == to_bits) {
        return value;
    }

    if (to_bits > from_bits) {
        Xvr_Signedness sign = Xvr_TypeGetSignedness(from);
        if (sign == XVR_SIGNEDNESS_UNSIGNED) {
            return LLVMBuildZExt(
                builder, value,
                LLVMIntTypeInContext(LLVMGetModuleContext(emitter->module),
                                     to_bits),
                name);
        } else {
            return LLVMBuildSExt(
                builder, value,
                LLVMIntTypeInContext(LLVMGetModuleContext(emitter->module),
                                     to_bits),
                name);
        }
    } else {
        return LLVMBuildTrunc(
            builder, value,
            LLVMIntTypeInContext(LLVMGetModuleContext(emitter->module),
                                 to_bits),
            name);
    }
}

LLVMValueRef Xvr_EmitIntToFloat(Xvr_LLVMCastEmitter* emitter,
                                LLVMValueRef value, Xvr_Type* from,
                                Xvr_Type* to, const char* name) {
    if (!emitter || !value || !from || !to) return NULL;

    LLVMBuilderRef builder = emitter->builder;
    LLVMContextRef ctx = LLVMGetModuleContext(emitter->module);
    Xvr_Signedness sign = Xvr_TypeGetSignedness(from);
    int float_bits = Xvr_TypeGetSizeBits(to);

    LLVMTypeRef float_type = (float_bits == 64) ? LLVMDoubleTypeInContext(ctx)
                                                : LLVMFloatTypeInContext(ctx);

    if (sign == XVR_SIGNEDNESS_UNSIGNED) {
        return LLVMBuildUIToFP(builder, value, float_type, name);
    } else {
        return LLVMBuildSIToFP(builder, value, float_type, name);
    }
}

LLVMValueRef Xvr_EmitFloatToInt(Xvr_LLVMCastEmitter* emitter,
                                LLVMValueRef value, Xvr_Type* from,
                                Xvr_Type* to, const char* name) {
    if (!emitter || !value || !from || !to) return NULL;

    LLVMBuilderRef builder = emitter->builder;
    LLVMContextRef ctx = LLVMGetModuleContext(emitter->module);
    Xvr_Signedness sign = Xvr_TypeGetSignedness(to);
    int int_bits = Xvr_TypeGetSizeBits(to);

    LLVMTypeRef int_type = LLVMIntTypeInContext(ctx, int_bits);

    if (sign == XVR_SIGNEDNESS_UNSIGNED) {
        return LLVMBuildFPToUI(builder, value, int_type, name);
    } else {
        return LLVMBuildFPToSI(builder, value, int_type, name);
    }
}

LLVMValueRef Xvr_EmitFloatToFloat(Xvr_LLVMCastEmitter* emitter,
                                  LLVMValueRef value, Xvr_Type* from,
                                  Xvr_Type* to, const char* name) {
    if (!emitter || !value || !from || !to) return NULL;

    int from_bits = Xvr_TypeGetSizeBits(from);
    int to_bits = Xvr_TypeGetSizeBits(to);

    if (from_bits == to_bits) {
        return value;
    }

    LLVMContextRef ctx = LLVMGetModuleContext(emitter->module);
    LLVMTypeRef llvm_to = (to_bits == 64) ? LLVMDoubleTypeInContext(ctx)
                                          : LLVMFloatTypeInContext(ctx);

    return LLVMBuildFPCast(emitter->builder, value, llvm_to, name);
}

LLVMValueRef Xvr_EmitIntToPtr(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name) {
    if (!emitter || !value || !from || !to) return NULL;

    return LLVMBuildIntToPtr(
        emitter->builder, value,
        LLVMPointerType(
            LLVMInt8TypeInContext(LLVMGetModuleContext(emitter->module)), 0),
        name);
}

LLVMValueRef Xvr_EmitPtrToInt(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name) {
    if (!emitter || !value || !from || !to) return NULL;

    int int_bits = Xvr_TypeGetSizeBits(to);
    return LLVMBuildPtrToInt(
        emitter->builder, value,
        LLVMIntTypeInContext(LLVMGetModuleContext(emitter->module), int_bits),
        name);
}

LLVMValueRef Xvr_EmitPtrToPtr(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name) {
    if (!emitter || !value || !from || !to) return NULL;

    return LLVMBuildBitCast(
        emitter->builder, value,
        LLVMPointerType(
            LLVMInt8TypeInContext(LLVMGetModuleContext(emitter->module)), 0),
        name);
}

LLVMValueRef Xvr_EmitBoolCast(Xvr_LLVMCastEmitter* emitter, LLVMValueRef value,
                              Xvr_Type* from, Xvr_Type* to, const char* name) {
    if (!emitter || !value || !from || !to) return NULL;

    LLVMBuilderRef builder = emitter->builder;
    LLVMContextRef ctx = LLVMGetModuleContext(emitter->module);
    bool from_bool = from->kind == XVR_KIND_BOOL;
    bool to_bool = to->kind == XVR_KIND_BOOL;

    if (from_bool && to_bool) {
        return value;
    }

    if (from_bool && Xvr_TypeIsInteger(to)) {
        int bits = Xvr_TypeGetSizeBits(to);
        LLVMTypeRef int_type = LLVMIntTypeInContext(ctx, bits);
        return LLVMBuildZExt(builder, value, int_type, name);
    }

    if (from_bool && Xvr_TypeIsFloat(to)) {
        int bits = Xvr_TypeGetSizeBits(to);
        LLVMTypeRef float_type = (bits == 64) ? LLVMDoubleTypeInContext(ctx)
                                              : LLVMFloatTypeInContext(ctx);
        return LLVMBuildUIToFP(builder, value, float_type, name);
    }

    if (Xvr_TypeIsInteger(from) && to_bool) {
        LLVMValueRef zero = LLVMConstNull(LLVMTypeOf(value));
        return LLVMBuildICmp(builder, LLVMIntNE, value, zero, name);
    }

    if (Xvr_TypeIsFloat(from) && to_bool) {
        LLVMValueRef zero = LLVMConstNull(LLVMTypeOf(value));
        return LLVMBuildFCmp(builder, LLVMRealONE, value, zero, name);
    }

    return NULL;
}

bool Xvr_CastEmitterCheckSafety(Xvr_LLVMCastEmitter* emitter,
                                Xvr_LLVMCastType cast_type,
                                bool enable_runtime_checks) {
    if (!emitter) return false;

    emitter->runtime_checks_enabled = enable_runtime_checks;

    switch (cast_type) {
    case XVR_LLVM_CAST_FLOAT_TO_INT:
    case XVR_LLVM_CAST_PTR_TO_INT:
    case XVR_LLVM_CAST_PTR_TO_PTR:
        return enable_runtime_checks;
    default:
        return true;
    }
}

const char* Xvr_LLVMCastTypeToString(Xvr_LLVMCastType type) {
    switch (type) {
    case XVR_LLVM_CAST_INT_TO_INT:
        return "int->int";
    case XVR_LLVM_CAST_INT_TO_FLOAT:
        return "int->float";
    case XVR_LLVM_CAST_FLOAT_TO_INT:
        return "float->int";
    case XVR_LLVM_CAST_FLOAT_TO_FLOAT:
        return "float->float";
    case XVR_LLVM_CAST_INT_TO_PTR:
        return "int->ptr";
    case XVR_LLVM_CAST_PTR_TO_INT:
        return "ptr->int";
    case XVR_LLVM_CAST_PTR_TO_PTR:
        return "ptr->ptr";
    case XVR_LLVM_CAST_BOOL_TO_INT:
        return "bool->int";
    case XVR_LLVM_CAST_INT_TO_BOOL:
        return "int->bool";
    case XVR_LLVM_CAST_BOOL_TO_FLOAT:
        return "bool->float";
    case XVR_LLVM_CAST_FLOAT_TO_BOOL:
        return "float->bool";
    default:
        return "invalid";
    }
}
