#include <stdio.h>
#include <string.h>

#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_module_bundle.h"
#include "xvr_opcodes.h"
#include "xvr_parser.h"

int test_bundle_header(Xvr_Bucket** bucketHandle) {
    {
        Xvr_Ast* ast = NULL;
        Xvr_private_emitAstPass(bucketHandle, &ast);

        Xvr_ModuleBundle bundle;
        Xvr_initModuleBundle(&bundle);
        Xvr_appendModuleBundle(&bundle, ast);

        if (bundle.count % 4 != 0) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: module bundle size is not a multiple of 4, size "
                    "is: %d\n" XVR_CC_RESET,
                    (int)bundle.count);

            Xvr_freeModuleBundle(&bundle);
            return -1;
        }
    }

    return 0;
}

int test_bundle_from_source(Xvr_Bucket** bucketHandle) {
    {
        // setup
        const char* source = "(1 + 2) * (3 + 4);";
        Xvr_Lexer lexer;
        Xvr_Parser parser;

        Xvr_bindLexer(&lexer, source);
        Xvr_bindParser(&parser, &lexer);
        Xvr_Ast* ast = Xvr_scanParser(bucketHandle, &parser);

        // run
        Xvr_ModuleBundle bundle;
        Xvr_initModuleBundle(&bundle);
        Xvr_appendModuleBundle(&bundle, ast);

        // check bytecode alignment
        if (bundle.count % 4 != 0) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: module bundle size is not a multiple of 4 (size is "
                    "%d), source: %s\n" XVR_CC_RESET,
                    (int)bundle.count, source);

            // cleanup and return
            Xvr_freeModuleBundle(&bundle);
            return -1;
        }

        // check bytecode header
        // check
        if (bundle.ptr[0] != XVR_VERSION_MAJOR ||
            bundle.ptr[1] != XVR_VERSION_MINOR ||
            bundle.ptr[2] != XVR_VERSION_PATCH || bundle.ptr[3] != 1 ||
            strcmp((char*)(bundle.ptr + 4), XVR_VERSION_BUILD) != 0) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: failed to write the module bundle header, source: "
                    "%s\n" XVR_CC_RESET,
                    source);

            Xvr_freeModuleBundle(&bundle);
            return -1;
        }

        Xvr_freeModuleBundle(&bundle);
    }

    return 0;
}

int main(void) {
    printf(XVR_CC_WARN "TESTING: XVR MODULE BUNDLE\n" XVR_CC_RESET);
    int total = 0, res = 0;

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        res = test_bundle_header(&bucket);
        Xvr_freeBucket(&bucket);

        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "BUNDLE HEADER: woilah cik jalan loh ya\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        res = test_bundle_from_source(&bucket);
        Xvr_freeBucket(&bucket);
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "BUNDLE FROM SOURCE: woilah cik jalan juga loh "
                   "ya\n" XVR_CC_RESET);
        }
        total += res;
    }

    return total;
}
