#include <stdio.h>
#include <string.h>

#include "xvr_console_colors.h"
#include "xvr_lexer.h"

int main(void) {
    int testCount = 0;
    int passCount = 0;

    // Test 1: Basic semicolon tokenization
    {
        testCount++;
        char* source = "var null;";
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, source);

        Xvr_Token print = Xvr_private_scanLexer(&lexer);
        Xvr_Token null = Xvr_private_scanLexer(&lexer);
        Xvr_Token semi = Xvr_private_scanLexer(&lexer);
        Xvr_Token eof = Xvr_private_scanLexer(&lexer);

        if (strncmp(print.lexeme, "var", print.length)) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: woilah cik print lexeme wrong: %s\n" XVR_CC_RESET,
                    print.lexeme);
        } else if (strncmp(null.lexeme, "null", null.length)) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: woilah cik null lexeme is wrong: %s\n" XVR_CC_RESET,
                    null.lexeme);
        } else if (strncmp(semi.lexeme, ";", semi.length)) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: woilah rek semicolon lexeme is wrong: "
                    "%s\n" XVR_CC_RESET,
                    semi.lexeme);
        } else if (eof.type != XVR_TOKEN_EOF) {
            fprintf(
                stderr, XVR_CC_ERROR
                "ERROR: woilah cik Failed to find EOF token\n" XVR_CC_RESET);
        } else {
            passCount++;
            printf(XVR_CC_NOTICE
                   "PASS: basic semicolon tokenization\n" XVR_CC_RESET);
        }
    }

    // Test 2: Multiple semicolons
    {
        testCount++;
        char* source = ";;";
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, source);

        Xvr_Token semi1 = Xvr_private_scanLexer(&lexer);
        Xvr_Token semi2 = Xvr_private_scanLexer(&lexer);
        Xvr_Token eof = Xvr_private_scanLexer(&lexer);

        if (semi1.type != XVR_TOKEN_SEMICOLON) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: First semicolon not recognized\n" XVR_CC_RESET);
        } else if (semi2.type != XVR_TOKEN_SEMICOLON) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: Second semicolon not recognized\n" XVR_CC_RESET);
        } else if (eof.type != XVR_TOKEN_EOF) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: EOF not found after multiple "
                    "semicolons\n" XVR_CC_RESET);
        } else {
            passCount++;
            printf(XVR_CC_NOTICE "PASS: multiple semicolons\n" XVR_CC_RESET);
        }
    }

    // Test 3: Semicolon token type verification
    {
        testCount++;
        char* source = ";";
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, source);

        Xvr_Token semi = Xvr_private_scanLexer(&lexer);

        if (semi.type != XVR_TOKEN_SEMICOLON) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Token type is %d, expected %d\n" XVR_CC_RESET,
                    semi.type, XVR_TOKEN_SEMICOLON);
        } else if (semi.length != 1) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Semicolon length is %d, expected 1\n" XVR_CC_RESET,
                    semi.length);
        } else {
            passCount++;
            printf(XVR_CC_NOTICE
                   "PASS: semicolon token type verification\n" XVR_CC_RESET);
        }
    }

    // Test 4: Semicolons in for loop context
    {
        testCount++;
        char* source = "for (;;)";
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, source);

        Xvr_Token for_kw = Xvr_private_scanLexer(&lexer);
        Xvr_Token paren_l = Xvr_private_scanLexer(&lexer);
        Xvr_Token semi1 = Xvr_private_scanLexer(&lexer);
        Xvr_Token semi2 = Xvr_private_scanLexer(&lexer);
        Xvr_Token paren_r = Xvr_private_scanLexer(&lexer);
        Xvr_Token eof = Xvr_private_scanLexer(&lexer);

        if (for_kw.type != XVR_TOKEN_FOR) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: 'for' keyword not recognized\n" XVR_CC_RESET);
        } else if (paren_l.type != XVR_TOKEN_PAREN_LEFT) {
            fprintf(stderr,
                    XVR_CC_ERROR "ERROR: '(' not recognized\n" XVR_CC_RESET);
        } else if (semi1.type != XVR_TOKEN_SEMICOLON ||
                   semi2.type != XVR_TOKEN_SEMICOLON) {
            fprintf(
                stderr, XVR_CC_ERROR
                "ERROR: Semicolons in for loop not recognized\n" XVR_CC_RESET);
        } else if (paren_r.type != XVR_TOKEN_PAREN_RIGHT) {
            fprintf(stderr,
                    XVR_CC_ERROR "ERROR: ')' not recognized\n" XVR_CC_RESET);
        } else if (eof.type != XVR_TOKEN_EOF) {
            fprintf(stderr, XVR_CC_ERROR "ERROR: EOF not found\n" XVR_CC_RESET);
        } else {
            passCount++;
            printf(XVR_CC_NOTICE
                   "PASS: semicolons in for loop context\n" XVR_CC_RESET);
        }
    }

    // Test 5: Mixed statements with semicolons
    {
        testCount++;
        char* source = "var x = 1; var y = 2";
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, source);

        int semi_count = 0;
        Xvr_Token tok;
        while ((tok = Xvr_private_scanLexer(&lexer)).type != XVR_TOKEN_EOF) {
            if (tok.type == XVR_TOKEN_SEMICOLON) {
                semi_count++;
            }
        }

        if (semi_count != 1) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Expected 1 semicolon, found %d\n" XVR_CC_RESET,
                    semi_count);
        } else {
            passCount++;
            printf(XVR_CC_NOTICE
                   "PASS: mixed statements with semicolons\n" XVR_CC_RESET);
        }
    }

    printf(XVR_CC_NOTICE "\nLEXER TEST: %d/%d passed\n" XVR_CC_RESET, passCount,
           testCount);

    if (passCount != testCount) {
        fprintf(stderr, XVR_CC_ERROR
                "LEXER TEST: woilah cik, some tests failed!\n" XVR_CC_RESET);
        return 1;
    }

    printf(XVR_CC_NOTICE "LEXER TEST: aman rek\n" XVR_CC_RESET);
    return 0;
}
