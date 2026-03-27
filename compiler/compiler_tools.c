/**
 * MIT License
 * Copyright (c) 2025 arfy slowy
 *
 * Compiler Tools - Simplified for LLVM AOT only
 */

#include "compiler_tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#    include <windows.h>
#else
#    include <unistd.h>
#endif

#include "xvr_ast_node.h"
#include "xvr_lexer.h"
#include "xvr_parser.h"

#ifdef XVR_EXPORT_LLVM
#    include "backend/xvr_llvm_codegen.h"
#endif

const unsigned char* Xvr_readFile(const char* path, size_t* fileSize) {
    FILE* file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\"\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    *fileSize = ftell(file);
    rewind(file);

    unsigned char* buffer = (unsigned char*)malloc(*fileSize + 1);

    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\"\n", path);
        return NULL;
    }

    size_t bytesRead = fread(buffer, sizeof(unsigned char), *fileSize, file);

    buffer[*fileSize] = '\0';

    if (bytesRead < *fileSize) {
        fprintf(stderr, "Could not read file \"%s\"\n", path);
        return NULL;
    }

    fclose(file);

    return buffer;
}

#if defined(_WIN32) || defined(_WIN64)
char* Xvr_getTempDir(void) {
    char* tmpdir = (char*)malloc(MAX_PATH);
    if (!tmpdir) return NULL;

    DWORD ret = GetTempPathA(MAX_PATH, tmpdir);
    if (ret == 0 || ret >= MAX_PATH) {
        free(tmpdir);
        tmpdir = (char*)malloc(4);
        if (tmpdir) strcpy(tmpdir, ".");
        return tmpdir;
    }

    return tmpdir;
}

int Xvr_unlink(const char* path) { return DeleteFileA(path) ? 0 : -1; }
#else
char* Xvr_getTempDir(void) {
    const char* tmpdir = "/tmp";
    char* result = (char*)malloc(strlen(tmpdir) + 1);
    if (result) strcpy(result, tmpdir);
    return result;
}

int Xvr_unlink(const char* path) { return unlink(path); }
#endif

#ifdef XVR_EXPORT_LLVM
Xvr_ASTNode** parse_to_ast(const char* source, int* out_count) {
    Xvr_Lexer lexer;
    Xvr_Parser parser;

    Xvr_initLexer(&lexer, source);
    Xvr_initParser(&parser, &lexer);

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
            Xvr_freeParser(&parser);
            *out_count = 0;
            return NULL;
        }

        if (nodeCount >= nodeCapacity) {
            nodeCapacity = nodeCapacity < 8 ? 8 : nodeCapacity * 2;
            nodes = realloc(nodes, sizeof(Xvr_ASTNode*) * nodeCapacity);
        }
        nodes[nodeCount++] = node;
        node = Xvr_scanParser(&parser);
    }

    Xvr_freeParser(&parser);
    *out_count = nodeCount;
    return nodes;
}
#endif
