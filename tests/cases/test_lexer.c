#include <stdio.h>
#include <string.h>

#include "xvr_console_colors.h"
#include "xvr_lexer.h"

int main() {
    printf(XVR_CC_WARN "TESTING: XVR LEXER\n" XVR_CC_RESET);
    {
        char* source = "print null;";

        Xvr_Lexer lexer;
        Xvr_bindLexer(&lexer, source);

        Xvr_Token print = Xvr_private_scanLexer(&lexer);
        Xvr_Token null = Xvr_private_scanLexer(&lexer);
        Xvr_Token semi = Xvr_private_scanLexer(&lexer);
        Xvr_Token eof = Xvr_private_scanLexer(&lexer);

        if (strncmp(print.lexeme, "print", print.length)) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "test_lexer(print lexeme): lexeme error: %s" XVR_CC_RESET,
                    print.lexeme);
            return -1;
        }

        if (strncmp(null.lexeme, "null", null.length)) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "test_lexer(null lexeme): lexeme error: %s" XVR_CC_RESET,
                    null.lexeme);
            return -1;
        }

        if (strncmp(semi.lexeme, ";", semi.length)) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "test_lexer(semicolon): lexeme error: %s" XVR_CC_RESET,
                    null.lexeme);
        }

        if (eof.type != XVR_TOKEN_EOF) {
            fprintf(stderr, XVR_CC_ERROR
                    "test_lexer(EOF): failed to find EOF token" XVR_CC_RESET);
            return -1;
        }
    }

    printf(XVR_CC_NOTICE "LEXER: PASSED nice one rek gass\n" XVR_CC_RESET);
    return 0;
}
