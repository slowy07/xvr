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

/**
 * @brief abstrack syntax tree (AST) node representation for the xvr compiler
 *
 * memory management:
 *   - all nodes are heap-allocated
 *   - Xvr_freeASTNode recursively frees all children (DFS post-order)
 *   - Nodes Xvr_Literal values by value (copied / deep-freed)
 *   - child pointers are owned
 *
 * threading:
 *   - not thread-safe external synchronization required
 *   - no atomic operations on node structure
 *
 */

#ifndef XVR_AST_NODE_H
#define XVR_AST_NODE_H

#include "xvr_common.h"
#include "xvr_literal.h"
#include "xvr_opcodes.h"
#include "xvr_token_types.h"

typedef union Xvr_private_node Xvr_ASTNode;

/**
 * @enum Xvr_ASTNodeType
 *
 * grouping data
 * - error first
 * - expression
 * - grouping / containers (GROUPING -> PAIR -> INDEX)
 * - declaration (VAR_DECL -> FN_DECL)
 * - procedure related (FN_COLLECTION -> FN_RETURN)
 * - control flow (if -> continue)
 * - operators (PREFIX_* -> POSTIF_*)
 * - import last
 */
typedef enum Xvr_ASTNodeType {
    XVR_AST_NODE_ERROR,     // parse / semantic error placeholder
    XVR_AST_NODE_LITERAL,   // constant: 42, "wello", true
    XVR_AST_NODE_UNARY,     // !x, -x, ~x
    XVR_AST_NODE_BINARY,    // x + y, x < y (opcode + left + right)
    XVR_AST_NODE_TERNARY,   // a ? b : c (condition ? then : else)
    XVR_AST_NODE_GROUPING,  // (expr) (parentheses, precedence)
    XVR_AST_NODE_BLOCK,     // {statement; statement} (sequence of statement)
    XVR_AST_NODE_COMPOUND,  // [a, b, c] or {k : v} (type + elements)
    XVR_AST_NODE_PAIR,      // key-value pair (internals use in compound)
    XVR_AST_NODE_INDEX,     // arr[i] or dict[i] or arr[i:j:k] (1-3 indices)
    XVR_AST_NODE_VAR_DECL,  // var x: int = 2 (name ,type, initializer)

    XVR_AST_NODE_FN_DECL,  // proc add(x: int, y: int): int {x + y; }

    XVR_AST_NODE_FN_COLLECTION,      // collection of cuntion declaration
                                     // (module-level)
    XVR_AST_NODE_FN_CALL,            // add(1, 2) (calle + args)
    XVR_AST_NODE_FN_RETURN,          // return expr (optional value)
    XVR_AST_NODE_IF,                 // if cond then stmt else stmt
    XVR_AST_NODE_WHILE,              // while cond stmt
    XVR_AST_NODE_FOR,                // for pre; cond; post stmt
    XVR_AST_NODE_BREAK,              // break
    XVR_AST_NODE_CONTINUE,           // continue
    XVR_AST_NODE_PREFIX_INCREMENT,   // ++x
    XVR_AST_NODE_POSTFIX_INCREMENT,  // x++
    XVR_AST_NODE_PREFIX_DECREMENT,   // --x
    XVR_AST_NODE_POSTFIX_DECREMENT,  // x--
    XVR_AST_NODE_IMPORT,             // import lib as alias_lib
    XVR_AST_NODE_PASS                // for do nothing
} Xvr_ASTNodeType;

void Xvr_emitASTNodeLiteral(Xvr_ASTNode** nodeHandle, Xvr_Literal literal);

/**
 * @struct Xvr_NodeLiteral
 * @brief leaf node: constant value
 *
 * example: 30, "hello", true
 * memory: holds Xvr_Literal by value (copied on creatio, freed on destroy)
 */
typedef struct Xvr_NodeLiteral {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_LITERAL
    Xvr_Literal literal;   // constant value
} Xvr_NodeLiteral;

void Xvr_emitASTNodeUnary(Xvr_ASTNode** nodeHandle, Xvr_Opcode opcode,
                          Xvr_ASTNode* child);

/**
 * @struct Xvr_NodeUnary
 * @brief unary operation: !x, -x, ~x
 *
 * memory: child is owned; opcode is by-value
 */
typedef struct Xvr_NodeUnary {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_UNARY
    Xvr_Opcode opcode;     // operation
    Xvr_ASTNode* child;    // operand expression
} Xvr_NodeUnary;

void Xvr_emitASTNodeBinary(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* rhs,
                           Xvr_Opcode opcode);

/**
 * @struct Xvr_NodeBinary
 * @brief binary operation: x + y, x < y, x and y
 *
 * memory: left, right are owned
 */
typedef struct Xvr_NodeBinary {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_BINARY
    Xvr_Opcode opcode;     // operation
    Xvr_ASTNode* left;     // left operand
    Xvr_ASTNode* right;    // right operand
} Xvr_NodeBinary;

void Xvr_emitASTNodeTernary(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* condition,
                            Xvr_ASTNode* thenPath, Xvr_ASTNode* elsePath);

/**
 * @struct Xvr_NodeTernary
 * @brief ternary conditional: condition ? then : else
 *
 * memory: condition, thenPath, elsePath are owned
 */
typedef struct Xvr_NodeTernary {
    Xvr_ASTNodeType type;    // XVR_AST_NODE_TERNARY
    Xvr_ASTNode* condition;  // boolean expression
    Xvr_ASTNode* thenPath;   // value if true
    Xvr_ASTNode* elsePath;   // value is false
} Xvr_NodeTernary;

void Xvr_emitASTNodeGrouping(Xvr_ASTNode** nodeHandle);

/**
 * @struct Xvr_NodeGrouping
 * @brief parentheses expression: (expr) -> (1 + 2) * 3
 *
 * memory: child is owned
 */
typedef struct Xvr_NodeGrouping {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_GROUPING
    Xvr_ASTNode* child;    // inner expression
} Xvr_NodeGrouping;

void Xvr_emitASTNodeBlock(Xvr_ASTNode** nodeHandle);

/**
 * @struct Xvr_NodeBlock
 * @brief statement sequence: {stmt; stmt}
 *
 * example: procedure body, loop body
 * memory : `nodes` is array of owned `Xvr_ASTNode` (dynamic array)
 */
typedef struct Xvr_NodeBlock {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_BLOCK
    Xvr_ASTNode* nodes;    // array of statement nodes
    int capacity;          // allocated slots
    int count;             // active statements
} Xvr_NodeBlock;

void Xvr_emitASTNodeCompound(Xvr_ASTNode** nodeHandle,
                             Xvr_LiteralType literalType);

/**
 * @struct Xvr_NodeCompound
 * @brief array / dictionary literal: [a, b] or {k: v}
 *
 * example : [1, 2, 3], {"key": "value"}
 * memory: nodes contains Xvr_NodePair or values; literalType is by-value
 */
typedef struct Xvr_NodeCompound {
    Xvr_ASTNodeType type;         // XVR_AST_NODE_COMPOUND
    Xvr_LiteralType literalType;  // target type
    Xvr_ASTNode* nodes;           // array of elements
    int capacity;                 // allocated slots
    int count;                    // active elements
} Xvr_NodeCompound;

void Xvr_setASTNodePair(Xvr_ASTNode* node, Xvr_ASTNode* left,
                        Xvr_ASTNode* right);

/**
 * @struct Xvr_NodePair
 * @brief key-value pair for dictionary: "key: value"
 *
 * memory : ley (key) and right (value) are owned
 */
typedef struct Xvr_NodePair {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_PAIR
    Xvr_ASTNode* left;     // key expression
    Xvr_ASTNode* right;    // value expression
} Xvr_NodePair;

void Xvr_emitASTNodeIndex(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* first,
                          Xvr_ASTNode* second, Xvr_ASTNode* third);

/**
 * @struct Xvr_NodeIndex
 * @brief array / dictionary access: arr[i], arr[i:j:k]
 *
 * memory: first, second, third are owned (may be NULL for missing indices)
 */
typedef struct Xvr_NodeIndex {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_INDEX
    Xvr_ASTNode* first;    // target
    Xvr_ASTNode* second;   // index
    Xvr_ASTNode* third;    // step
} Xvr_NodeIndex;

void Xvr_emitASTNodeVarDecl(Xvr_ASTNode** nodeHandle, Xvr_Literal identifier,
                            Xvr_Literal typeLiteral, Xvr_ASTNode* expression);

/**
 * @struct Xvr_NodeVarDecl
 * @brief variable declaration: `var x: int = 30`
 *
 * memory: identifier, typeLiteral, copied; expression owned
 */
typedef struct Xvr_NodeVarDecl {
    Xvr_ASTNodeType type;     // XVR_AST_NODE_VAR_DECL
    Xvr_Literal identifier;   // variable name (XVR_LITERAL_IDENTIFIER)
    Xvr_Literal typeLiteral;  // type annotation
    Xvr_ASTNode* expression;  // initializer expression
} Xvr_NodeVarDecl;

void Xvr_emitASTNodeFnCollection(Xvr_ASTNode** nodeHandle);

/**
 * @struct Xvr_NodeFnCollection
 * @brief collection of function declaration
 *
 * memory : nodes array of Xvr_NodeFnDecl
 */
typedef struct Xvr_NodeFnCollection {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_FN_COLLECTION
    Xvr_ASTNode* nodes;    // array of function declaration
    int capacity;          // allocated slots
    int count;             // active function
} Xvr_NodeFnCollection;

void Xvr_emitASTNodeFnDecl(Xvr_ASTNode** nodeHandle, Xvr_Literal identifier,
                           Xvr_ASTNode* arguments, Xvr_ASTNode* returns,
                           Xvr_ASTNode* block);

/**
 * @struct Xvr_NodeFnDecl
 * @brief Function declaration: `proc add(x: int): int { return x + 1; }`
 *
 * memory: identifier, copied; argument, returns
 */
typedef struct Xvr_NodeFnDecl {
    Xvr_ASTNodeType type;    // XVR_AST_NODE_FN_DECL
    Xvr_Literal identifier;  // function name (XVR_LITERAL_IDENTIFIER)
    Xvr_ASTNode*
        arguments;  // parameter list (Xvr_NodeCompound or Xvr_NodeVarDecl)
    Xvr_ASTNode* returns;  // return type
    Xvr_ASTNode* block;    // procedure body
} Xvr_NodeFnDecl;

void Xvr_emitASTNodeFnCall(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* arguments);

/**
 * @struct Xvr_NodeFnCall
 * @brief function call: `add(1, 2)`
 *
 * memory : arguments is compound node of call args; `argumentCount` cached for
 * performance
 */
typedef struct Xvr_NodeFnCall {
    Xvr_ASTNodeType type;    // XVR_AST_NODE_FN_CALL
    Xvr_ASTNode* arguments;  // argument list
    int argumentCount;       // cached count
} Xvr_NodeFnCall;

void Xvr_emitASTNodeFnReturn(Xvr_ASTNode** nodeHandle, Xvr_ASTNode* returns);

/**
 * @struct Xvr_NodeFnReturn
 * @brief return statement
 *
 * memory: returns is owned
 */
typedef struct Xvr_NodeFnReturn {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_FN_RETURN
    Xvr_ASTNode* returns;  // value to retunr
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

/**
 * @struct Xvr_NodeIf
 * @brief conditional statement: if condition then stmt else stmt
 *
 * memory : condition, thenPath, elsePath
 */
typedef struct Xvr_NodeIf {
    Xvr_ASTNodeType type;    // XVR_AST_NODE_IF
    Xvr_ASTNode* condition;  // boolean expression
    Xvr_ASTNode* thenPath;   // executed if true
    Xvr_ASTNode* elsePath;   // executed if false
} Xvr_NodeIf;

/**
 * @struct Xvr_NodeWhile
 * @brief while loop: while condition stmt
 *
 * memory: condition, thenPath are owned
 */
typedef struct Xvr_NodeWhile {
    Xvr_ASTNodeType type;    // XVR_AST_NODE_WHILE
    Xvr_ASTNode* condition;  // loop condition
    Xvr_ASTNode* thenPath;   // loop body
} Xvr_NodeWhile;

/**
 * @struct Xvr_NodeFor
 * @brief for loop `for pre; condition; post stmt`
 *
 * memory preClause, condition, postClause, thenPath
 */
typedef struct Xvr_NodeFor {
    Xvr_ASTNodeType type;     // XVR_AST_NODE_FOR
    Xvr_ASTNode* preClause;   // intialization
    Xvr_ASTNode* condition;   // loop condition
    Xvr_ASTNode* postClause;  // update
    Xvr_ASTNode* thenPath;    // loop body
} Xvr_NodeFor;

/**
 * @struct Xvr_NodeBreak
 * @brief break statement
 *
 * memory: no children
 */
typedef struct Xvr_NodeBreak {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_BREAK
} Xvr_NodeBreak;

/**
 * @struct Xvr_NodeContinue
 * @brief continue statment: for i in arr { if skip(i) continue; process(i); }
 *
 * memory: no children
 */
typedef struct Xvr_NodeContinue {
    Xvr_ASTNodeType type;  // XVR_AST_NODE_CONTINUE
} Xvr_NodeContinue;

void Xvr_emitASTNodePrefixIncrement(Xvr_ASTNode** nodeHandle,
                                    Xvr_Literal identifier);
void Xvr_emitASTNodePrefixDecrement(Xvr_ASTNode** nodeHandle,
                                    Xvr_Literal identifier);
void Xvr_emitASTNodePostfixIncrement(Xvr_ASTNode** nodeHandle,
                                     Xvr_Literal identifier);
void Xvr_emitASTNodePostfixDecrement(Xvr_ASTNode** nodeHandle,
                                     Xvr_Literal identifier);

/**
 * @struct Xvr_NodePrefixIncrement
 * @brief prefix increment
 *
 * memory: identifier copied
 */
typedef struct Xvr_NodePrefixIncrement {
    Xvr_ASTNodeType type;    // XVR_AST_NODE_PREFIX_INCREMENT
    Xvr_Literal identifier;  // variable name (XVR_LITERAL_IDENTIFIER)
} Xvr_NodePrefixIncrement;

/**
 * @struct Xvr_NodePrefixDecrement
 * @brief prefix decrement
 *
 * example: '--counter'
 * memory: identifier copied
 */
typedef struct Xvr_NodePrefixDecrement {
    Xvr_ASTNodeType type;
    Xvr_Literal identifier;
} Xvr_NodePrefixDecrement;

/**
 * @struct Xvr_NodePostfixIncrement
 * @brief postfix increment
 *
 * example: counter++
 * memory: identifier copied
 */
typedef struct Xvr_NodePostfixIncrement {
    Xvr_ASTNodeType type;    // XVR_AST_NODE_POSTFIX_INCREMENT
    Xvr_Literal identifier;  // variable name
} Xvr_NodePostfixIncrement;

/**
 * @struct Xvr_NodePostfixDecrement
 * @brief postfix decrement
 *
 * example: counter--
 * memory: identifier copied
 */
typedef struct Xvr_NodePostfixDecrement {
    Xvr_ASTNodeType type;    // XVR_AST_NODE_POSTFIX_DECREMENT
    Xvr_Literal identifier;  // variable name
} Xvr_NodePostfixDecrement;

void Xvr_emitASTNodeImport(Xvr_ASTNode** nodeHandle, Xvr_Literal identifier,
                           Xvr_Literal alias);

/**
 * @struct Xvr_NodeImport
 * @brief import statement
 *
 * example: import lib as alias
 * memory: `identifier` `alias` copied
 */
typedef struct Xvr_NodeImport {
    Xvr_ASTNodeType type;
    Xvr_Literal identifier;
    Xvr_Literal alias;
} Xvr_NodeImport;

/**
 *  @brief perform single compiler pass over an AST node,
 *      generate bytecode transform based on the node type
 *
 *      @param nodeaHandle double pointer to AST node being processed
 */
void Xvr_emitASTNodePass(Xvr_ASTNode** nodeHandle);

/**
 * @union Xvr_private_node
 * @brief tagging union representation of AST node
 *
 * @note name `Xvr_private_node` internally preventing from direct use
 */
union Xvr_private_node {
    Xvr_ASTNodeType type;                     // discriminant
    Xvr_NodeLiteral atomic;                   // XVR_AST_NODE_LITERAL
    Xvr_NodeUnary unary;                      // XVR_AST_NODE_UNARY
    Xvr_NodeBinary binary;                    // XVR_AST_NODE_BINARY
    Xvr_NodeTernary ternary;                  // XVR_AST_NODE_TERNARY
    Xvr_NodeGrouping grouping;                // XVR_AST_NODE_GROUPING
    Xvr_NodeBlock block;                      // XVR_AST_NODE_BLOCK
    Xvr_NodeCompound compound;                // XVR_AST_NODE_COMPOUND
    Xvr_NodePair pair;                        // XVR_AST_NODE_PAIR
    Xvr_NodeIndex index;                      // XVR_AST_NODE_INDEX
    Xvr_NodeVarDecl varDecl;                  // XVR_AST_NODE_VAR_DECL
    Xvr_NodeFnCollection fnCollection;        // XVR_AST_NODE_FN_COLLECTION
    Xvr_NodeFnDecl fnDecl;                    // XVR_AST_NODE_FN_DECL
    Xvr_NodeFnCall fnCall;                    // XVR_AST_NODE_FN_CALL
    Xvr_NodeFnReturn returns;                 // XVR_AST_NODE_FN_RETURN
    Xvr_NodeIf pathIf;                        // XVR_AST_NODE_IF
    Xvr_NodeWhile pathWhile;                  // XVR_AST_NODE_WHILE
    Xvr_NodeFor pathFor;                      // XVR_AST_NODE_FOR
    Xvr_NodeBreak pathBreak;                  // XVR_AST_NODE_BREAK
    Xvr_NodeContinue pathContinue;            // XVR_AST_NODE_CONTINUE
    Xvr_NodePrefixIncrement prefixIncrement;  // XVR_AST_NODE_PREFIX_INCREMENT
    Xvr_NodePrefixDecrement prefixDecrement;  // XVR_AST_NODE_PREFIX_DECREMENT
    Xvr_NodePostfixIncrement
        postfixIncrement;  // XVR_AST_NODE_POSTFIX_INCREMENT
    Xvr_NodePostfixDecrement
        postfixDecrement;  // XVR_AST_NODE_POSTFIX_DECREMENT
    Xvr_NodeImport import;
};

/**
 * @brief recursively free AST node and children of AST
 *
 * @param[in, out] node node to destroy (may be NULL)
 *
 * @note safe to call multiple time (idempotent)
 */
XVR_API void Xvr_freeASTNode(Xvr_ASTNode* node);

#endif  // !XVR_AST_NODE_H
