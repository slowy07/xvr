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

#pragma once

#include "xvr_ast_node.h"
#include "xvr_common.h"
#include "xvr_literal_array.h"
#include "xvr_opcodes.h"

typedef struct Xvr_Compiler {
    Xvr_LiteralArray literalCache;
    unsigned char* bytecode;
    int capacity;
    int count;
} Xvr_Compiler;

XVR_API void Xvr_initCompiler(Xvr_Compiler* compiler);
XVR_API void Xvr_writeCompiler(Xvr_Compiler* compiler, Xvr_ASTNode* node);
XVR_API void Xvr_freeCompiler(Xvr_Compiler* compiler);

XVR_API unsigned char* Xvr_collateCompiler(Xvr_Compiler* compiler, int* size);

