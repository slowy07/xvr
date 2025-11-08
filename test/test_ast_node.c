#include <stdio.h>
#include <stdlib.h>

#include "xvr_ast_node.h"
#include "xvr_console_colors.h"
#include "xvr_literal.h"
#include "xvr_opcodes.h"
#include "xvr_refstring.h"

#define ASSERT(test_for_true)                                            \
    if (!(test_for_true)) {                                              \
        fprintf(stderr, XVR_CC_ERROR "assert failed: %s\n" XVR_CC_RESET, \
                #test_for_true);                                         \
        exit(1);                                                         \
    }

int main(void) {
    {
        char* str = "foobar";
        Xvr_Literal literal = XVR_TO_STRING_LITERAL(Xvr_createRefString(str));

        Xvr_ASTNode* node = NULL;
        Xvr_emitASTNodeLiteral(&node, literal);
        ASSERT(node->type == XVR_AST_NODE_LITERAL);

        Xvr_freeLiteral(literal);
        Xvr_freeASTNode(node);
    }

    {
        char* str = "foobar";
        Xvr_Literal literal = XVR_TO_STRING_LITERAL(Xvr_createRefString(str));
        Xvr_ASTNode* nodeHandle = NULL;
        Xvr_emitASTNodeLiteral(&nodeHandle, literal);

        Xvr_ASTNode* rhsChildNode = NULL;
        Xvr_emitASTNodeLiteral(&rhsChildNode, literal);

        Xvr_emitASTNodeBinary(&nodeHandle, rhsChildNode, XVR_OP_PRINT);

        ASSERT(nodeHandle->type == XVR_AST_NODE_BINARY);
        ASSERT(nodeHandle->binary.opcode == XVR_OP_PRINT);

        Xvr_freeLiteral(literal);
        Xvr_freeASTNode(nodeHandle);
    }

    printf(XVR_CC_NOTICE "AST NODE: woilah cik jalan loh ya\n" XVR_CC_RESET);
    return 0;
}
