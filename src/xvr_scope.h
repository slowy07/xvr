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

#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_string.h"
#include "xvr_table.h"
#include "xvr_value.h"

typedef struct Xvr_Scope {
    struct Xvr_Scope* next;
    Xvr_Table* table;
    unsigned int refCount;
} Xvr_Scope;

XVR_API Xvr_Scope* Xvr_pushScope(Xvr_Bucket** bucketHandle, Xvr_Scope* scope);
XVR_API Xvr_Scope* Xvr_popScope(Xvr_Scope* scope);

XVR_API Xvr_Scope* Xvr_deepCopyScope(Xvr_Bucket** bucketHandle,
                                     Xvr_Scope* scope);

XVR_API void Xvr_declareScope(Xvr_Scope* scope, Xvr_String* key,
                              Xvr_Value value);
XVR_API void Xvr_assignScope(Xvr_Scope* scope, Xvr_String* key,
                             Xvr_Value value);
XVR_API Xvr_Value* Xvr_accessScopeAsPointer(Xvr_Scope* scope, Xvr_String* key);
XVR_API bool Xvr_isDeclaredScope(Xvr_Scope* scope, Xvr_String* key);

#endif  // !XVR_SCOPE_H
