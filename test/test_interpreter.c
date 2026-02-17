#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter_tools.h"
#include "xvr_compiler.h"
#include "xvr_console_colors.h"
#include "xvr_interpreter.h"
#include "xvr_lexer.h"
#include "xvr_memory.h"
#include "xvr_parser.h"

static void noPrintFn(const char* output) {}

int ignoredAssertions = 0;

static void noAssertFn(const char* output) {
    if (strncmp(output, "!ignore", 7) == 0) {
        ignoredAssertions++;
    } else {
        fprintf(stderr, XVR_CC_ERROR "Assertion failure: ");
        fprintf(stderr, "%s", output);
        fprintf(stderr, "\n" XVR_CC_RESET);
    }
}

void runBinaryCustom(const unsigned char* tb, size_t size) {
    Xvr_Interpreter interpreter;
    Xvr_initInterpreter(&interpreter);

    Xvr_setInterpreterPrint(&interpreter, noPrintFn);
    Xvr_setInterpreterAssert(&interpreter, noAssertFn);

    Xvr_runInterpreter(&interpreter, tb, size);
    Xvr_freeInterpreter(&interpreter);
}

void runSourceCustom(const char* source) {
    size_t size = 0;
    const unsigned char* tb = Xvr_compileString(source, &size);
    if (!tb) {
        return;
    }
    runBinaryCustom(tb, size);
}

void runSourceFileCustom(const char* fname) {
    size_t size = 0;  // not used
    const char* source = (const char*)Xvr_readFile(fname, &size);
    runSourceCustom(source);
    free((void*)source);
}

int main(void) {
    {
        Xvr_Interpreter interpreter;
        Xvr_initInterpreter(&interpreter);
        Xvr_freeInterpreter(&interpreter);
    }

    {
        const char* source = "print(null);";
        Xvr_Lexer lexer;
        Xvr_Parser parser;
        Xvr_Compiler compiler;
        Xvr_Interpreter interpreter;

        Xvr_initLexer(&lexer, source);
        Xvr_initParser(&parser, &lexer);
        Xvr_initCompiler(&compiler);
        Xvr_initInterpreter(&interpreter);

        Xvr_ASTNode* node = Xvr_scanParser(&parser);

        Xvr_writeCompiler(&compiler, node);

        size_t size = 0;
        const unsigned char* bytecode = Xvr_collateCompiler(&compiler, &size);
        Xvr_setInterpreterPrint(&interpreter, noPrintFn);
        Xvr_setInterpreterAssert(&interpreter, noAssertFn);
        Xvr_runInterpreter(&interpreter, bytecode, size);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
        Xvr_freeCompiler(&compiler);
        Xvr_freeInterpreter(&interpreter);
    }

    if (ignoredAssertions > 1) {
        fprintf(stderr, XVR_CC_ERROR "Woilah cik, assert: %d\n" XVR_CC_RESET,
                ignoredAssertions);
        return -1;
    }

    printf(XVR_CC_NOTICE "INTERPRETER: woilah cik aman loh ya\n" XVR_CC_RESET);
    return 0;
}
