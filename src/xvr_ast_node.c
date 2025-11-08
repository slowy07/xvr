#include "xvr_ast_node.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "xvr_literal.h"
#include "xvr_memory.h"

static void freeASTNodeCustom(Xvr_ASTNode* node, bool freeSelf) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
    case XVR_AST_NODE_ERROR:
        break;

    case XVR_AST_NODE_LITERAL:
        Xvr_freeLiteral(node->atomic.literal);
        break;

    case XVR_AST_NODE_UNARY:
        Xvr_freeASTNode(node->unary.child);
        break;

    case XVR_AST_NODE_BINARY:
        Xvr_freeASTNode(node->binary.left);
        Xvr_freeASTNode(node->binary.right);
        break;

    case XVR_AST_NODE_TERNARY:
        Xvr_freeASTNode(node->ternary.condition);
        Xvr_freeASTNode(node->ternary.thenPath);
        Xvr_freeASTNode(node->ternary.elsePath);
        break;

    case XVR_AST_NODE_GROUPING:
        Xvr_freeASTNode(node->grouping.child);
        break;

    case XVR_AST_NODE_BLOCK:
        for (int i = 0; i < node->block.count; i++) {
            freeASTNodeCustom(node->block.nodes + i, false);
        }
        XVR_FREE_ARRAY(Xvr_ASTNode, node->block.nodes, node->block.capacity);
        break;

    case XVR_AST_NODE_COMPOUND:
        for (int i = 0; i < node->compound.count; i++) {
            freeASTNodeCustom(node->compound.nodes + i, false);
        }
        XVR_FREE_ARRAY(Xvr_ASTNode, node->compound.nodes,
                       node->compound.capacity);
        break;

    case XVR_AST_NODE_PAIR:
        Xvr_freeASTNode(node->pair.left);
        Xvr_freeASTNode(node->pair.right);
        break;

    case XVR_AST_NODE_INDEX:
        Xvr_freeASTNode(node->index.first);
        Xvr_freeASTNode(node->index.second);
        Xvr_freeASTNode(node->index.third);
        break;

    case XVR_AST_NODE_VAR_DECL:
        Xvr_freeLiteral(node->varDecl.identifier);
        Xvr_freeLiteral(node->varDecl.typeLiteral);
        Xvr_freeASTNode(node->varDecl.expression);
        break;

    case XVR_AST_NODE_FN_COLLECTION:
        for (int i = 0; i < node->fnCollection.count; i++) {
            freeASTNodeCustom(node->fnCollection.nodes + i, false);
        }
        XVR_FREE_ARRAY(Xvr_ASTNode, node->fnCollection.nodes,
                       node->fnCollection.capacity);
        break;

    case XVR_AST_NODE_FN_DECL:
        Xvr_freeLiteral(node->fnDecl.identifier);
        Xvr_freeASTNode(node->fnDecl.arguments);
        Xvr_freeASTNode(node->fnDecl.returns);
        Xvr_freeASTNode(node->fnDecl.block);
        break;

    case XVR_AST_NODE_FN_CALL:
        Xvr_freeASTNode(node->fnCall.arguments);
        break;

    case XVR_AST_NODE_FN_RETURN:
        Xvr_freeASTNode(node->returns.returns);
        break;

    case XVR_AST_NODE_IF:
        Xvr_freeASTNode(node->pathIf.condition);
        Xvr_freeASTNode(node->pathIf.thenPath);
        Xvr_freeASTNode(node->pathIf.elsePath);
        break;

    case XVR_AST_NODE_WHILE:
        Xvr_freeASTNode(node->pathWhile.condition);
        Xvr_freeASTNode(node->pathWhile.thenPath);
        break;

    case XVR_AST_NODE_FOR:
        Xvr_freeASTNode(node->pathFor.preClause);
        Xvr_freeASTNode(node->pathFor.postClause);
        Xvr_freeASTNode(node->pathFor.condition);
        Xvr_freeASTNode(node->pathFor.thenPath);
        break;

    case XVR_AST_NODE_BREAK:
        break;

    case XVR_AST_NODE_CONTINUE:
        break;

    case XVR_AST_NODE_PREFIX_INCREMENT:
        Xvr_freeLiteral(node->prefixIncrement.identifier);
        break;
    case XVR_AST_NODE_PREFIX_DECREMENT:
        Xvr_freeLiteral(node->prefixDecrement.identifier);
        break;
    case XVR_AST_NODE_POSTFIX_INCREMENT:
        Xvr_freeLiteral(node->postfixIncrement.identifier);
        break;
    case XVR_AST_NODE_POSTFIX_DECREMENT:
        Xvr_freeLiteral(node->postfixDecrement.identifier);
        break;

    case XVR_AST_NODE_IMPORT:
        Xvr_freeLiteral(node->import.identifier);
        Xvr_freeLiteral(node->import.alias);
        break;
    }

    if (freeSelf) {
        XVR_FREE(Xvr_ASTNode, node);
    }
}

void Xvr_freeASTNode(Xvr_ASTNode* node) { freeASTNodeCustom(node, true); }

void Xvr_emitASTNodeLiteral(Xvr_ASTNode** nodeHandle, Xvr_Literal literal) {
    // allocate a new node
    *nodeHandle = XVR_ALLOCATE(Xvr_ASTNode, 1);

    (*nodeHandle)->type = XVR_AST_NODE_LITERAL;
    (*nodeHandle)->atomic.literal = Xvr_copyLiteral(literal);
}

void Xvr_emitASTNodeUnary(Xvr_ASTNode** nodeHandle, Xvr_Opcode opcode,
                          Xvr_ASTNode* child) {
    // allocate a new node
    *nodeHandle = XVR_ALLOCATE(Xvr_ASTNode, 1);

    (*nodeHandle)->type = XVR_AST_NODE_UNARY;
    (*nodeHandle)->unary.opcode = opcode;
    (*nodeHandle)->unary.child = child;
}

void Xvr_emitASTNodeBinary(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* rhs,
                           Xvr_Opcode opcode) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_BINARY;
    tmp->binary.opcode = opcode;
    tmp->binary.left = *nodeHandle;
    tmp->binary.right = rhs;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeTernary(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* condition,
                            Xvr_ASTNode* thenPath, Xvr_ASTNode* elsePath) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_TERNARY;
    tmp->ternary.condition = condition;
    tmp->ternary.thenPath = thenPath;
    tmp->ternary.elsePath = elsePath;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeGrouping(Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_GROUPING;
    tmp->grouping.child = *nodeHandle;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeBlock(Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_BLOCK;
    tmp->block.nodes = NULL;  // NOTE: appended by the parser
    tmp->block.capacity = 0;
    tmp->block.count = 0;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeCompound(Xvr_ASTNode** nodeHandle,
                             Xvr_LiteralType literalType) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_COMPOUND;
    tmp->compound.literalType = literalType;
    tmp->compound.nodes = NULL;
    tmp->compound.capacity = 0;
    tmp->compound.count = 0;

    *nodeHandle = tmp;
}

void Xvr_setASTNodePair(Xvr_ASTNode* node, Xvr_ASTNode* left,
                        Xvr_ASTNode* right) {
    node->type = XVR_AST_NODE_PAIR;
    node->pair.left = left;
    node->pair.right = right;
}

void Xvr_emitASTNodeIndex(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* first,
                          Xvr_ASTNode* second, Xvr_ASTNode* third) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_INDEX;
    tmp->index.first = first;
    tmp->index.second = second;
    tmp->index.third = third;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeVarDecl(Xvr_ASTNode** nodeHandle, Xvr_Literal identifier,
                            Xvr_Literal typeLiteral, Xvr_ASTNode* expression) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_VAR_DECL;
    tmp->varDecl.identifier = identifier;
    tmp->varDecl.typeLiteral = typeLiteral;
    tmp->varDecl.expression = expression;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeFnCollection(Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_FN_COLLECTION;
    tmp->fnCollection.nodes = NULL;
    tmp->fnCollection.capacity = 0;
    tmp->fnCollection.count = 0;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeFnDecl(Xvr_ASTNode** nodeHandle, Xvr_Literal identifier,
                           Xvr_ASTNode* arguments, Xvr_ASTNode* returns,
                           Xvr_ASTNode* block) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_FN_DECL;
    tmp->fnDecl.identifier = identifier;
    tmp->fnDecl.arguments = arguments;
    tmp->fnDecl.returns = returns;
    tmp->fnDecl.block = block;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeFnCall(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* arguments) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_FN_CALL;
    tmp->fnCall.arguments = arguments;
    tmp->fnCall.argumentCount = arguments->fnCollection.count;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeFnReturn(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* returns) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_FN_RETURN;
    tmp->returns.returns = returns;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeIf(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* condition,
                       Xvr_ASTNode* thenPath, Xvr_ASTNode* elsePath) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_IF;
    tmp->pathIf.condition = condition;
    tmp->pathIf.thenPath = thenPath;
    tmp->pathIf.elsePath = elsePath;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeWhile(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* condition,
                          Xvr_ASTNode* thenPath) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_WHILE;
    tmp->pathWhile.condition = condition;
    tmp->pathWhile.thenPath = thenPath;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeFor(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* preClause,
                        Xvr_ASTNode* condition, Xvr_ASTNode* postClause,
                        Xvr_ASTNode* thenPath) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_FOR;
    tmp->pathFor.preClause = preClause;
    tmp->pathFor.condition = condition;
    tmp->pathFor.postClause = postClause;
    tmp->pathFor.thenPath = thenPath;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeBreak(Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_BREAK;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeContinue(Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_CONTINUE;

    *nodeHandle = tmp;
}

void Xvr_emitASTNodePrefixIncrement(Xvr_ASTNode** nodeHandle,
                                    Xvr_Literal identifier) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_PREFIX_INCREMENT;
    tmp->prefixIncrement.identifier = Xvr_copyLiteral(identifier);

    *nodeHandle = tmp;
}

void Xvr_emitASTNodePrefixDecrement(Xvr_ASTNode** nodeHandle,
                                    Xvr_Literal identifier) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_PREFIX_DECREMENT;
    tmp->prefixDecrement.identifier = Xvr_copyLiteral(identifier);

    *nodeHandle = tmp;
}

void Xvr_emitASTNodePostfixIncrement(Xvr_ASTNode** nodeHandle,
                                     Xvr_Literal identifier) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_POSTFIX_INCREMENT;
    tmp->postfixIncrement.identifier = Xvr_copyLiteral(identifier);

    *nodeHandle = tmp;
}

void Xvr_emitASTNodePostfixDecrement(Xvr_ASTNode** nodeHandle,
                                     Xvr_Literal identifier) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_POSTFIX_DECREMENT;
    tmp->postfixDecrement.identifier = Xvr_copyLiteral(identifier);

    *nodeHandle = tmp;
}

void Xvr_emitASTNodeImport(Xvr_ASTNode** nodeHandle, Xvr_Literal identifier,
                           Xvr_Literal alias) {
    Xvr_ASTNode* tmp = XVR_ALLOCATE(Xvr_ASTNode, 1);

    tmp->type = XVR_AST_NODE_IMPORT;
    tmp->import.identifier = Xvr_copyLiteral(identifier);
    tmp->import.alias = Xvr_copyLiteral(alias);

    *nodeHandle = tmp;
}
