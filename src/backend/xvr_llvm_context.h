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

#ifndef XVR_LLVM_CONTEXT_H
#define XVR_LLVM_CONTEXT_H

#include <llvm-c/Core.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Opaque structure representing the LLVM context manager
 *
 * This structure encapsulates an LLVM context and provides
 * error handling capabilities. Each codegen session should
 * have its own context.
 *
 * Memory ownership: owned by caller, must be freed with
 * Xvr_LLVMContextDestroy()
 */
typedef struct Xvr_LLVMContext Xvr_LLVMContext;

/**
 * @brief Creates a new LLVM context
 * @return Pointer to new context, or NULL on allocation failure
 *
 * Thread safety: Not thread-safe, each thread should
 * create its own context
 *
 * @post Caller must call Xvr_LLVMContextDestroy() when done
 */
Xvr_LLVMContext* Xvr_LLVMContextCreate(void);

/**
 * @brief Destroys an LLVM context and frees all resources
 * @param ctx Context to destroy (can be NULL)
 *
 * This function:
 * - Disposes of the LLVM context
 * - Frees any owned error message
 * - Frees the context structure itself
 *
 * Safe to call with NULL pointer (idempotent)
 */
void Xvr_LLVMContextDestroy(Xvr_LLVMContext* ctx);

/**
 * @brief Gets the underlying LLVM context reference
 * @param ctx Our context wrapper
 * @return LLVM context reference, or NULL if ctx is NULL
 *
 * The returned reference is valid for the lifetime of ctx
 */
LLVMContextRef Xvr_LLVMContextGetLLVMContext(Xvr_LLVMContext* ctx);

/**
 * @brief Checks if an error has occurred in the context
 * @param ctx Context to check
 * @return true if error occurred, false otherwise
 *
 * Use this to check if any operation failed before
 * retrieving the error message
 */
bool Xvr_LLVMContextHasError(Xvr_LLVMContext* ctx);

/**
 * @brief Gets the current error message
 * @param ctx Context to query
 * @return Error message string, or NULL if no error
 *
 * Returned string is owned by the context and MUST NOT
 * be freed by the caller. The string is invalidated
 * on the next operation that sets an error or when
 * the context is destroyed.
 */
const char* Xvr_LLVMContextGetErrorMessage(Xvr_LLVMContext* ctx);

/**
 * @brief Clears any existing error state
 * @param ctx Context to clear error from
 *
 * Frees any owned error message string and resets
 * the error state. Safe to call even if no error
 * exists (idempotent).
 */
void Xvr_LLVMContextClearError(Xvr_LLVMContext* ctx);

#endif
