#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_module.h"
#include "xvr_module_builder.h"
#include "xvr_parser.h"
#include "xvr_stack.h"
#include "xvr_value.h"
#include "xvr_vm.h"

unsigned char* makeCodeFromSource(Xvr_Bucket** bucketHandle,
                                  const char* source) {
    Xvr_Lexer lexer;
    Xvr_bindLexer(&lexer, source);

    Xvr_Parser parser;
    Xvr_bindParser(&parser, &lexer);

    Xvr_Ast* ast = Xvr_scanParser(bucketHandle, &parser);
    return Xvr_compileModuleBuilder(ast);
}

int test_setup_and_teardown(Xvr_Bucket** bucketHandle) {
    {
        const char* source = "(1 + 2) * (3 + 4);";

        Xvr_Lexer lexer;
        Xvr_bindLexer(&lexer, source);

        Xvr_Parser parser;
        Xvr_bindParser(&parser, &lexer);

        Xvr_Ast* ast = Xvr_scanParser(bucketHandle, &parser);
        unsigned char* buffer = Xvr_compileModuleBuilder(ast);
        Xvr_Module module = Xvr_parseModule(buffer);

        Xvr_VM vm;
        Xvr_initVM(&vm);
        Xvr_bindVM(&vm, &module, false);

        if (vm.codeAddr != 24 || vm.jumpsCount != 0 || vm.paramCount != 0 ||
            vm.dataCount != 0 || vm.subsCount != 0) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: failed to setup and teadown Xvr_VM, source: "
                    "%s\n" XVR_CC_RESET,
                    source);
            Xvr_freeVM(&vm);
            free(buffer);
            return -1;
        }
        Xvr_freeVM(&vm);
        free(buffer);
    }

    return 0;
}

int test_simple_execution(Xvr_Bucket** bucketHandle) {
    {
        const char* source = "(1 + 2) * (3 + 4);";

        Xvr_Lexer lexer;
        Xvr_bindLexer(&lexer, source);

        Xvr_Parser parser;
        Xvr_bindParser(&parser, &lexer);

        Xvr_Ast* ast = Xvr_scanParser(bucketHandle, &parser);
        unsigned char* buffer = Xvr_compileModuleBuilder(ast);
        Xvr_Module module = Xvr_parseModule(buffer);

        Xvr_VM vm;
        Xvr_initVM(&vm);
        Xvr_bindVM(&vm, &module, false);
        Xvr_runVM(&vm);

        if (vm.stack == NULL || vm.stack->count != 1 ||
            XVR_VALUE_IS_INTEGER(Xvr_peekStack(&vm.stack)) != true ||
            XVR_VALUE_AS_INTEGER(Xvr_peekStack(&vm.stack)) != 21) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Unexpected result in 'Xvr_VM', source: "
                    "%s\n" XVR_CC_RESET,
                    source);

            // cleanup and return
            Xvr_freeVM(&vm);
            free(buffer);
            return -1;
        }
        Xvr_freeVM(&vm);
        free(buffer);
    }
    return 0;
}

int main(void) {
    printf(XVR_CC_WARN "TESTING: XVR VM\n" XVR_CC_RESET);
    int total = 0, res = 0;

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        res = test_setup_and_teardown(&bucket);
        Xvr_freeBucket(&bucket);
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "SETUP AND TEARDOWN: PASSED nice one rek jalan loh "
                   "ya cik\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        res = test_simple_execution(&bucket);
        Xvr_freeBucket(&bucket);
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "SIMPLE EXECUTION: PASSED nice one cik jalan loh "
                   "ya\n" XVR_CC_RESET);
        }
        total += res;
    }

    return total;
}
