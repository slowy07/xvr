#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "backend/xvr_llvm_codegen.h"
#include "compiler_tools.h"
#include "optimizer/xvr_ast_optimizer.h"
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

    Xvr_ASTOptimizer* ast_opt = Xvr_ASTOptimizerCreate();
    if (ast_opt) {
        int opt_level = Xvr_commandLine.optimizationLevel;
        Xvr_OptimizationLevel xvr_opt_level =
            Xvr_OptimizationLevelFromInt(opt_level);
        Xvr_ASTOptimizerSetLevel(ast_opt, xvr_opt_level);

        if (opt_level > 0) {
            Xvr_ASTOptimizerAddStandardPasses(ast_opt);
            Xvr_ASTOptimizerResult result =
                Xvr_ASTOptimizerRun(ast_opt, nodes, nodeCount);
            if (Xvr_commandLine.verbose && result.changes_made > 0) {
                fprintf(stderr,
                        XVR_CC_NOTICE
                        "AST optimization: %d changes\n" XVR_CC_RESET,
                        result.changes_made);
            }
        }
        Xvr_ASTOptimizerDestroy(ast_opt);
    }

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

    int opt_level = Xvr_commandLine.optimizationLevel;
    if (opt_level > 0 && !Xvr_commandLine.dumpLLVM) {
        Xvr_LLVMOptimizationLevel llvm_level = XVR_LLVM_OPT_O2;
        switch (opt_level) {
        case 1:
            llvm_level = XVR_LLVM_OPT_O1;
            break;
        case 2:
            llvm_level = XVR_LLVM_OPT_O2;
            break;
        case 3:
            llvm_level = XVR_LLVM_OPT_O3;
            break;
        default:
            llvm_level = XVR_LLVM_OPT_O2;
            break;
        }
        Xvr_LLVMCodegenSetOptimizationLevel(codegen, llvm_level);
        if (!Xvr_LLVMCodegenRunOptimizer(codegen)) {
            if (Xvr_commandLine.verbose) {
                fprintf(stderr, XVR_CC_NOTICE
                        "LLVM optimization warning: optimization pass failed, "
                        "continuing\n" XVR_CC_RESET);
            }
        }
    }

    bool shouldRun = !Xvr_commandLine.compileOnly &&
                     !Xvr_commandLine.dumpLLVM &&
                     (Xvr_commandLine.emitType == NULL);
    char* outFile = NULL;
    char* objFile = NULL;

    bool useEmitType = Xvr_commandLine.emitType != NULL;
    int emitFileType = 0;
    if (useEmitType) {
        if (strcmp(Xvr_commandLine.emitType, "asm") == 0) {
            emitFileType = 1;
        } else if (strcmp(Xvr_commandLine.emitType, "llvm-ir") == 0) {
            emitFileType = 2;
        } else {
            emitFileType = 0;
        }
    }

    if (shouldRun) {
        objFile = strdup("/tmp/xvr_compile.o");
        if (Xvr_commandLine.outFile) {
            outFile = strdup(Xvr_commandLine.outFile);
        }
    } else {
        outFile = Xvr_commandLine.outFile ? strdup(Xvr_commandLine.outFile)
                                          : strdup("a.out");
        objFile = strdup(outFile);
    }

    if (Xvr_commandLine.dumpLLVM || (useEmitType && emitFileType == 2)) {
        size_t ir_len = 0;
        char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
        if (ir) {
            if (useEmitType && emitFileType == 2 && Xvr_commandLine.outFile) {
                FILE* f = fopen(Xvr_commandLine.outFile, "w");
                if (f) {
                    fputs(ir, f);
                    fclose(f);
                } else {
                    free(ir);
                    print_compiler_error(srcForError, 0, "error",
                                         "failed to write LLVM IR file", NULL);
                    free(outFile);
                    free(objFile);
                    return 1;
                }
            } else {
                printf("%s\n", ir);
            }
            free(ir);
        }
    } else {
        if (!Xvr_LLVMCodegenWriteObjectFile(codegen, objFile, emitFileType)) {
            print_compiler_error(
                srcForError, 0, "error", "failed to write output file",
                "Check write permissions in the output directory");
            Xvr_LLVMCodegenDestroy(codegen);
            for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
            free(nodes);
            free(outFile);
            free(objFile);
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

        char search_paths[24][4096];
        int path_count = 0;

        const char* env_build = getenv("XVR_BUILD_DIR");
        if (env_build) {
            snprintf(search_paths[path_count++], sizeof(search_paths[0]), "%s",
                     env_build);
        }

        const char* home = getenv("HOME");
        if (home) {
            char path[4096];
            snprintf(path, sizeof(path),
                     "%s/Documents/project/xvrlang/xvr/build", home);
            snprintf(search_paths[path_count++], sizeof(search_paths[0]), "%s",
                     path);
            snprintf(path, sizeof(path), "%s/project/xvrlang/xvr/build", home);
            snprintf(search_paths[path_count++], sizeof(search_paths[0]), "%s",
                     path);
        }

        snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                 "/home/arfyslowy/Documents/project/xvrlang/xvr/build");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                 "/home/arfyslowy/project/xvrlang/xvr/build");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]), "build");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                 "../build");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                 "./build");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                 "../../build");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]), ".");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]), "..");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]), "../..");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                 "/usr/local/xvr/build");
        snprintf(search_paths[path_count++], sizeof(search_paths[0]),
                 "/opt/xvr/build");

        for (int i = 0; i < path_count; i++) {
            char test_path[8192];
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
        char* final_exe =
            Xvr_commandLine.outFile ? Xvr_commandLine.outFile : "/tmp/xvr_bin";

        if (has_libxvr && libxvr_path) {
            asprintf(&cmd, "gcc %s -o %s -lm -lpthread -lxml2 -lcurl %s",
                     objFile, final_exe, libxvr_path);
        } else {
            asprintf(&cmd, "gcc %s -o %s -lm -lpthread -lxml2 -lcurl", objFile,
                     final_exe);
        }

        int rc = system(cmd);
        if (libxvr_path) free(libxvr_path);
        free(cmd);

        if (rc != 0) {
            fprintf(stderr, "error: failed to link executable\n");
        }

        long bin_size = get_file_size(final_exe);

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
            system(final_exe);
        }

        unlink(objFile);
        if (!Xvr_commandLine.outFile) {
            unlink("/tmp/xvr_bin");
        }
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
    free(objFile);
    return 0;
}
