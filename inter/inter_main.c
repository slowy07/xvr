#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter_tools.h"
#include "lib_about.h"
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
    // repl does it's own thing for now
    bool error = false;

    const int size = 2048;
    char input[size];
    memset(input, 0, size);

    Xvr_Interpreter interpreter;  // persist the interpreter for the scopes
    Xvr_initInterpreter(&interpreter);

    // inject the libs
    Xvr_injectNativeHook(&interpreter, "about", Xvr_hookAbout);
    Xvr_injectNativeHook(&interpreter, "standard", Xvr_hookStandard);
    Xvr_injectNativeHook(&interpreter, "timer", Xvr_hookTimer);
    Xvr_injectNativeHook(&interpreter, "runner", Xvr_hookRunner);

    for (;;) {
        printf("> ");

        // handle EOF for exits
        if (!fgets(input, size, stdin)) {
            break;
        }

        // escape the repl (length of 5 to accomodate the newline)
        if (strlen(input) == 5 &&
            (!strncmp(input, "exit", 4) || !strncmp(input, "quit", 4))) {
            break;
        }

        // setup this iteration
        Xvr_Lexer lexer;
        Xvr_Parser parser;
        Xvr_Compiler compiler;

        Xvr_initLexer(&lexer, input);
        Xvr_initParser(&parser, &lexer);
        Xvr_initCompiler(&compiler);

        // run this iteration
        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        while (node != NULL) {
            // pack up and restart
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
            // get the bytecode dump
            int size = 0;
            unsigned char* tb = Xvr_collateCompiler(&compiler, &size);

            // run the bytecode
            Xvr_runInterpreter(&interpreter, tb, size);
        }

        // clean up this iteration
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

    // command line specific actions
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

    // version
    if (Xvr_commandLine.verbose) {
        printf(XVR_CC_NOTICE
               "Xvr Programming Language Version %d.%d.%d, built "
               "'%s'\n" XVR_CC_RESET,
               XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
               XVR_VERSION_BUILD);
    }

    // run source file
    if (Xvr_commandLine.sourceFile) {
        Xvr_runSourceFile(Xvr_commandLine.sourceFile);

        // lib cleanup
        Xvr_freeDriveDictionary();

        return 0;
    }

    // run from stdin
    if (Xvr_commandLine.source) {
        const char* s = strrchr(Xvr_commandLine.sourceFile, '.');
        if (!s || strcmp(s, ".xvr")) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "bad file extension passing to %s (expected `.xvr`, got "
                    "%s)" XVR_CC_ERROR,
                    argv[0], s);
            return -1;
        }

        Xvr_runSource(Xvr_commandLine.source);

        // lib cleanup
        Xvr_freeDriveDictionary();

        return 0;
    }

    // compile source file
    if (Xvr_commandLine.compileFile && Xvr_commandLine.outFile) {
        const char* c = strrchr(Xvr_commandLine.compileFile, '.');
        if (!c || strcmp(c, ".xvr")) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "bad file extension passing to %s (expecting `.xvr` got "
                    "%s)" XVR_CC_RESET,
                    argv[0], c);

            return -1;
        }

        const char* o = strrchr(Xvr_commandLine.outFile, '.');
        if (!o || strcmp(o, ".xb")) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "bad file extension passing to %s (expted  '.xb') got "
                    "%s" XVR_CC_RESET,
                    argv[0], o);
            return -1;
        }

        size_t size = 0;
        const char* source = Xvr_readFile(Xvr_commandLine.compileFile, &size);

        if (!source) {
            return 1;
        }

        const unsigned char* tb = Xvr_compileString(source, &size);
        if (!tb) {
            return 1;
        }
        Xvr_writeFile(Xvr_commandLine.outFile, tb, size);
        return 0;
    }

    if (Xvr_commandLine.binaryFile) {
        const char* c = strrchr(Xvr_commandLine.binaryFile, '.');
        if (!c || strcmp(c, ".xb")) {
            fprintf(
                stderr,
                XVR_CC_ERROR
                "bad file extension passing to %s (expected are `.xb` got %s)",
                argv[0], c);

            return -1;
        }

        Xvr_runBinaryFile(Xvr_commandLine.binaryFile);

        Xvr_freeDriveDictionary();

        return 0;
    }

    inter();

    Xvr_freeDriveDictionary();

    return 0;
}
