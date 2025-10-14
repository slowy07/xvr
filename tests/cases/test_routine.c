#include <stdio.h>
#include <stdlib.h>

#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_opcodes.h"
#include "xvr_parser.h"
#include "xvr_routine.h"

int test_routine_expression(Xvr_Bucket** bucketHandle) {
    {
        Xvr_Ast* ast = NULL;
        Xvr_private_emitAstPass(bucketHandle, &ast);

        void* buffer = Xvr_compileRoutine(ast);
        int len = ((int*)buffer)[0];

        int* ptr = (int*)buffer;

        if ((ptr++)[0] != 28 || (ptr++)[0] != 0 || (ptr++)[0] != 0 ||
            (ptr++)[0] != 0 || (ptr++)[0] != 0) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed to producing expecting "
                    "routine header: ast: PASS\n" XVR_CC_RESET);
            free(buffer);
            return -1;
        }

        if (*((unsigned char*)(buffer + 24)) != XVR_OPCODE_RETURN ||
            *((unsigned char*)(buffer + 25)) != 0 ||
            *((unsigned char*)(buffer + 27)) != 0) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed to produce the expected "
                    "routine cod, ast: PASS\n" XVR_CC_RESET);
            free(buffer);
            return -1;
        }

        free(buffer);
    }

    return 0;
}

int test_routine_keywords(Xvr_Bucket** bucketHandle) {
    {
        const char* source = "if (true) print \"woilah cik\";";
        Xvr_Lexer lexer;
        Xvr_Parser parser;

        Xvr_bindLexer(&lexer, source);
        Xvr_bindParser(&parser, &lexer);
        Xvr_Ast* ast = Xvr_scanParser(bucketHandle, &parser);

        void* buffer = Xvr_compileRoutine(ast);
        int len = ((int*)buffer)[0];

        int* ptr = (int*)buffer;
        if ((ptr++)[0] != 76 || (ptr++)[0] != 0 || (ptr++)[0] != 4 ||
            (ptr++)[0] != 12 || (ptr++)[0] != 0 || (ptr++)[0] != 32 ||
            (ptr++)[0] != 60 || (ptr++)[0] != 64 || false) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "Error: failed to produce the expected routine "
                    "header, source: %s\n" XVR_CC_RESET,
                    source);
            free(buffer);
            return -1;
        }
        free(buffer);
    }

    return 0;
}

int main() {
    printf(XVR_CC_WARN "testing: xvr routine\n" XVR_CC_RESET);
    int total = 0, res = 0;

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(sizeof(Xvr_Ast) * 32);
        res = test_routine_expression(&bucket);
        Xvr_freeBucket(&bucket);
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "test_routine_expression(): aman loh ya\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        res = test_routine_keywords(&bucket);
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "test_routine_keywords(): aman cik\n" XVR_CC_RESET);
        }
        total += res;
    }

    return total;
}
