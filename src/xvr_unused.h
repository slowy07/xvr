/**
MIT License

Copyright (c) 2025 arfy slowy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#ifndef XVR_UNUSED_H
#define XVR_UNUSED_H

#include <stdbool.h>

#include "xvr_ast_node.h"
#include "xvr_common.h"

/**
 * @struct Xvr_UnusedDecl
 * @brief Represents a single declaration (variable or procedure) being tracked.
 *
 * @var Xvr_UnusedDecl::identifier
 * The name of the declared variable or procedure.
 *
 * @var Xvr_UnusedDecl::line
 * The line number where the declaration appears (1-indexed).
 *
 * @var Xvr_UnusedDecl::used
 * Set to true if the declaration is referenced anywhere in the code.
 *
 * @var Xvr_UnusedDecl::isFunction
 * Set to true if this is a procedure declaration, false for variable.
 */
typedef struct Xvr_UnusedDecl {
    Xvr_Literal identifier;
    int line;
    bool used;
    bool isFunction;
} Xvr_UnusedDecl;

/**
 * @struct Xvr_UnusedScope
 * @brief Represents a single scope containing declarations.
 *
 * Tracks all declarations within a scope (e.g., function body, block).
 *
 * @var Xvr_UnusedScope::declarations
 * Array of declarations in this scope.
 *
 * @var Xvr_UnusedScope::count
 * Number of active declarations in the array.
 *
 * @var Xvr_UnusedScope::capacity
 * Total allocated capacity for the declarations array.
 */
typedef struct Xvr_UnusedScope {
    Xvr_UnusedDecl* declarations;
    int count;
    int capacity;
} Xvr_UnusedScope;

/**
 * @struct Xvr_UnusedChecker
 * @brief Main checker structure for tracking unused declarations.
 *
 * Manages scope hierarchy and tracks errors found during checking.
 *
 * @var Xvr_UnusedChecker::scopes
 * Array of scopes (stack-based for scope management).
 *
 * @var Xvr_UnusedScope::scopeCount
 * Number of active scopes in the stack.
 *
 * @var Xvr_UnusedChecker::scopeCapacity
 * Total allocated capacity for scopes array.
 *
 * @var Xvr_UnusedChecker::hasError
 * Set to true if any unused declaration was found.
 */
typedef struct Xvr_UnusedChecker {
    Xvr_UnusedScope* scopes;
    int scopeCount;
    int scopeCapacity;
    bool hasError;
} Xvr_UnusedChecker;

/**
 * @brief Initializes an unused checker instance.
 *
 * Must be called before using the checker. Allocates internal data structures.
 *
 * @param checker Pointer to the checker structure to initialize.
 */
XVR_API void Xvr_initUnusedChecker(Xvr_UnusedChecker* checker);

/**
 * @brief Frees all memory associated with an unused checker.
 *
 * Call this when done using the checker to prevent memory leaks.
 *
 * @param checker Pointer to the checker to free.
 */
XVR_API void Xvr_freeUnusedChecker(Xvr_UnusedChecker* checker);

/**
 * @brief Begins a new scope for tracking declarations.
 *
 * Called when entering a new scope (e.g., function body, block statement).
 * All declarations after this call belong to this new scope until popScope
 * is called implicitly via Xvr_checkUnusedEnd.
 *
 * @param checker Pointer to the active checker.
 */
XVR_API void Xvr_checkUnusedBegin(Xvr_UnusedChecker* checker);

/**
 * @brief Checks a single AST node for declarations and references.
 *
 * Traverses the given AST node, recording any declarations found and
 * marking any referenced identifiers as used.
 *
 * @param checker Pointer to the active checker.
 * @param node Pointer to the AST node to check. Can be NULL.
 */
XVR_API void Xvr_checkUnusedNode(Xvr_UnusedChecker* checker, Xvr_ASTNode* node);

/**
 * @brief Ends checking and reports any unused declarations.
 *
 * Pops the current scope and reports all unused declarations within it
 * to stderr. Prints error messages in format:
 * "error: unused [variable|procedure] 'name' declared at line N"
 *
 * @param checker Pointer to the active checker.
 * @return true if no unused declarations were found, false otherwise.
 */
XVR_API bool Xvr_checkUnusedEnd(Xvr_UnusedChecker* checker);

#endif
