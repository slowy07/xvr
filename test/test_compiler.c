#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter_tools.h"
#include "xvr_ast_node.h"
#include "xvr_compiler.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_memory.h"
#include "xvr_parser.h"

int main(void) {
    {
        Xvr_Compiler compiler;
        Xvr_initCompiler(&compiler);
        Xvr_freeCompiler(&compiler);
    }

    {
        char* source = "print null;";

        Xvr_Lexer lexer;
        Xvr_Parser parser;
        Xvr_Compiler compiler;

        Xvr_initLexer(&lexer, source);
        Xvr_initParser(&parser, &lexer);
        Xvr_initCompiler(&compiler);

        Xvr_ASTNode* node = Xvr_scanParser(&parser);

        size_t size = 0;
        unsigned char* bytecode = Xvr_collateCompiler(&compiler, &size);

        XVR_FREE_ARRAY(unsigned char, bytecode, size);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
        Xvr_freeCompiler(&compiler);
    }

    {
        size_t sourceLength = 0;
        const char* source = (const char*)Xvr_readFile(
            "test/xvr_file/compiler.xvr", &sourceLength);

        Xvr_Lexer lexer;
        Xvr_Parser parser;
        Xvr_Compiler compiler;

        Xvr_initLexer(&lexer, source);
        Xvr_initParser(&parser, &lexer);
        Xvr_initCompiler(&compiler);

        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        while (node != NULL) {
            if (node->type == XVR_AST_NODE_ERROR) {
                fprintf(stderr, XVR_CC_ERROR
                        "ERROR: node tidak ada cik\n" XVR_CC_RESET);
                return -1;
            }

            Xvr_writeCompiler(&compiler, node);
            Xvr_freeASTNode(node);

            node = Xvr_scanParser(&parser);

        }

        size_t size = 0;
        unsigned char* bytecode = Xvr_collateCompiler(&compiler, &size);

        XVR_FREE_ARRAY(char, source, sourceLength);
        XVR_FREE_ARRAY(unsigned char, bytecode, size);
        Xvr_freeParser(&parser);
        Xvr_freeCompiler(&compiler);
    }

    printf(XVR_CC_NOTICE
           "TEST COMPILER: woilah cik aman loh ya\n" XVR_CC_RESET);
    return 0;
}
