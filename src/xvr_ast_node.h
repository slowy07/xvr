/**
MIT License

Copyright (c) 2025 arfy slowy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef XVR_AST_NODE_H
#define XVR_AST_NODE_H

#include "xvr_common.h"
#include "xvr_literal.h"
#include "xvr_opcodes.h"
#include "xvr_token_types.h"

typedef union Xvr_private_node Xvr_ASTNode;

typedef enum Xvr_ASTNodeType {
    XVR_AST_NODE_ERROR,
    XVR_AST_NODE_LITERAL,
    XVR_AST_NODE_UNARY,
    XVR_AST_NODE_BINARY,
    XVR_AST_NODE_TERNARY,
    XVR_AST_NODE_GROUPING,
    XVR_AST_NODE_BLOCK,
    XVR_AST_NODE_COMPOUND,
    XVR_AST_NODE_PAIR,
    XVR_AST_NODE_INDEX,
    XVR_AST_NODE_VAR_DECL,

    XVR_AST_NODE_FN_DECL,

    XVR_AST_NODE_FN_COLLECTION,
    XVR_AST_NODE_FN_CALL,
    XVR_AST_NODE_FN_RETURN,
    XVR_AST_NODE_IF,
    XVR_AST_NODE_WHILE,
    XVR_AST_NODE_FOR,
    XVR_AST_NODE_BREAK,
    XVR_AST_NODE_CONTINUE,
    XVR_AST_NODE_PREFIX_INCREMENT,
    XVR_AST_NODE_POSTFIX_INCREMENT,
    XVR_AST_NODE_PREFIX_DECREMENT,
    XVR_AST_NODE_POSTFIX_DECREMENT,
    XVR_AST_NODE_IMPORT,
} Xvr_ASTNodeType;

void Xvr_emitASTNodeLiteral(Xvr_ASTNode** nodeHandle, Xvr_Literal literal);

typedef struct Xvr_NodeLiteral {
    Xvr_ASTNodeType type;
    Xvr_Literal literal;
} Xvr_NodeLiteral;

void Xvr_emitASTNodeUnary(Xvr_ASTNode** nodeHandle, Xvr_Opcode opcode,
                          Xvr_ASTNode* child);

typedef struct Xvr_NodeUnary {
    Xvr_ASTNodeType type;
    Xvr_Opcode opcode;
    Xvr_ASTNode* child;
} Xvr_NodeUnary;

void Xvr_emitASTNodeBinary(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* rhs,
                           Xvr_Opcode opcode);

typedef struct Xvr_NodeBinary {
    Xvr_ASTNodeType type;
    Xvr_Opcode opcode;
    Xvr_ASTNode* left;
    Xvr_ASTNode* right;
} Xvr_NodeBinary;

void Xvr_emitASTNodeTernary(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* condition,
                            Xvr_ASTNode* thenPath, Xvr_ASTNode* elsePath);

typedef struct Xvr_NodeTernary {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* condition;
    Xvr_ASTNode* thenPath;
    Xvr_ASTNode* elsePath;
} Xvr_NodeTernary;

void Xvr_emitASTNodeGrouping(Xvr_ASTNode** nodeHandle);

typedef struct Xvr_NodeGrouping {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* child;
} Xvr_NodeGrouping;

void Xvr_emitASTNodeBlock(Xvr_ASTNode** nodeHandle);

typedef struct Xvr_NodeBlock {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* nodes;
    int capacity;
    int count;
} Xvr_NodeBlock;

void Xvr_emitASTNodeCompound(Xvr_ASTNode** nodeHandle,
                             Xvr_LiteralType literalType);

typedef struct Xvr_NodeCompound {
    Xvr_ASTNodeType type;
    Xvr_LiteralType literalType;
    Xvr_ASTNode* nodes;
    int capacity;
    int count;
} Xvr_NodeCompound;

void Xvr_setASTNodePair(Xvr_ASTNode* node, Xvr_ASTNode* left,
                        Xvr_ASTNode* right);

typedef struct Xvr_NodePair {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* left;
    Xvr_ASTNode* right;
} Xvr_NodePair;

void Xvr_emitASTNodeIndex(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* first,
                          Xvr_ASTNode* second, Xvr_ASTNode* third);

typedef struct Xvr_NodeIndex {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* first;
    Xvr_ASTNode* second;
    Xvr_ASTNode* third;
} Xvr_NodeIndex;

void Xvr_emitASTNodeVarDecl(Xvr_ASTNode** nodeHandle, Xvr_Literal identifier,
                            Xvr_Literal typeLiteral, Xvr_ASTNode* expression);

typedef struct Xvr_NodeVarDecl {
    Xvr_ASTNodeType type;
    Xvr_Literal identifier;
    Xvr_Literal typeLiteral;
    Xvr_ASTNode* expression;
} Xvr_NodeVarDecl;

void Xvr_emitASTNodeFnCollection(Xvr_ASTNode** nodeHandle);

typedef struct Xvr_NodeFnCollection {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* nodes;
    int capacity;
    int count;
} Xvr_NodeFnCollection;

void Xvr_emitASTNodeFnDecl(Xvr_ASTNode** nodeHandle, Xvr_Literal identifier,
                           Xvr_ASTNode* arguments, Xvr_ASTNode* returns,
                           Xvr_ASTNode* block);

typedef struct Xvr_NodeFnDecl {
    Xvr_ASTNodeType type;
    Xvr_Literal identifier;
    Xvr_ASTNode* arguments;
    Xvr_ASTNode* returns;
    Xvr_ASTNode* block;
} Xvr_NodeFnDecl;

void Xvr_emitASTNodeFnCall(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* arguments);

typedef struct Xvr_NodeFnCall {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* arguments;
    int argumentCount;
} Xvr_NodeFnCall;

void Xvr_emitASTNodeFnReturn(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* returns);

typedef struct Xvr_NodeFnReturn {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* returns;
} Xvr_NodeFnReturn;

void Xvr_emitASTNodeIf(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* condition,
                       Xvr_ASTNode* thenPath, Xvr_ASTNode* elsePath);
void Xvr_emitASTNodeWhile(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* condition,
                          Xvr_ASTNode* thenPath);
void Xvr_emitASTNodeFor(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* preClause,
                        Xvr_ASTNode* condition, Xvr_ASTNode* postClause,
                        Xvr_ASTNode* thenPath);
void Xvr_emitASTNodeBreak(Xvr_ASTNode** nodeHandle);
void Xvr_emitASTNodeContinue(Xvr_ASTNode** nodeHandle);

typedef struct Xvr_NodeIf {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* condition;
    Xvr_ASTNode* thenPath;
    Xvr_ASTNode* elsePath;
} Xvr_NodeIf;

typedef struct Xvr_NodeWhile {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* condition;
    Xvr_ASTNode* thenPath;
} Xvr_NodeWhile;

typedef struct Xvr_NodeFor {
    Xvr_ASTNodeType type;
    Xvr_ASTNode* preClause;
    Xvr_ASTNode* condition;
    Xvr_ASTNode* postClause;
    Xvr_ASTNode* thenPath;
} Xvr_NodeFor;

typedef struct Xvr_NodeBreak {
    Xvr_ASTNodeType type;
} Xvr_NodeBreak;

typedef struct Xvr_NodeContinue {
    Xvr_ASTNodeType type;
} Xvr_NodeContinue;

void Xvr_emitASTNodePrefixIncrement(Xvr_ASTNode** nodeHandle,
                                    Xvr_Literal identifier);
void Xvr_emitASTNodePrefixDecrement(Xvr_ASTNode** nodeHandle,
                                    Xvr_Literal identifier);
void Xvr_emitASTNodePostfixIncrement(Xvr_ASTNode** nodeHandle,
                                     Xvr_Literal identifier);
void Xvr_emitASTNodePostfixDecrement(Xvr_ASTNode** nodeHandle,
                                     Xvr_Literal identifier);

typedef struct Xvr_NodePrefixIncrement {
    Xvr_ASTNodeType type;
    Xvr_Literal identifier;
} Xvr_NodePrefixIncrement;

typedef struct Xvr_NodePrefixDecrement {
    Xvr_ASTNodeType type;
    Xvr_Literal identifier;
} Xvr_NodePrefixDecrement;

typedef struct Xvr_NodePostfixIncrement {
    Xvr_ASTNodeType type;
    Xvr_Literal identifier;
} Xvr_NodePostfixIncrement;

typedef struct Xvr_NodePostfixDecrement {
    Xvr_ASTNodeType type;
    Xvr_Literal identifier;
} Xvr_NodePostfixDecrement;

void Xvr_emitASTNodeImport(Xvr_ASTNode** nodeHandle, Xvr_Literal identifier,
                           Xvr_Literal alias);

typedef struct Xvr_NodeImport {
    Xvr_ASTNodeType type;
    Xvr_Literal identifier;
    Xvr_Literal alias;
} Xvr_NodeImport;

union Xvr_private_node {
    Xvr_ASTNodeType type;
    Xvr_NodeLiteral atomic;
    Xvr_NodeUnary unary;
    Xvr_NodeBinary binary;
    Xvr_NodeTernary ternary;
    Xvr_NodeGrouping grouping;
    Xvr_NodeBlock block;
    Xvr_NodeCompound compound;
    Xvr_NodePair pair;
    Xvr_NodeIndex index;
    Xvr_NodeVarDecl varDecl;
    Xvr_NodeFnCollection fnCollection;
    Xvr_NodeFnDecl fnDecl;
    Xvr_NodeFnCall fnCall;
    Xvr_NodeFnReturn returns;
    Xvr_NodeIf pathIf;
    Xvr_NodeWhile pathWhile;
    Xvr_NodeFor pathFor;
    Xvr_NodeBreak pathBreak;
    Xvr_NodeContinue pathContinue;
    Xvr_NodePrefixIncrement prefixIncrement;
    Xvr_NodePrefixDecrement prefixDecrement;
    Xvr_NodePostfixIncrement postfixIncrement;
    Xvr_NodePostfixDecrement postfixDecrement;
    Xvr_NodeImport import;
};

XVR_API void Xvr_freeASTNode(Xvr_ASTNode* node);

#endif  // !XVR_AST_NODE_H
