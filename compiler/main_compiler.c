#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "compiler_tools.h"
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

#ifdef XVR_EXPORT_LLVM
#    include "backend/xvr_llvm_codegen.h"
#endif

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
        fflush(stdout);

        // handle EOF for exits
        if (!Xvr_readLine(input, size)) {
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
            size_t size = 0;
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

#ifdef XVR_EXPORT_LLVM
    // LLVM compile mode - always use compiler
    // -c: compile only
    // (default): compile and run
    // -S: dump LLVM IR
    if (Xvr_commandLine.sourceFile) {
        const char* src = strrchr(Xvr_commandLine.sourceFile, '.');
        if (!src || strcmp(src, ".xvr")) {
            fprintf(stderr,
                    XVR_CC_ERROR "Input must be .xvr file\n" XVR_CC_RESET);
            return -1;
        }

        size_t size = 0;
        const char* source =
            (const char*)Xvr_readFile(Xvr_commandLine.sourceFile, &size);
        if (!source) {
            return 1;
        }

        int nodeCount = 0;
        Xvr_ASTNode** nodes = parse_to_ast(source, &nodeCount);
        if (!nodes) {
            fprintf(stderr,
                    XVR_CC_ERROR "Failed to parse source\n" XVR_CC_RESET);
            return 1;
        }

        Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("xvr_module");
        if (!codegen) {
            fprintf(stderr,
                    XVR_CC_ERROR "Failed to create codegen\n" XVR_CC_RESET);
            for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
            free(nodes);
            return 1;
        }

        for (int i = 0; i < nodeCount; i++) {
            Xvr_LLVMCodegenEmitAST(codegen, nodes[i]);
        }

        if (Xvr_LLVMCodegenHasError(codegen)) {
            fprintf(stderr, XVR_CC_ERROR "%s\n" XVR_CC_RESET,
                    Xvr_LLVMCodegenGetError(codegen));
            Xvr_LLVMCodegenDestroy(codegen);
            for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
            free(nodes);
            return 1;
        }

        // compileOnly = false means compile and run
        // dumpLLVM means don't run, just show IR
        bool shouldRun =
            !Xvr_commandLine.compileOnly && !Xvr_commandLine.dumpLLVM;

        char* outFile = NULL;

        if (shouldRun) {
            outFile = strdup("/tmp/xvr_compile.o");
        } else {
            outFile = Xvr_commandLine.outFile ? Xvr_commandLine.outFile : "a.o";
        }

        const char* ext = strrchr(outFile, '.');

        if (Xvr_commandLine.dumpLLVM ||
            (!shouldRun && ext && strcmp(ext, ".o") != 0)) {
            size_t ir_len = 0;
            char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
            if (ir) {
                printf("%s\n", ir);
                free(ir);
            }
        } else {
            if (!Xvr_LLVMCodegenWriteObjectFile(codegen, outFile)) {
                fprintf(stderr, XVR_CC_ERROR
                        "Failed to write object file\n" XVR_CC_RESET);
                Xvr_LLVMCodegenDestroy(codegen);
                for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
                free(nodes);
                if (shouldRun) free(outFile);
                return 1;
            }
            if (!shouldRun) {
                printf("Compiled to: %s\n", outFile);
            }
        }

        Xvr_LLVMCodegenDestroy(codegen);
        for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
        free(nodes);

        if (shouldRun) {
            // Use fixed temp filenames
            const char* runtime_src = "/tmp/xvr_runtime.c";
            const char* out_bin = "/tmp/xvr_bin";

            // Create and write runtime source
            const char* runtime_code =
                "#include <stdarg.h>\n"
                "#include <stdio.h>\n"
                "int print(const char* fmt, ...) {\n"
                "    va_list args;\n"
                "    va_start(args, fmt);\n"
                "    int result = vprintf(fmt, args);\n"
                "    va_end(args);\n"
                "    return result;\n"
                "}\n"
                "int printf(const char* fmt, ...) {\n"
                "    va_list args;\n"
                "    va_start(args, fmt);\n"
                "    int result = vprintf(fmt, args);\n"
                "    va_end(args);\n"
                "    return result;\n"
                "}\n";

            FILE* rf = fopen(runtime_src, "w");
            if (rf) {
                fputs(runtime_code, rf);
                fclose(rf);
            }

            // Compile runtime
            char* cmd;
            asprintf(&cmd, "gcc -c %s -o /tmp/xvr_runtime.o", runtime_src);
            system(cmd);
            free(cmd);

            // Compile and link
            asprintf(&cmd, "gcc %s /tmp/xvr_runtime.o -o %s", outFile, out_bin);
            int rc = system(cmd);
            free(cmd);

            if (rc == 0) {
                system(out_bin);
            }

            // Cleanup
            unlink(runtime_src);
            unlink("/tmp/xvr_runtime.o");
            unlink(outFile);
            unlink(out_bin);

            return 0;
        }

        if (shouldRun) free(outFile);
        return 0;
    }
#else
    // Fallback to interpreter if LLVM not available
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
#endif

    // No source file, show help
    Xvr_helpCommandLine(argc, argv);

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
