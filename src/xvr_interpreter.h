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

#ifndef XVR_INTERPRETER_H
#define XVR_INTERPRETER_H

#include "xvr_common.h"
#include "xvr_literal.h"
#include "xvr_literal_array.h"
#include "xvr_literal_dictionary.h"
#include "xvr_scope.h"

typedef void (*Xvr_PrintFn)(const char*);

typedef struct Xvr_Interpreter {
    const unsigned char* bytecode;
    int length;
    int count;
    int codeStart;

    Xvr_LiteralArray literalCache;

    Xvr_Scope* scope;
    Xvr_LiteralArray stack;

    Xvr_LiteralDictionary* hooks;

    Xvr_PrintFn printOutput;
    Xvr_PrintFn assertOutput;
    Xvr_PrintFn errorOutput;

    int depth;
    bool panic;
} Xvr_Interpreter;

XVR_API bool Xvr_injectNativeFn(Xvr_Interpreter* interpreter, const char* name,
                                Xvr_NativeFn func);
XVR_API bool Xvr_injectNativeHook(Xvr_Interpreter* interpreter, const char* name,
                                  Xvr_HookFn hook);

XVR_API bool Xvr_callLiteralFn(Xvr_Interpreter* interpreter, Xvr_Literal func,
                               Xvr_LiteralArray* arguments,
                               Xvr_LiteralArray* returns);
XVR_API bool Xvr_callFn(Xvr_Interpreter* interpreter, const char* name,
                        Xvr_LiteralArray* arguments, Xvr_LiteralArray* returns);

XVR_API bool Xvr_parseIdentifierToValue(Xvr_Interpreter* interpreter,
                                        Xvr_Literal* literalPtr);
XVR_API void Xvr_setInterpreterPrint(Xvr_Interpreter* interpreter,
                                     Xvr_PrintFn printOutput);
XVR_API void Xvr_setInterpreterAssert(Xvr_Interpreter* interpreter,
                                      Xvr_PrintFn assertOutput);
XVR_API void Xvr_setInterpreterError(Xvr_Interpreter* interpreter,
                                     Xvr_PrintFn errorOutput);

XVR_API void Xvr_initInterpreter(Xvr_Interpreter* interpreter);
XVR_API void Xvr_runInterpreter(Xvr_Interpreter* interpreter,
                                const unsigned char* bytecode, int length);
XVR_API void Xvr_resetInterpreter(Xvr_Interpreter* interpreter);
XVR_API void Xvr_freeInterpreter(Xvr_Interpreter* interpreter);

#endif  // !XVR_INTERPRETER_H
