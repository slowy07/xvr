#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstdlib>

#include "xvr_ast_node.h"
#include "xvr_console_colors.h"
#include "xvr_literal.h"
#include "xvr_opcodes.h"
#include "xvr_refstring.h"

TEST_CASE("AST node literal emission", "[ast][unit]") {
    char* str = (char*)"foobar";
    Xvr_Literal literal = XVR_TO_STRING_LITERAL(Xvr_createRefString(str));

    Xvr_ASTNode* node = nullptr;
    Xvr_emitASTNodeLiteral(&node, literal);
    
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_LITERAL);

    Xvr_freeLiteral(literal);
    Xvr_freeASTNode(node);
}

TEST_CASE("AST node binary operation", "[ast][unit]") {
    char* str = (char*)"foobar";
    Xvr_Literal literal = XVR_TO_STRING_LITERAL(Xvr_createRefString(str));
    Xvr_ASTNode* nodeHandle = nullptr;
    Xvr_emitASTNodeLiteral(&nodeHandle, literal);

    Xvr_ASTNode* rhsChildNode = nullptr;
    Xvr_emitASTNodeLiteral(&rhsChildNode, literal);

    Xvr_emitASTNodeBinary(&nodeHandle, rhsChildNode, XVR_OP_PRINT);

    REQUIRE(nodeHandle->type == XVR_AST_NODE_BINARY);
    REQUIRE(nodeHandle->binary.opcode == XVR_OP_PRINT);

    Xvr_freeLiteral(literal);
    Xvr_freeASTNode(nodeHandle);
}