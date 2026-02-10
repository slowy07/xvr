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

/**
 * @brief tree-walking bytecode compiler for XVR runtime - AST to bytecode
 *
 * compile startegy:
 *   - pre-order traversal: visit parent before childern
 *   - immediate emission: generate bytecode as AST is traversed (no
 * intermediate representation)
 *   - literal deduplication: identiucal constant share slots in `literalCache`
 *   - backpatching: forward jumps, patched after target known
 *
 * memory management:
 *   - compiler owns bytecode buffer (grows as needed)
 *   - compiler owns literalCache (shared across all emitted code)
 *   - no ownership transfer - AST nodes remain owned by parser
 *
 * currently benefits:
 *  - fast compilation (single pass, without using IR)
 *  - small bytecode size (compact opcodes, deduplication literals)
 *  - REPL environments
 */

#ifndef XVR_COMPILER_H
#define XVR_COMPILER_H

#include <stdbool.h>

#include "xvr_ast_node.h"
#include "xvr_common.h"
#include "xvr_literal_array.h"
#include "xvr_opcodes.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @struct Xvr_Compiler
 * @brief compiler state machine - AST input to bytecode output
 *
 * @note: size ~40bytes - designed for stack allocation if needed
 */
typedef struct Xvr_Compiler {
    Xvr_LiteralArray
        literalCache;  // ppol of unique constant (shared across all code)
    unsigned char* bytecode;  // emitted bytecode buffer (grows as needed)
    int capacity;             // allocated size of `bytecode`
    int count;                // current size of `bytecode`
    bool panic;               // true if compilation error occurred
} Xvr_Compiler;

/**
 * @brief initializes compiler state with empty bytecode and literal cache
 *
 * @param[out] compiler compiler to initialize (must not be NULL)
 *
 * @note safe to call on zeroed memory, calling twice is safe (but overwrites
 * the old state which means its idempotent)
 */
XVR_API void Xvr_initCompiler(Xvr_Compiler* compiler);

/**
 * @brief free all compiler resource
 *
 * @param[in, out] compiler compiler to destroy (must not be NULL)
 *
 * @note safe to call even if `compiler->panic == true`, safe to call multiple
 * time which means are idempotent
 */
XVR_API void Xvr_writeCompiler(Xvr_Compiler* compiler, Xvr_ASTNode* node);

/**
 * @brief free all compiler resource
 *
 * @param[in, out] compiler compierl to destroy
 *
 * @note safe to call even if `compiler->panic == true` idempotent: safe to call
 * multiple times
 */
XVR_API void Xvr_freeCompiler(Xvr_Compiler* compiler);

/**
 * @brief finalize compilation and retunr bytecode buffer
 *
 * @param[in, out] compiler compiler state
 * @param[out] size pointer to store bytecode length
 * @return pointer to finalized bytecode buffer (owned by caller)
 *
 * @note caller owns returned buffer - must `free()` or pass to VM, compiler
 * state is reset
 */
XVR_API unsigned char* Xvr_collateCompiler(Xvr_Compiler* compiler, size_t* size);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !XVR_COMPILER_H
