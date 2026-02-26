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

#include "xvr_llvm_type_mapper.h"

#include <stdlib.h>

#include "xvr_literal.h"

struct Xvr_LLVMTypeMapper {
    Xvr_LLVMContext* context;
    LLVMTypeRef void_type;
    LLVMTypeRef int1_type;
    LLVMTypeRef int8_type;
    LLVMTypeRef int16_type;
    LLVMTypeRef int32_type;
    LLVMTypeRef int64_type;
    LLVMTypeRef float_type;
    LLVMTypeRef double_type;
    LLVMTypeRef int8_ptr_type;
    LLVMTypeRef runtime_type;
};

Xvr_LLVMTypeMapper* Xvr_LLVMTypeMapperCreate(Xvr_LLVMContext* ctx) {
    if (!ctx) {
        return NULL;
    }

    Xvr_LLVMTypeMapper* mapper = calloc(1, sizeof(Xvr_LLVMTypeMapper));
    if (!mapper) {
        return NULL;
    }

    mapper->context = ctx;
    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(ctx);

    mapper->void_type = LLVMVoidTypeInContext(llvm_ctx);
    mapper->int1_type = LLVMInt1TypeInContext(llvm_ctx);
    mapper->int8_type = LLVMInt8TypeInContext(llvm_ctx);
    mapper->int16_type = LLVMInt16TypeInContext(llvm_ctx);
    mapper->int32_type = LLVMInt32TypeInContext(llvm_ctx);
    mapper->int64_type = LLVMInt64TypeInContext(llvm_ctx);
    mapper->float_type = LLVMFloatTypeInContext(llvm_ctx);
    mapper->double_type = LLVMDoubleTypeInContext(llvm_ctx);
    mapper->int8_ptr_type = LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0);

    mapper->runtime_type = LLVMStructCreateNamed(llvm_ctx, "XvrRuntimeValue");
    LLVMTypeRef runtime_fields[] = {
        LLVMPointerType(LLVMInt8TypeInContext(llvm_ctx), 0),
        LLVMInt64TypeInContext(llvm_ctx)};
    LLVMStructSetBody(mapper->runtime_type, runtime_fields, 2, false);

    return mapper;
}

void Xvr_LLVMTypeMapperDestroy(Xvr_LLVMTypeMapper* mapper) { free(mapper); }

LLVMTypeRef Xvr_LLVMTypeMapperGetType(Xvr_LLVMTypeMapper* mapper,
                                      Xvr_LiteralType type) {
    if (!mapper) {
        return NULL;
    }

    switch (type) {
    case XVR_LITERAL_NULL:
        return mapper->int8_ptr_type;
    case XVR_LITERAL_BOOLEAN:
        return mapper->int1_type;
    case XVR_LITERAL_INTEGER:
        return mapper->int32_type;
    case XVR_LITERAL_INT8:
        return mapper->int8_type;
    case XVR_LITERAL_INT16:
        return mapper->int16_type;
    case XVR_LITERAL_INT32:
        return mapper->int32_type;
    case XVR_LITERAL_INT64:
        return mapper->int64_type;
    case XVR_LITERAL_UINT8:
        return mapper->int8_type;
    case XVR_LITERAL_UINT16:
        return mapper->int16_type;
    case XVR_LITERAL_UINT32:
        return mapper->int32_type;
    case XVR_LITERAL_UINT64:
        return mapper->int64_type;
    case XVR_LITERAL_FLOAT:
        return mapper->float_type;
    case XVR_LITERAL_FLOAT16:
        return mapper->int16_type;
    case XVR_LITERAL_FLOAT32:
        return mapper->float_type;
    case XVR_LITERAL_FLOAT64:
        return mapper->double_type;
    case XVR_LITERAL_STRING:
        return mapper->int8_ptr_type;
    case XVR_LITERAL_ARRAY:
    case XVR_LITERAL_DICTIONARY:
    case XVR_LITERAL_FUNCTION:
    case XVR_LITERAL_FUNCTION_NATIVE:
    case XVR_LITERAL_FUNCTION_HOOK:
        return mapper->int8_ptr_type;
    default:
        return mapper->int8_ptr_type;
    }
}

LLVMTypeRef Xvr_LLVMTypeMapperGetPointerType(Xvr_LLVMTypeMapper* mapper,
                                             Xvr_LiteralType type) {
    if (!mapper) {
        return NULL;
    }

    LLVMTypeRef element_type = Xvr_LLVMTypeMapperGetType(mapper, type);
    if (!element_type) {
        return NULL;
    }

    return LLVMPointerType(element_type, 0);
}

LLVMTypeRef Xvr_LLVMTypeMapperGetRuntimeType(Xvr_LLVMTypeMapper* mapper) {
    if (!mapper) {
        return NULL;
    }
    return mapper->runtime_type;
}

bool Xvr_LLVMTypeMapperIsSigned(Xvr_LiteralType type) {
    switch (type) {
    case XVR_LITERAL_INTEGER:
    case XVR_LITERAL_INT8:
    case XVR_LITERAL_INT16:
    case XVR_LITERAL_INT32:
    case XVR_LITERAL_INT64:
    case XVR_LITERAL_FLOAT:
    case XVR_LITERAL_FLOAT16:
    case XVR_LITERAL_FLOAT32:
    case XVR_LITERAL_FLOAT64:
        return true;
    case XVR_LITERAL_UINT8:
    case XVR_LITERAL_UINT16:
    case XVR_LITERAL_UINT32:
    case XVR_LITERAL_UINT64:
        return false;
    default:
        return false;
    }
}
