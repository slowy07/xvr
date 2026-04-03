#include <llvm-c/Core.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "backend/xvr_llvm_codegen.h"
#include "compiler_tools.h"
#include "xvr_ast_node.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_parser.h"
#include "xvr_unused.h"

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static long get_file_size(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

static void print_error(const char* filename, int line, const char* error_type,
                        const char* message) {
    if (filename) {
        fprintf(stderr,
                XVR_CC_ERROR "%s:%d: " XVR_CC_FONT_RED "%s: " XVR_CC_RESET
                             "%s\n",
                filename, line, error_type, message);
    } else {
        fprintf(stderr, XVR_CC_ERROR "%s: " XVR_CC_RESET "%s\n", error_type,
                message);
    }
}

static void print_note(const char* filename, int line, const char* message) {
    if (filename && line > 0) {
        fprintf(stderr, XVR_CC_NOTICE "  --> " XVR_CC_RESET "%s:%d\n", filename,
                line);
        fprintf(stderr, XVR_CC_NOTICE "   |\n" XVR_CC_RESET);
    }
}

static void print_compiler_error(const char* filename, int line,
                                 const char* error_type, const char* message,
                                 const char* hint) {
    fprintf(stderr, "\n");
    fprintf(stderr, XVR_CC_FONT_RED "error" XVR_CC_RESET ": %s\n", message);
    if (filename && line > 0) {
        fprintf(stderr, "  --> %s:%d\n", filename, line);
    }
    if (hint) {
        fprintf(stderr, XVR_CC_NOTICE "help: " XVR_CC_RESET "%s\n", hint);
    }
    fprintf(stderr, "\n");
}

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

    const char* source = NULL;
    size_t size = 0;
    char module_name[256] = "inline";

    if (Xvr_commandLine.source) {
        source = Xvr_commandLine.source;
        size = strlen(source);
    } else if (Xvr_commandLine.sourceFile) {
        const char* srcExt = strrchr(Xvr_commandLine.sourceFile, '.');
        if (!srcExt || strcmp(srcExt, ".xvr")) {
            print_compiler_error(Xvr_commandLine.sourceFile, 0, "error",
                                 "input file must have .xvr extension",
                                 "Example: xvr program.xvr");
            return -1;
        }

        const char* srcBase = strrchr(Xvr_commandLine.sourceFile, '/');
        srcBase = srcBase ? srcBase + 1 : Xvr_commandLine.sourceFile;
        snprintf(module_name, sizeof(module_name), "%s", srcBase);
        char* dot = strrchr(module_name, '.');
        if (dot) *dot = '\0';

        source = (const char*)Xvr_readFile(Xvr_commandLine.sourceFile, &size);
        if (!source) {
            print_compiler_error(
                Xvr_commandLine.sourceFile, 0, "error",
                "could not read source file",
                "Check that the file exists and you have read permissions");
            return 1;
        }
    } else {
        Xvr_helpCommandLine(argc, argv);
        return 0;
    }

    double start_time = get_time_ms();

    int nodeCount = 0;
    const char* srcForError =
        Xvr_commandLine.sourceFile ? Xvr_commandLine.sourceFile : "<inline>";
    Xvr_ASTNode** nodes = parse_to_ast(source, &nodeCount);
    if (!nodes) {
        print_compiler_error(srcForError, 0, "error",
                             "parsing failed - check syntax", NULL);
        free((void*)source);
        return 1;
    }

    if (Xvr_commandLine.dumpAST) {
        fprintf(stderr,
                "\n" XVR_CC_NOTICE "AST:" XVR_CC_RESET " %d top-level nodes\n",
                nodeCount);
        for (int i = 0; i < nodeCount; i++) {
            fprintf(stderr, "  [%d] node type %d\n", i, nodes[i]->type);
        }
        fprintf(stderr, "\n");
    }

    Xvr_UnusedChecker checker;
    Xvr_initUnusedChecker(&checker);
    Xvr_checkUnusedBegin(&checker);

    for (int i = 0; i < nodeCount; i++) {
        Xvr_checkUnusedNode(&checker, nodes[i]);
    }

    if (!Xvr_checkUnusedEnd(&checker)) {
        Xvr_freeUnusedChecker(&checker);
        for (int i = 0; i < nodeCount; i++) {
            Xvr_freeASTNode(nodes[i]);
        }
        free(nodes);
        if (Xvr_commandLine.sourceFile) free((void*)source);
        return 1;
    }
    Xvr_freeUnusedChecker(&checker);

    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate(module_name);
    if (!codegen) {
        print_compiler_error(srcForError, 0, "error",
                             "failed to initialize code generator",
                             "This may indicate an out-of-memory condition");
        for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
        free(nodes);
        if (Xvr_commandLine.sourceFile) free((void*)source);
        return 1;
    }

    for (int i = 0; i < nodeCount; i++) {
        Xvr_LLVMCodegenEmitAST(codegen, nodes[i]);
    }

    if (Xvr_LLVMCodegenHasError(codegen)) {
        const char* err = Xvr_LLVMCodegenGetError(codegen);
        print_compiler_error(
            srcForError, 0, "error", err ? err : "unknown compilation error",
            "Check your code for type errors or unsupported features");
        Xvr_LLVMCodegenDestroy(codegen);
        for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
        free(nodes);
        if (Xvr_commandLine.sourceFile) free((void*)source);
        return 1;
    }

    /* FIXME: Optimizer causes SIGSEGV - re-enable once target machine
     * configuration is fixed in xvr_llvm_optimizer.c
     *
     * TODO: Add command-line flag for optimization level (-O0, -O1, -O2, -O3)
     *
    if (!Xvr_commandLine.dumpLLVM) {
        Xvr_LLVMCodegenRunOptimizer(codegen);
    }
    */

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
            print_compiler_error(
                srcForError, 0, "error", "failed to write object file",
                "Check write permissions in the output directory");
            Xvr_LLVMCodegenDestroy(codegen);
            for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
            free(nodes);
            free(outFile);
            if (Xvr_commandLine.sourceFile) free((void*)source);
            return 1;
        }
    }

    Xvr_LLVMCodegenDestroy(codegen);
    for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
    free(nodes);
    if (Xvr_commandLine.sourceFile) free((void*)source);

    double total_time = get_time_ms() - start_time;

    if (shouldRun) {
        const char* runtime_src = "/tmp/xvr_runtime.c";
        const char* build_dir = getenv("XVR_BUILD_DIR");
        if (!build_dir) build_dir = "build";
        char* libxvr_path;
        asprintf(&libxvr_path, "%s/src/libxvr.a", build_dir);

        FILE* rf = fopen(libxvr_path, "r");
        int has_libxvr = (rf != NULL);
        if (rf) fclose(rf);

        const char* runtime_code;
        if (has_libxvr) {
            runtime_code = "";
        } else {
            runtime_code =
                "#include <stdio.h>\n"
                "#include <stdarg.h>\n"
                "#include <stdlib.h>\n"
                "#include <string.h>\n"
                "int printf(const char *fmt, ...) {\n"
                "    va_list args;\n"
                "    va_start(args, fmt);\n"
                "    int result = vprintf(fmt, args);\n"
                "    va_end(args);\n"
                "    return result;\n"
                "}\n"
                "char* xvr_string_concat(const char* lhs, const char* rhs) {\n"
                "    if (!lhs) lhs = \"\";\n"
                "    if (!rhs) rhs = \"\";\n"
                "    size_t lhs_len = strlen(lhs);\n"
                "    size_t rhs_len = strlen(rhs);\n"
                "    size_t total_len = lhs_len + rhs_len;\n"
                "    char* result = (char*)malloc(total_len + 1);\n"
                "    if (!result) return NULL;\n"
                "    memcpy(result, lhs, lhs_len);\n"
                "    memcpy(result + lhs_len, rhs, rhs_len);\n"
                "    result[total_len] = '\\0';\n"
                "    return result;\n"
                "}\n"
                "int xvr_str_len(const char* str) {\n"
                "    if (!str) return 0;\n"
                "    return (int)strlen(str);\n"
                "}\n";
        }

        if (runtime_code && strlen(runtime_code) > 0) {
            FILE* rf2 = fopen(runtime_src, "w");
            if (rf2) {
                fputs(runtime_code, rf2);
                fclose(rf2);
            }

            char* cmd2;
            asprintf(&cmd2, "gcc -c %s -o /tmp/xvr_runtime.o 2>/dev/null",
                     runtime_src);
            system(cmd2);
            free(cmd2);
        }

        char* cmd;
        if (has_libxvr) {
            asprintf(&cmd,
                     "gcc %s -o /tmp/xvr_bin -lm -lpthread "
                     "-lxml2 -lcurl %s 2>&1",
                     outFile, libxvr_path);
        } else {
            asprintf(&cmd,
                     "gcc %s -o /tmp/xvr_bin -lm -lpthread "
                     "-lxml2 -lcurl 2>&1",
                     outFile);
        }
        int rc = system(cmd);
        free(libxvr_path);

        long bin_size = get_file_size("/tmp/xvr_bin");

        if (Xvr_commandLine.showTiming) {
            const char* target = LLVMGetDefaultTargetTriple();
            printf("\n");
            printf("  " XVR_CC_NOTICE "Target:" XVR_CC_RESET " %s\n", target);
            printf("  " XVR_CC_NOTICE "Size:" XVR_CC_RESET " %.1f KB\n",
                   bin_size / 1024.0);
            printf("  " XVR_CC_NOTICE "Time:" XVR_CC_RESET " %.2f ms\n",
                   total_time);
            printf("\n");
            LLVMDisposeMessage((char*)target);
        }

        if (rc == 0) {
            system("/tmp/xvr_bin");
        }

        unlink(runtime_src);
        unlink("/tmp/xvr_runtime.o");
        unlink(outFile);
        unlink("/tmp/xvr_bin");
    } else {
        long obj_size = get_file_size(outFile);
        if (Xvr_commandLine.showTiming) {
            const char* target = LLVMGetDefaultTargetTriple();
            printf("\n");
            printf("  " XVR_CC_NOTICE "Target:" XVR_CC_RESET " %s\n", target);
            printf("  " XVR_CC_NOTICE "Size:" XVR_CC_RESET " %.1f KB\n",
                   obj_size / 1024.0);
            printf("  " XVR_CC_NOTICE "Time:" XVR_CC_RESET " %.2f ms\n",
                   total_time);
            printf("\n");
            LLVMDisposeMessage((char*)target);
        }
    }

    free(outFile);
    return 0;
}
