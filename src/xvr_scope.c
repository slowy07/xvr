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

#include "xvr_scope.h"

#include <stdatomic.h>

#include "xvr_literal.h"
#include "xvr_literal_dictionary.h"
#include "xvr_memory.h"

static void freeAncestorChain(Xvr_Scope* scope) {
    scope->references--;

    if (scope->ancestor != NULL) {
        freeAncestorChain(scope->ancestor);
    }

    if (scope->references > 0) {
        return;
    }

    Xvr_freeLiteralDictionary(&scope->variables);
    Xvr_freeLiteralDictionary(&scope->types);

    XVR_FREE(Xvr_Scope, scope);
}

// return false if invalid type
static bool checkType(Xvr_Literal typeLiteral, Xvr_Literal original,
                      Xvr_Literal value, bool constCheck) {
    // for constants, fail if original != value
    if (constCheck && XVR_AS_TYPE(typeLiteral).constant &&
        !Xvr_literalsAreEqual(original, value)) {
        return false;
    }

    // for any types
    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_ANY) {
        return true;
    }

    // don't allow null types
    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_NULL) {
        return false;
    }

    // always allow null values
    if (XVR_IS_NULL(value)) {
        return true;
    }

    // for each type, if a mismatch is found, return false
    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_BOOLEAN &&
        !XVR_IS_BOOLEAN(value)) {
        return false;
    }

    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_INTEGER &&
        !XVR_IS_INTEGER(value)) {
        return false;
    }

    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_FLOAT &&
        !XVR_IS_FLOAT(value)) {
        return false;
    }

    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_STRING &&
        !XVR_IS_STRING(value)) {
        return false;
    }

    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_ARRAY &&
        !XVR_IS_ARRAY(value)) {
        return false;
    }

    if (XVR_IS_ARRAY(value)) {
        // check value's type
        if (XVR_AS_TYPE(typeLiteral).typeOf != XVR_LITERAL_ARRAY) {
            return false;
        }

        // if null, assume it's a new array variable that needs checking
        if (XVR_IS_NULL(original)) {
            for (int i = 0; i < XVR_AS_ARRAY(value)->count; i++) {
                if (!checkType(
                        ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[0],
                        XVR_TO_NULL_LITERAL, XVR_AS_ARRAY(value)->literals[i],
                        constCheck)) {
                    return false;
                }
            }

            return true;
        }

        // check children
        for (int i = 0; i < XVR_AS_ARRAY(value)->count; i++) {
            if (XVR_AS_ARRAY(original)->count <= i) {
                return true;  // assume new entry pushed
            }

            if (!checkType(
                    ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[0],
                    XVR_AS_ARRAY(original)->literals[i],
                    XVR_AS_ARRAY(value)->literals[i], constCheck)) {
                return false;
            }
        }
    }

    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_DICTIONARY &&
        !XVR_IS_DICTIONARY(value)) {
        return false;
    }

    if (XVR_IS_DICTIONARY(value)) {
        // check value's type
        if (XVR_AS_TYPE(typeLiteral).typeOf != XVR_LITERAL_DICTIONARY) {
            return false;
        }

        // if null, assume it's a new dictionary variable that needs checking
        if (XVR_IS_NULL(original)) {
            for (int i = 0; i < XVR_AS_DICTIONARY(value)->capacity; i++) {
                // check the type of key and value
                if (!checkType(
                        ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[0],
                        XVR_TO_NULL_LITERAL,
                        XVR_AS_DICTIONARY(value)->entries[i].key, constCheck)) {
                    return false;
                }

                if (!checkType(
                        ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[1],
                        XVR_TO_NULL_LITERAL,
                        XVR_AS_DICTIONARY(value)->entries[i].value,
                        constCheck)) {
                    return false;
                }
            }

            return true;
        }

        // check each child of value against the child of original
        for (int i = 0; i < XVR_AS_DICTIONARY(value)->capacity; i++) {
            if (XVR_IS_NULL(XVR_AS_DICTIONARY(value)
                                ->entries[i]
                                .key)) {  // only non-tombstones
                continue;
            }

            // find the internal child of original that matches this child of
            // value
            Xvr_private_entry* ptr = NULL;

            for (int j = 0; j < XVR_AS_DICTIONARY(original)->capacity; j++) {
                if (Xvr_literalsAreEqual(
                        XVR_AS_DICTIONARY(original)->entries[j].key,
                        XVR_AS_DICTIONARY(value)->entries[i].key)) {
                    ptr = &XVR_AS_DICTIONARY(original)->entries[j];
                    break;
                }
            }

            // if not found, assume it's a new entry
            if (!ptr) {
                continue;
            }

            // check the type of key and value
            if (!checkType(
                    ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[0],
                    ptr->key, XVR_AS_DICTIONARY(value)->entries[i].key,
                    constCheck)) {
                return false;
            }

            if (!checkType(
                    ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[1],
                    ptr->value, XVR_AS_DICTIONARY(value)->entries[i].value,
                    constCheck)) {
                return false;
            }
        }
    }

    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_FUNCTION &&
        !XVR_IS_FUNCTION(value)) {
        return false;
    }

    if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_TYPE &&
        !XVR_IS_TYPE(value)) {
        return false;
    }

    return true;
}

// exposed functions
Xvr_Scope* Xvr_pushScope(Xvr_Scope* ancestor) {
    Xvr_Scope* scope = XVR_ALLOCATE(Xvr_Scope, 1);
    scope->ancestor = ancestor;
    Xvr_initLiteralDictionary(&scope->variables);
    Xvr_initLiteralDictionary(&scope->types);

    // tick up all scope reference counts
    scope->references = 0;
    for (Xvr_Scope* ptr = scope; ptr != NULL; ptr = ptr->ancestor) {
        ptr->references++;
    }

    return scope;
}

Xvr_Scope* Xvr_popScope(Xvr_Scope* scope) {
    if (scope == NULL) {  // CAN pop a null
        return NULL;
    }

    Xvr_Scope* ret = scope->ancestor;

    // INFO: when freeing a scope, free the function's scopes manually
    for (int i = 0; i < scope->variables.capacity; i++) {
        // handle keys, just in case
        if (XVR_IS_FUNCTION(scope->variables.entries[i].key)) {
            Xvr_popScope(
                XVR_AS_FUNCTION(scope->variables.entries[i].key).scope);
            XVR_AS_FUNCTION(scope->variables.entries[i].key).scope = NULL;
        }

        if (XVR_IS_FUNCTION(scope->variables.entries[i].value)) {
            Xvr_popScope(
                XVR_AS_FUNCTION(scope->variables.entries[i].value).scope);
            XVR_AS_FUNCTION(scope->variables.entries[i].value).scope = NULL;
        }
    }

    freeAncestorChain(scope);

    return ret;
}

Xvr_Scope* Xvr_copyScope(Xvr_Scope* original) {
    Xvr_Scope* scope = XVR_ALLOCATE(Xvr_Scope, 1);
    scope->ancestor = original->ancestor;
    Xvr_initLiteralDictionary(&scope->variables);
    Xvr_initLiteralDictionary(&scope->types);

    // tick up all scope reference counts
    scope->references = 0;
    for (Xvr_Scope* ptr = scope; ptr != NULL; ptr = ptr->ancestor) {
        ptr->references++;
    }

    // copy the contents of the dictionaries
    for (int i = 0; i < original->variables.capacity; i++) {
        if (!XVR_IS_NULL(original->variables.entries[i].key)) {
            Xvr_setLiteralDictionary(&scope->variables,
                                     original->variables.entries[i].key,
                                     original->variables.entries[i].value);
        }
    }

    for (int i = 0; i < original->types.capacity; i++) {
        if (!XVR_IS_NULL(original->types.entries[i].key)) {
            Xvr_setLiteralDictionary(&scope->types,
                                     original->types.entries[i].key,
                                     original->types.entries[i].value);
        }
    }

    return scope;
}

// returns false if error
bool Xvr_declareScopeVariable(Xvr_Scope* scope, Xvr_Literal key,
                              Xvr_Literal type) {
    // don't redefine a variable within this scope
    if (Xvr_existsLiteralDictionary(&scope->variables, key)) {
        return false;
    }

    if (!XVR_IS_TYPE(type)) {
        return false;
    }

    // store the type, for later checking on assignment
    Xvr_setLiteralDictionary(&scope->types, key, type);

    Xvr_setLiteralDictionary(&scope->variables, key, XVR_TO_NULL_LITERAL);
    return true;
}

bool Xvr_isDeclaredScopeVariable(Xvr_Scope* scope, Xvr_Literal key) {
    if (scope == NULL) {
        return false;
    }

    // if it's not in this scope, keep searching up the chain
    if (!Xvr_existsLiteralDictionary(&scope->variables, key)) {
        return Xvr_isDeclaredScopeVariable(scope->ancestor, key);
    }

    return true;
}

// return false if undefined, or can't be assigned
bool Xvr_setScopeVariable(Xvr_Scope* scope, Xvr_Literal key, Xvr_Literal value,
                          bool constCheck) {
    // dead end
    if (scope == NULL) {
        return false;
    }

    // if it's not in this scope, keep searching up the chain
    if (!Xvr_existsLiteralDictionary(&scope->variables, key)) {
        return Xvr_setScopeVariable(scope->ancestor, key, value, constCheck);
    }

    // type checking
    Xvr_Literal typeLiteral = Xvr_getLiteralDictionary(&scope->types, key);
    Xvr_Literal original = Xvr_getLiteralDictionary(&scope->variables, key);

    if (!checkType(typeLiteral, original, value, constCheck)) {
        Xvr_freeLiteral(typeLiteral);
        Xvr_freeLiteral(original);
        return false;
    }

    // actually assign
    Xvr_setLiteralDictionary(&scope->variables, key, value);

    Xvr_freeLiteral(typeLiteral);
    Xvr_freeLiteral(original);

    return true;
}

bool Xvr_getScopeVariable(Xvr_Scope* scope, Xvr_Literal key,
                          Xvr_Literal* valueHandle) {
    // dead end
    if (scope == NULL) {
        return false;
    }

    if (!Xvr_existsLiteralDictionary(&scope->variables, key)) {
        return Xvr_getScopeVariable(scope->ancestor, key, valueHandle);
    }

    *valueHandle = Xvr_getLiteralDictionary(&scope->variables, key);
    return true;
}

Xvr_Literal Xvr_getScopeType(Xvr_Scope* scope, Xvr_Literal key) {
    // dead end
    if (scope == NULL) {
        return XVR_TO_NULL_LITERAL;
    }

    // if it's not in this scope, keep searching up the chain
    if (!Xvr_existsLiteralDictionary(&scope->types, key)) {
        return Xvr_getScopeType(scope->ancestor, key);
    }

    return Xvr_getLiteralDictionary(&scope->types, key);
}
