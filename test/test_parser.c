#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter_tools.h"
#include "xvr_console_colors.h"
#include "xvr_parser.h"

int main(void) {
    {
        char* source = "print null;";
        Xvr_Lexer lexer;
        Xvr_Parser parser;
        Xvr_initLexer(&lexer, source);
        Xvr_initParser(&parser, &lexer);
    }

    {
        char* source = "print(100_000);";
        Xvr_Lexer lexer;
        Xvr_Parser parser;
        Xvr_initLexer(&lexer, source);
        Xvr_initParser(&parser, &lexer);

        Xvr_ASTNode* node = Xvr_scanParser(&parser);

        if (node == NULL) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: ASTNode error null failed cik\n" XVR_CC_RESET);
            return -1;
        }
    }

    printf(XVR_CC_NOTICE "TEST PARSER: woilah cik aman loh ya\n" XVR_CC_RESET);
    return 0;
}
