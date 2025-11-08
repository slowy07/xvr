#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter_tools.h"
#include "lib_about.h"
#include "lib_compound.h"
#include "lib_runner.h"
#include "lib_standard.h"
#include "lib_timer.h"
#include "xvr_ast_node.h"
#include "xvr_common.h"
#include "xvr_compiler.h"
#include "xvr_console_colors.h"
#include "xvr_interpreter.h"
#include "xvr_lexer.h"
#include "xvr_literal.h"
#include "xvr_literal_dictionary.h"
#include "xvr_parser.h"
#include "xvr_refstring.h"

void inter(void) {
    bool error = false;
    const int size = 2048;
    char input[size];
    memset(input, 0, size);

    Xvr_Interpreter interpreter;
    Xvr_initInterpreter(&interpreter);

    Xvr_injectNativeHook(&interpreter, "about", Xvr_hookAbout);
    Xvr_injectNativeHook(&interpreter, "compound", Xvr_hookCompound);
    Xvr_injectNativeHook(&interpreter, "standard", Xvr_hookStandard);
    Xvr_injectNativeHook(&interpreter, "timer", Xvr_hookTimer);
    Xvr_injectNativeHook(&interpreter, "runner", Xvr_hookRunner);

    for (;;) {
        printf(">> ");

        if (!fgets(input, size, stdin)) {
            break;
        }

        if (strlen(input) == 5 &&
            (!strncmp(input, "exit", 4) || !strncmp(input, "quit", 4))) {
            break;
        }

        Xvr_Lexer lexer;
        Xvr_Parser parser;
        Xvr_Compiler compiler;

        Xvr_initLexer(&lexer, input);
        Xvr_initParser(&parser, &lexer);
        Xvr_initCompiler(&compiler);

        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        while (node != NULL) {
            if (node->type == XVR_AST_NODE_ERROR) {
                if (Xvr_commandLine.verbose) {
                    printf(XVR_CC_ERROR "Error node detected\n" XVR_CC_RESET);
                }
                error = true;
                Xvr_freeASTNode(node);
                break;
            }

            Xvr_writeCompiler(&compiler, node);
            Xvr_freeASTNode(node);
            node = Xvr_scanParser(&parser);
        }

        if (!error) {
            int size = 0;
            unsigned char* tb = Xvr_collateCompiler(&compiler, &size);
            Xvr_runInterpreter(&interpreter, tb, size);
        }

        Xvr_freeCompiler(&compiler);
        Xvr_freeParser(&parser);
        error = false;
    }

    Xvr_freeInterpreter(&interpreter);
}

int main(int argc, const char* argv[]) {
    Xvr_initCommandLine(argc, argv);

    Xvr_initDriveDictionary();
    Xvr_Literal driveLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefString("scripts"));
    Xvr_Literal pathLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefString("scripts"));

    Xvr_setLiteralDictionary(Xvr_getDriveDictionary(), driveLiteral,
                             pathLiteral);

    Xvr_freeLiteral(driveLiteral);
    Xvr_freeLiteral(pathLiteral);

    if (Xvr_commandLine.error) {
        Xvr_usageCommandLine(argc, argv);
        return 0;
    }

    if (Xvr_commandLine.help) {
        Xvr_helpCommandLine(argc, argv);
        return 0;
    }

    if (Xvr_commandLine.version) {
        Xvr_copyrightCommandLine(argc, argv);
        return 0;
    }

    if (Xvr_commandLine.verbose) {
        printf(XVR_CC_NOTICE
               "The XVR Programming Language Version %d.%d.%d, built data "
               "'%s'\n" XVR_CC_RESET,
               XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
               XVR_VERSION_BUILD);
    }

    if (Xvr_commandLine.sourceFile) {
        Xvr_runSourceFile(Xvr_commandLine.sourceFile);
        Xvr_freeDriveDictionary();
        return 0;
    }

    if (Xvr_commandLine.source) {
        Xvr_runSource(Xvr_commandLine.source);
        Xvr_freeDriveDictionary();
        return 0;
    }

    if (Xvr_commandLine.compileFile && Xvr_commandLine.outFile) {
        size_t size = 0;
        char* source = Xvr_readFile(Xvr_commandLine.compileFile, &size);
        unsigned char* tb = Xvr_compileString(source, &size);
        if (!tb) {
            return 1;
        }
        Xvr_writeFile(Xvr_commandLine.outFile, tb, size);
        return 0;
    }

    if (Xvr_commandLine.binaryFile) {
        Xvr_runBinaryFile(Xvr_commandLine.binaryFile);
        Xvr_freeDriveDictionary();
        return 0;
    }

    inter();
    Xvr_freeDriveDictionary();
    return 0;
}
