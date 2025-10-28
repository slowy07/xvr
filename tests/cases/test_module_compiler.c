#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_module_compiler.h"
#include "xvr_opcodes.h"
#include "xvr_parser.h"

int test_builder_expression(Xvr_Bucket** bucketHandle) {
    {
        Xvr_Ast* ast = NULL;
        Xvr_private_emitAstPass(bucketHandle, &ast);
        unsigned char* buffer = Xvr_compileModule(ast);

        int* ptr = (int*)buffer;

        if ((ptr++)[0] != 28 ||  // total size
            (ptr++)[0] != 0 ||   // jump count
            (ptr++)[0] != 0 ||   // param count
            (ptr++)[0] != 0 ||   // data count
            (ptr++)[0] != 0)     // subs count
        {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: failed to produce the expected module builder "
                    "header, ast: PASS\n" XVR_CC_RESET);

            // cleanup and return
            free(buffer);
            return -1;
        }

        if (*((unsigned char*)(buffer + 24)) != XVR_OPCODE_RETURN ||
            *((unsigned char*)(buffer + 25)) != 0 ||
            *((unsigned char*)(buffer + 26)) != 0 ||
            *((unsigned char*)(buffer + 27)) != 0) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: failed to produce the expected module builder "
                    "code, ast: PASS\n" XVR_CC_RESET);

            // cleanup and return
            free(buffer);
            return -1;
        }

        // cleanup
        free(buffer);
    }

    return 0;
}

int test_builder_keywords(Xvr_Bucket** bucketHandle) {
    {
        const char* source = "assert true;";
        Xvr_Lexer lexer;
        Xvr_Parser parser;

        Xvr_bindLexer(&lexer, source);
        Xvr_bindParser(&parser, &lexer);
        Xvr_Ast* ast = Xvr_scanParser(bucketHandle, &parser);

        unsigned char* buffer = Xvr_compileModule(ast);
        int* ptr = (int*)buffer;

        if ((ptr++)[0] != 36 ||  // total size
            (ptr++)[0] != 0 ||   // jump count
            (ptr++)[0] != 0 ||   // param count
            (ptr++)[0] != 0 ||   // data count
            (ptr++)[0] != 0)     // subs count
        {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: failed to produce the expected module builder "
                    "header, source: %s\n" XVR_CC_RESET,
                    source);

            // cleanup and return
            free(buffer);
            return -1;
        }

        if (*((unsigned char*)(buffer + 24)) != XVR_OPCODE_READ ||
            *((unsigned char*)(buffer + 25)) != XVR_VALUE_BOOLEAN ||
            *((bool*)(buffer + 26)) != true ||  // bools are packed
            *((unsigned char*)(buffer + 27)) != 0 ||

            *((unsigned char*)(buffer + 28)) != XVR_OPCODE_ASSERT ||
            *((unsigned char*)(buffer + 29)) != 1 ||  // one parameter
            *((unsigned char*)(buffer + 30)) != 0 ||
            *((unsigned char*)(buffer + 31)) != 0 ||

            *((unsigned char*)(buffer + 32)) != XVR_OPCODE_RETURN ||
            *((unsigned char*)(buffer + 33)) != 0 ||
            *((unsigned char*)(buffer + 34)) != 0 ||
            *((unsigned char*)(buffer + 35)) != 0) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: failed to produce the expected module builder "
                    "code, source: %s\n" XVR_CC_RESET,
                    source);

            free(buffer);
            return -1;
        }
        free(buffer);
    }

    return 0;
}

int test_compiler_string_reuse(Xvr_Bucket** bucketHandle) {
    {
        // setup
        const char* source =
            "var first: string = \"Hello world\"; var second: string = \"Hello "
            "world\";";
        Xvr_Lexer lexer;
        Xvr_Parser parser;

        Xvr_bindLexer(&lexer, source);
        Xvr_bindParser(&parser, &lexer);
        Xvr_Ast* ast = Xvr_scanParser(bucketHandle, &parser);

        // run
        unsigned char* buffer = Xvr_compileModule(ast);

        // check header
        int* ptr = (int*)buffer;

        if ((ptr++)[0] != 108 ||  // total size
            (ptr++)[0] != 12 ||   // jump count
            (ptr++)[0] != 0 ||    // param count
            (ptr++)[0] != 28 ||   // data count
            (ptr++)[0] != 0 ||    // subs count
            (ptr++)[0] != 32 ||   // code addr
            (ptr++)[0] != 68 ||   // jumps addr
            (ptr++)[0] != 80 ||   // data addr
            false)                // terminator
        {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: failed to reuse strings in module header, source: "
                    "%s\n" XVR_CC_RESET,
                    source);

            free(buffer);
            return -1;
        }

        // check code
        if (*((unsigned char*)(buffer + 32)) != XVR_OPCODE_READ ||
            *((unsigned char*)(buffer + 33)) != XVR_VALUE_STRING ||
            *((unsigned char*)(buffer + 34)) != XVR_STRING_LEAF ||
            *((unsigned char*)(buffer + 35)) != 0 ||

            *((unsigned int*)(buffer + 36)) != 0 ||

            *((unsigned char*)(buffer + 40)) != XVR_OPCODE_DECLARE ||
            *((unsigned char*)(buffer + 41)) != XVR_VALUE_STRING ||
            *((unsigned char*)(buffer + 42)) != 5 ||
            *((unsigned char*)(buffer + 43)) != 0 ||

            *((unsigned int*)(buffer + 44)) != 4 ||

            *((unsigned char*)(buffer + 48)) != XVR_OPCODE_READ ||
            *((unsigned char*)(buffer + 49)) != XVR_VALUE_STRING ||
            *((unsigned char*)(buffer + 50)) != XVR_STRING_LEAF ||
            *((unsigned char*)(buffer + 51)) != 0 ||

            *((unsigned int*)(buffer + 52)) != 0 ||  // duplicate

            *((unsigned char*)(buffer + 56)) != XVR_OPCODE_DECLARE ||
            *((unsigned char*)(buffer + 57)) != XVR_VALUE_STRING ||
            *((unsigned char*)(buffer + 58)) != 6 ||
            *((unsigned char*)(buffer + 59)) != 0 ||

            *((unsigned int*)(buffer + 60)) != 8 ||

            *((unsigned char*)(buffer + 64)) != XVR_OPCODE_RETURN ||
            *((unsigned char*)(buffer + 65)) != 0 ||
            *((unsigned char*)(buffer + 66)) != 0 ||
            *((unsigned char*)(buffer + 67)) != 0) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: failed to produce the expected module code, "
                    "source: %s\n" XVR_CC_RESET,
                    source);

            free(buffer);
            return -1;
        }

        free(buffer);
    }
    return 0;
}

int main(void) {
    printf(XVR_CC_WARN "TESTING: XVR MODULE BUILDER\n" XVR_CC_RESET);

    int total = 0, res = 0;

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        res = test_builder_expression(&bucket);
        Xvr_freeBucket(&bucket);
        if (res == 0) {
            printf(
                XVR_CC_NOTICE
                "BUILDER EXPRESSION: woilah cik jalan loh ya\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        res = test_builder_keywords(&bucket);
        Xvr_freeBucket(&bucket);
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "BUILDER KEYWORDS: woilah cik jalan loh ya\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        res = test_compiler_string_reuse(&bucket);
        Xvr_freeBucket(&bucket);
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "COMPILER STRING REUSE: woilah cik jalan loh "
                   "ya\n" XVR_CC_RESET);
        }
        total += res;
    }

    return total;
}
