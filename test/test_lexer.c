#include <stdio.h>
#include <string.h>

#include "xvr_console_colors.h"
#include "xvr_lexer.h"

int main() {
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
        return -1;
    }

    if (strncmp(null.lexeme, "null", null.length)) {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: woilah cik null lexeme is wrong: %s\n" XVR_CC_RESET,
                null.lexeme);
        return -1;
    }

    if (strncmp(semi.lexeme, ";", semi.length)) {
        fprintf(
            stderr,
            XVR_CC_ERROR
            "ERROR: woilah rek semicolon lexeme is wrong: %s\n" XVR_CC_RESET,
            semi.lexeme);
        return -1;
    }

    if (eof.type != XVR_TOKEN_EOF) {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: woilah cik Failed to find EOF token\n" XVR_CC_RESET);
        return -1;
    }

    printf(XVR_CC_NOTICE "LEXER TEST: aman rek\n" XVR_CC_RESET);
    return 0;
}
