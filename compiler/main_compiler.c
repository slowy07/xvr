#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "backend/xvr_llvm_codegen.h"
#include "compiler_tools.h"
#include "xvr_ast_node.h"
#include "xvr_common.h"
#include "xvr_compiler.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_literal.h"
#include "xvr_parser.h"
#include "xvr_refstring.h"

int main(int argc, const char* argv[]) {
    Xvr_initCommandLine(argc, argv);

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
               "Xvr Programming Language Version %d.%d.%d, built "
               "'%s'\n" XVR_CC_RESET,
               XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
               XVR_VERSION_BUILD);
    }

    if (!Xvr_commandLine.sourceFile) {
        Xvr_helpCommandLine(argc, argv);
        return 0;
    }

    const char* src = strrchr(Xvr_commandLine.sourceFile, '.');
    if (!src || strcmp(src, ".xvr")) {
        fprintf(stderr, XVR_CC_ERROR "Input must be .xvr file\n" XVR_CC_RESET);
        return -1;
    }

    size_t size = 0;
    const char* source =
        (const char*)Xvr_readFile(Xvr_commandLine.sourceFile, &size);
    if (!source) {
        fprintf(stderr, XVR_CC_ERROR "Could not open file\n" XVR_CC_RESET);
        return 1;
    }

    int nodeCount = 0;
    Xvr_ASTNode** nodes = parse_to_ast(source, &nodeCount);
    if (!nodes) {
        fprintf(stderr, XVR_CC_ERROR "Failed to parse source\n" XVR_CC_RESET);
        return 1;
    }

    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("xvr_module");
    if (!codegen) {
        fprintf(stderr, XVR_CC_ERROR "Failed to create codegen\n" XVR_CC_RESET);
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

    bool shouldRun = !Xvr_commandLine.compileOnly && !Xvr_commandLine.dumpLLVM;
    char* outFile = NULL;

    if (shouldRun) {
        outFile = strdup("/tmp/xvr_compile.o");
    } else {
        outFile = Xvr_commandLine.outFile ? strdup(Xvr_commandLine.outFile)
                                          : strdup("a.out");
    }

    if (Xvr_commandLine.dumpLLVM) {
        size_t ir_len = 0;
        char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
        if (ir) {
            printf("%s\n", ir);
            free(ir);
        }
    } else {
        if (!Xvr_LLVMCodegenWriteObjectFile(codegen, outFile)) {
            fprintf(stderr,
                    XVR_CC_ERROR "Failed to write object file\n" XVR_CC_RESET);
            Xvr_LLVMCodegenDestroy(codegen);
            for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
            free(nodes);
            free(outFile);
            return 1;
        }
        printf("Compiled to: %s\n", outFile);
    }

    Xvr_LLVMCodegenDestroy(codegen);
    for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
    free(nodes);

    if (shouldRun) {
        const char* runtime_src = "/tmp/xvr_runtime.c";
        const char* runtime_code =
            "#include <stdio.h>\n"
            "#include <stdarg.h>\n"
            "int printf(const char *fmt, ...) {\n"
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

        char* cmd;
        asprintf(&cmd, "gcc -c %s -o /tmp/xvr_runtime.o", runtime_src);
        system(cmd);
        free(cmd);

        asprintf(&cmd, "gcc %s /tmp/xvr_runtime.o -o /tmp/xvr_bin", outFile);
        int rc = system(cmd);
        free(cmd);

        if (rc == 0) {
            system("/tmp/xvr_bin");
        }

        unlink(runtime_src);
        unlink("/tmp/xvr_runtime.o");
        unlink(outFile);
        unlink("/tmp/xvr_bin");
    }

    free(outFile);
    return 0;
}
