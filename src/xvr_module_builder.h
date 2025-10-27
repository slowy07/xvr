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

#ifndef XVR_ROUTINE_H
#define XVR_ROUTINE_H

#include "xvr_ast.h"
#include "xvr_common.h"

#ifndef XVR_ESCAPE_INITIAL_CAPACITY
#    define XVR_ESCAPE_INITIAL_CAPACITY 32
#endif  // !XVR_ESCAPE_INITIAL_CAPACITY

#ifndef XVR_ESCAPE_EXPANSION_RATE
#    define XVR_ESCAPE_EXPANSION_RATE 4
#endif  // !XVR_ESCAPE_EXPANSION_RATE

typedef struct Xvr_private_EscapeEntry_t {
    unsigned int addr;   // the address to write *to*
    unsigned int depth;  // the current depth
} Xvr_private_EscapeEntry_t;

typedef struct Xvr_private_EscapeArray {
    unsigned int capacity;
    unsigned int count;
    Xvr_private_EscapeEntry_t data[];
} Xvr_private_EscapeArray;

XVR_API void* Xvr_private_resizeEscapeArray(Xvr_private_EscapeArray* ptr,
                                            unsigned int capacity);

// structure for holding the module as it is built
typedef struct Xvr_ModuleBuilder {
    unsigned char* code;  // the instruction set
    unsigned int codeCapacity;
    unsigned int codeCount;

    unsigned char* jumps;  // each 'jump' is the starting address of an element
                           // within 'data'
    unsigned int jumpsCapacity;
    unsigned int jumpsCount;

    unsigned char* param;  // each 'param' is the starting address of a name
                           // string within 'data'
    unsigned int paramCapacity;
    unsigned int paramCount;

    unsigned char* data;  // a block of read-only data
    unsigned int dataCapacity;
    unsigned int dataCount;

    unsigned char* subs;  // submodules, built recursively
    unsigned int subsCapacity;
    unsigned int subsCount;

    // tools for handling the build process
    unsigned int currentScopeDepth;
    Xvr_private_EscapeArray* breakEscapes;
    Xvr_private_EscapeArray* continueEscapes;

    // compilation errors
    bool panic;
} Xvr_ModuleBuilder;

XVR_API void* Xvr_compileModuleBuilder(Xvr_Ast* ast);
#endif  // !XVR_ROUTINE_H
