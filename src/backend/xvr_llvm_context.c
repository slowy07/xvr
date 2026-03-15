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

#include "xvr_llvm_context.h"

#include <stdlib.h>
#include <string.h>

#include "xvr_common.h"

struct Xvr_LLVMContext {
    LLVMContextRef llvm_context;
    char* error_message;
    bool has_error;
};

Xvr_LLVMContext* Xvr_LLVMContextCreate(void) {
    Xvr_LLVMContext* ctx = calloc(1, sizeof(Xvr_LLVMContext));
    if (!ctx) {
        return NULL;
    }
    ctx->llvm_context = LLVMContextCreate();
    ctx->has_error = false;
    ctx->error_message = NULL;
    return ctx;
}

void Xvr_LLVMContextDestroy(Xvr_LLVMContext* ctx) {
    if (!ctx) {
        return;
    }
    if (ctx->llvm_context) {
        LLVMContextDispose(ctx->llvm_context);
    }
    if (ctx->error_message) {
        free(ctx->error_message);
    }
    free(ctx);
}

LLVMContextRef Xvr_LLVMContextGetLLVMContext(Xvr_LLVMContext* ctx) {
    if (!ctx) {
        return NULL;
    }
    return ctx->llvm_context;
}

bool Xvr_LLVMContextHasError(Xvr_LLVMContext* ctx) {
    return ctx && ctx->has_error;
}

const char* Xvr_LLVMContextGetErrorMessage(Xvr_LLVMContext* ctx) {
    if (!ctx) {
        return NULL;
    }
    return ctx->error_message;
}

void Xvr_LLVMContextSetError(Xvr_LLVMContext* ctx, const char* message) {
    if (!ctx) {
        return;
    }
    if (ctx->error_message) {
        free(ctx->error_message);
    }
    if (message) {
        ctx->error_message = Xvr_strdup(message);
    } else {
        ctx->error_message = NULL;
    }
    ctx->has_error = (ctx->error_message != NULL);
}

void Xvr_LLVMContextClearError(Xvr_LLVMContext* ctx) {
    if (!ctx) {
        return;
    }
    if (ctx->error_message) {
        free(ctx->error_message);
        ctx->error_message = NULL;
    }
    ctx->has_error = false;
}
