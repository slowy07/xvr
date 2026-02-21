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

#include "inter_tools.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib_about.h"
#include "lib_runner.h"
#include "lib_standard.h"
#include "lib_timer.h"
#include "xvr_ast_node.h"
#include "xvr_compiler.h"
#include "xvr_console_colors.h"
#include "xvr_interpreter.h"
#include "xvr_lexer.h"
#include "xvr_parser.h"
#include "xvr_unused.h"

const unsigned char* Xvr_readFile(const char* path, size_t* fileSize) {
    FILE* file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr,
                XVR_CC_ERROR "Could not open file \"%s\"\n" XVR_CC_RESET, path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    *fileSize = ftell(file);
    rewind(file);

    unsigned char* buffer = (unsigned char*)malloc(*fileSize + 1);

    if (buffer == NULL) {
        fprintf(stderr,
                XVR_CC_ERROR "Not enough memory to read \"%s\"\n" XVR_CC_RESET,
                path);
        return NULL;
    }

    size_t bytesRead = fread(buffer, sizeof(unsigned char), *fileSize, file);

    buffer[*fileSize] = '\0';  // NOTE: fread doesn't append this

    if (bytesRead < *fileSize) {
        fprintf(stderr,
                XVR_CC_ERROR "Could not read file \"%s\"\n" XVR_CC_RESET, path);
        return NULL;
    }

    fclose(file);

    return buffer;
}

int Xvr_writeFile(const char* path, const unsigned char* bytes, size_t size) {
    FILE* file = fopen(path, "wb");

    if (file == NULL) {
        fprintf(stderr,
                XVR_CC_ERROR "Could not open file \"%s\"\n" XVR_CC_RESET, path);
        return -1;
    }

    int written = fwrite(bytes, size, 1, file);

    if (written != 1) {
        fprintf(stderr,
                XVR_CC_ERROR "Could not write file \"%s\"\n" XVR_CC_RESET,
                path);
        return -1;
    }

    fclose(file);

    return 0;
}

// repl functions
const unsigned char* Xvr_compileString(const char* source, size_t* size) {
    Xvr_Lexer lexer;
    Xvr_Parser parser;
    Xvr_Compiler compiler;

    Xvr_initLexer(&lexer, source);
    Xvr_initParser(&parser, &lexer);
    Xvr_initCompiler(&compiler);

    Xvr_ASTNode** nodes = NULL;
    int nodeCount = 0;
    int nodeCapacity = 0;

    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    while (node != NULL) {
        if (node->type == XVR_AST_NODE_ERROR) {
            Xvr_freeASTNode(node);
            for (int i = 0; i < nodeCount; i++) {
                Xvr_freeASTNode(nodes[i]);
            }
            free(nodes);
            Xvr_freeCompiler(&compiler);
            Xvr_freeParser(&parser);
            return NULL;
        }

        if (nodeCount >= nodeCapacity) {
            nodeCapacity = nodeCapacity < 8 ? 8 : nodeCapacity * 2;
            nodes = realloc(nodes, sizeof(Xvr_ASTNode*) * nodeCapacity);
        }
        nodes[nodeCount++] = node;
        node = Xvr_scanParser(&parser);
    }

    bool unusedError = false;
    Xvr_UnusedChecker checker;
    Xvr_initUnusedChecker(&checker);
    Xvr_checkUnusedBegin(&checker);

    for (int i = 0; i < nodeCount; i++) {
        Xvr_checkUnusedNode(&checker, nodes[i]);
    }

    if (!Xvr_checkUnusedEnd(&checker)) {
        unusedError = true;
    }
    Xvr_freeUnusedChecker(&checker);

    if (unusedError) {
        for (int i = 0; i < nodeCount; i++) {
            Xvr_freeASTNode(nodes[i]);
        }
        free(nodes);
        Xvr_freeCompiler(&compiler);
        Xvr_freeParser(&parser);
        return NULL;
    }

    for (int i = 0; i < nodeCount; i++) {
        Xvr_writeCompiler(&compiler, nodes[i]);
        Xvr_freeASTNode(nodes[i]);
    }
    free(nodes);

    const unsigned char* tb = Xvr_collateCompiler(&compiler, size);

    Xvr_freeCompiler(&compiler);
    Xvr_freeParser(&parser);
    return tb;
}

void Xvr_runBinary(const unsigned char* tb, size_t size) {
    Xvr_Interpreter interpreter;
    Xvr_initInterpreter(&interpreter);

    Xvr_injectNativeHook(&interpreter, "about", Xvr_hookAbout);
    Xvr_injectNativeHook(&interpreter, "standard", Xvr_hookStandard);
    Xvr_injectNativeHook(&interpreter, "timer", Xvr_hookTimer);
    Xvr_injectNativeHook(&interpreter, "runner", Xvr_hookRunner);

    Xvr_runInterpreter(&interpreter, tb, size);
    Xvr_freeInterpreter(&interpreter);
}

void Xvr_runBinaryFile(const char* fname) {
    size_t size = 0;
    const unsigned char* tb = (unsigned char*)Xvr_readFile(fname, &size);
    if (!tb) {
        return;
    }
    Xvr_runBinary(tb, size);
    // interpreter takes ownership of the binary data
}

void Xvr_runSource(const char* source) {
    size_t size = 0;
    const unsigned char* tb = Xvr_compileString(source, &size);
    if (!tb) {
        return;
    }

    Xvr_runBinary(tb, size);
}

void Xvr_runSourceFile(const char* fname) {
    size_t size = 0;
    const char* source = (const char*)Xvr_readFile(fname, &size);

    if (!source) {
        return;
    }

    Xvr_runSource(source);
    free((void*)source);
}
