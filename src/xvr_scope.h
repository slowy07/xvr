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

#ifndef XVR_SCOPE_H
#define XVR_SCOPE_H

#include "xvr_literal_array.h"
#include "xvr_literal_dictionary.h"

typedef struct Xvr_Scope {
    Xvr_LiteralDictionary variables;
    Xvr_LiteralDictionary types;
    struct Xvr_Scope* ancestor;
    int references;
} Xvr_Scope;

Xvr_Scope* Xvr_pushScope(Xvr_Scope* scope);
Xvr_Scope* Xvr_popScope(Xvr_Scope* scope);
Xvr_Scope* Xvr_copyScope(Xvr_Scope* original);

bool Xvr_declareScopeVariable(Xvr_Scope* scope, Xvr_Literal key,
                              Xvr_Literal type);
bool Xvr_isDeclaredScopeVariable(Xvr_Scope* scope, Xvr_Literal key);

bool Xvr_setScopeVariable(Xvr_Scope* scope, Xvr_Literal key, Xvr_Literal value,
                          bool constCheck);
bool Xvr_getScopeVariable(Xvr_Scope* scope, Xvr_Literal key,
                          Xvr_Literal* value);

Xvr_Literal Xvr_getScopeType(Xvr_Scope* scope, Xvr_Literal key);

#endif  // !XVR_SCOPE_H
