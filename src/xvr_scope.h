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
 * @brief lexical scope management for the XVR runtime - variable, types and
 * nesting
 *
 * Xvr_Scope implement a linked list of dictionary representing lexical
 * environment
 * - each scope holds its own `variable` (name -> value) and `types` (name ->
 * type descriptor)
 * - reference enable manual reference counting
 *
 * memory ownership
 * - scope owns its variable and types dictionaries
 * - dictionaries own their `Xvr_Literal` keys / values (deep copiues on insert)
 */

#ifndef XVR_SCOPE_H
#define XVR_SCOPE_H

#include "xvr_literal_array.h"
#include "xvr_literal_dictionary.h"

/**
 * @struct Xvr_scope
 * @brief single lexical environment frame
 *
 * fields:
 *   - mapping from identifier
 *   - mapping from identifier to type descriptor
 *   - enclosing scope
 *   - reference count
 *
 * lifetime:
 *   - created by`Xvr_pushScope()` or `Xvr_copyScope()`
 *   - freed when reference drops to 0 in Xvr_popScope()
 *
 * @note for the size its ~56 bytes (2 dictionaries x 168  + ptrs)
 */
typedef struct Xvr_Scope {
    Xvr_LiteralDictionary variables;  // name -> value
    Xvr_LiteralDictionary types;      // name -> type descriptor
    struct Xvr_Scope* ancestor;       // enclosing scope
    int references;                   // reference count
} Xvr_Scope;

/**
 * @brief pushes new child scope into `scope`
 *
 * @param[in] scope current scope (may be NULL - create root scope)
 * @return new child with:
 *   - ancestor = scope
 *   - refrence = 1
 *   - empty variable and types
 *   - or NULL on allocation failure
 */
Xvr_Scope* Xvr_pushScope(Xvr_Scope* scope);

/**
 * @brief pop (release) a scope and returns its ancestor
 *
 * @param[in, out] scope scope to release (must not be NULL)
 * @return scope->ancestor or NULL if `scope` was root
 *
 * @note typical use: function return, block exit
 */
Xvr_Scope* Xvr_popScope(Xvr_Scope* scope);

/**
 * @brief creates a deep copy of original
 *
 * @param[in] original scope to copy
 * @return new idepdendent scope with
 *   - deep-copied variables and types
 * - same ancestor (shared, not copied)
 *   - reference = 1
 *   - or NULL or OOM
 *
 * @note used when creating closure that capture current scope
 */
Xvr_Scope* Xvr_copyScope(Xvr_Scope* original);

/**
 * @brief declare a variable name in `scope`
 *
 * @param[in, out] scope scope to modify (must not be NULL)
 * @param[in] key variable name (must be `XVR_LITERAL_IDENTIFIER`)
 * @param[in] type type descriptor
 * @return `true` on success, `false` if:
 *   - key is not an identifier
 * - name already declared in this scope
 *   - OOM during dictionary insert
 *
 * @note declaration does not search ancestor scope - local only
 */
bool Xvr_declareScopeVariable(Xvr_Scope* scope, Xvr_Literal key,
                              Xvr_Literal type);

/**
 * @brief check if key is declared in scope or any ancestor
 *
 * @param[in] scope scope to search
 * @param[in] key variable name
 * @return true if found in any scope in chain, false otherwise
 *
 * @note searches innermost -> outermost (lexical lookup order)
 */
bool Xvr_isDeclaredScopeVariable(Xvr_Scope* scope, Xvr_Literal key);

/**
 * @brief assing value to key in scope
 *
 * @param[in, out] scope scope to modify
 * @param[in] key variable name
 * @param[in] value value to assign
 * @return true on success, false if:
 *   - key not declared in scope chain
 *   - OOM
 */
bool Xvr_setScopeVariable(Xvr_Scope* scope, Xvr_Literal key, Xvr_Literal value,
                          bool constCheck);

/**
 * @brief retrieves value of key from scope chain
 *
 * @param[in] scope scope to search
 * @param[in] key variable name
 * @param[out] value pointer to store result
 * @return true if found, false if undeclared
 *
 * @note always return copy - ssafe for caller to modify / free searches
 * innermost -> outermost
 */
bool Xvr_getScopeVariable(Xvr_Scope* scope, Xvr_Literal key,
                          Xvr_Literal* value);

/**
 * @brief retrieves type of key from scope chain
 *
 * @param[in] scope scope to search
 * @param[in] key type name
 * @return Type descriptor if found, XVR_TO_NULL_LITERAL if not declared
 *
 * @note return a copy of the type literal (caller owns) use Xvr_freeLiteral()
 * when done
 */
Xvr_Literal Xvr_getScopeType(Xvr_Scope* scope, Xvr_Literal key);

#endif  // !XVR_SCOPE_H
