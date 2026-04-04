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
        char* libxvr_path = NULL;
        int has_libxvr = 0;

        char exe_path[1024] = {0};
        ssize_t len =
            readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len > 0) {
            exe_path[len] = '\0';
            char* lastSlash = strrchr(exe_path, '/');
            if (lastSlash) {
                *lastSlash = '\0';
            }
        }

        char search_paths[16][1024];
        int path_count = 0;

        const char* env_build = getenv("XVR_BUILD_DIR");
        if (env_build) {
            snprintf(search_paths[path_count++], sizeof(search_paths[0]), "%s",
                     env_build);
        }

        if (exe_path[0]) {
            snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                     "%s/build", exe_path);
            char parent[1024];
            char* p = strrchr(exe_path, '/');
            if (p) {
                *p = '\0';
                snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                         "%s/build", exe_path);
            }
        }

        snprintf(search_paths[path_count++], sizeof(search_paths[0]), "build");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                 "../build");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                 "./build");
        if (exe_path[0]) {
            snprintf(search_paths[path_count++], sizeof(search_paths[0]), ".");
        }

        for (int i = 0; i < path_count; i++) {
            char test_path[1024];
            snprintf(test_path, sizeof(test_path), "%s/src/libxvr.a",
                     search_paths[i]);
            FILE* rf = fopen(test_path, "r");
            if (rf) {
                fclose(rf);
                libxvr_path = strdup(test_path);
                has_libxvr = 1;
                break;
            }
        }

        char* cmd;
        if (has_libxvr && libxvr_path) {
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
        if (libxvr_path) free(libxvr_path);

        long bin_size = get_file_size("/tmp/xvr_bin");
        if (rc != 0) {
            fprintf(stderr, "gcc failed: rc=%d\n", rc);
        } else if (bin_size <= 0) {
            fprintf(stderr, "binary not created\n");
            rc = 1;
        }

        if (rc == 0) {
            system("/tmp/xvr_bin");
        }

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
