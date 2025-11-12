#include "xvr_parser.h"

#include <stdio.h>

#include "xvr_ast_node.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_literal.h"
#include "xvr_memory.h"
#include "xvr_opcodes.h"
#include "xvr_refstring.h"
#include "xvr_token_types.h"

static void error(Xvr_Parser* parser, Xvr_Token token, const char* message) {
    // keep going while panicing
    if (parser->panic) return;

    fprintf(stderr, XVR_CC_ERROR "[Line %d] Error", token.line);

    // check type
    if (token.type == XVR_TOKEN_EOF) {
        fprintf(stderr, " at end");
    }

    else {
        fprintf(stderr, " at '%.*s'", token.length, token.lexeme);
    }

    // finally
    fprintf(stderr, ": %s\n" XVR_CC_RESET, message);
    parser->error = true;
    parser->panic = true;
}

static void advance(Xvr_Parser* parser) {
    parser->previous = parser->current;
    parser->current = Xvr_scanLexer(parser->lexer);

    if (parser->current.type == XVR_TOKEN_ERROR) {
        error(parser, parser->current, "Xvr_Lexer error");
    }
}

static bool match(Xvr_Parser* parser, Xvr_TokenType tokenType) {
    if (parser->current.type == tokenType) {
        advance(parser);
        return true;
    }
    return false;
}

static void consume(Xvr_Parser* parser, Xvr_TokenType tokenType,
                    const char* msg) {
    if (parser->current.type != tokenType) {
        error(parser, parser->current, msg);
        return;
    }

    advance(parser);
}

static void synchronize(Xvr_Parser* parser) {
#ifndef XVR_EXPORT
    if (Xvr_commandLine.verbose) {
        fprintf(stderr, XVR_CC_ERROR "Synchronizing input\n" XVR_CC_RESET);
    }
#endif

    while (parser->current.type != XVR_TOKEN_EOF) {
        switch (parser->current.type) {
        // these tokens can start a line
        case XVR_TOKEN_ASSERT:
        case XVR_TOKEN_BREAK:
        case XVR_TOKEN_CLASS:
        case XVR_TOKEN_CONTINUE:
        case XVR_TOKEN_DO:
        case XVR_TOKEN_EXPORT:
        case XVR_TOKEN_FOR:
        case XVR_TOKEN_FOREACH:
        case XVR_TOKEN_IF:
        case XVR_TOKEN_IMPORT:
        case XVR_TOKEN_PRINT:
        case XVR_TOKEN_RETURN:
        case XVR_TOKEN_VAR:
        case XVR_TOKEN_WHILE:
            parser->panic = false;
            return;

        default:
            advance(parser);
        }
    }
}

// the pratt table collates the precedence rules
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_TERNARY,
    PREC_OR,
    PREC_AND,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} PrecedenceRule;

typedef Xvr_Opcode (*ParseFn)(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    PrecedenceRule precedence;
} ParseRule;

// no static!
ParseRule parseRules[];

// forward declarations
static void declaration(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle);
static void parsePrecedence(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle,
                            PrecedenceRule rule);
static Xvr_Literal readTypeToLiteral(Xvr_Parser* parser);

// TODO: resolve the messy order of these
// the expression rules
static Xvr_Opcode asType(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_Literal literal = readTypeToLiteral(parser);

    if (!XVR_IS_TYPE(literal)) {
        error(parser, parser->previous, "Expected type after 'astype' keyword");
        Xvr_freeLiteral(literal);
        return XVR_OP_EOF;
    }

    Xvr_emitASTNodeLiteral(nodeHandle, literal);

    Xvr_freeLiteral(literal);

    return XVR_OP_EOF;
}

static Xvr_Opcode typeOf(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* rhs = NULL;
    parsePrecedence(parser, &rhs, PREC_TERNARY);
    Xvr_emitASTNodeUnary(nodeHandle, XVR_OP_TYPE_OF, rhs);
    return XVR_OP_EOF;
}

static Xvr_Opcode compound(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // read either an array or a dictionary into a literal node

    int iterations = 0;  // count the number of entries iterated over

    // compound nodes to store what is read
    Xvr_ASTNode* array = NULL;
    Xvr_ASTNode* dictionary = NULL;

    while (!match(parser, XVR_TOKEN_BRACKET_RIGHT)) {
        // if empty dictionary, there will be a colon between the brackets
        if (iterations == 0 && match(parser, XVR_TOKEN_COLON)) {
            consume(parser, XVR_TOKEN_BRACKET_RIGHT,
                    "Expected ']' at the end of empty dictionary definition");
            // emit an empty dictionary and finish
            Xvr_emitASTNodeCompound(&dictionary, XVR_LITERAL_DICTIONARY);
            break;
        }

        if (iterations > 0) {
            consume(parser, XVR_TOKEN_COMMA,
                    "Expected ',' in array or dictionary");
        }

        iterations++;

        Xvr_ASTNode* left = NULL;
        Xvr_ASTNode* right = NULL;

        // store the left
        parsePrecedence(parser, &left, PREC_PRIMARY);

        if (!left) {  // error
            return XVR_OP_EOF;
        }

        // detect a dictionary
        if (match(parser, XVR_TOKEN_COLON)) {
            parsePrecedence(parser, &right, PREC_PRIMARY);

            if (!right) {  // error
                Xvr_freeASTNode(left);
                return XVR_OP_EOF;
            }

            // check we ARE defining a dictionary
            if (array) {
                error(parser, parser->previous,
                      "Incorrect detection between array and dictionary");
                Xvr_freeASTNode(array);
                return XVR_OP_EOF;
            }

            // init the dictionary
            if (!dictionary) {
                Xvr_emitASTNodeCompound(&dictionary, XVR_LITERAL_DICTIONARY);
            }

            // grow the node if needed
            if (dictionary->compound.capacity <
                dictionary->compound.count + 1) {
                int oldCapacity = dictionary->compound.capacity;

                dictionary->compound.capacity = XVR_GROW_CAPACITY(oldCapacity);
                dictionary->compound.nodes =
                    XVR_GROW_ARRAY(Xvr_ASTNode, dictionary->compound.nodes,
                                   oldCapacity, dictionary->compound.capacity);
            }

            // store the left and right in the node
            Xvr_setASTNodePair(
                &dictionary->compound.nodes[dictionary->compound.count++], left,
                right);
        }
        // detect an array
        else {
            // check we ARE defining an array
            if (dictionary) {
                error(parser, parser->current,
                      "Incorrect detection between array and dictionary");
                Xvr_freeASTNode(dictionary);
                return XVR_OP_EOF;
            }

            // init the array
            if (!array) {
                Xvr_emitASTNodeCompound(&array, XVR_LITERAL_ARRAY);
            }

            // grow the node if needed
            if (array->compound.capacity < array->compound.count + 1) {
                int oldCapacity = array->compound.capacity;

                array->compound.capacity = XVR_GROW_CAPACITY(oldCapacity);
                array->compound.nodes =
                    XVR_GROW_ARRAY(Xvr_ASTNode, array->compound.nodes,
                                   oldCapacity, array->compound.capacity);
            }

            // copy into the array, and manually free the temp node
            array->compound.nodes[array->compound.count++] = *left;
            XVR_FREE(Xvr_ASTNode, left);
        }
    }

    // save the result
    if (array) {
        (*nodeHandle) = array;
    } else if (dictionary) {
        (*nodeHandle) = dictionary;
    } else {
        // both are null, must be an array (because reasons)
        Xvr_emitASTNodeCompound(&array, XVR_LITERAL_ARRAY);
        (*nodeHandle) = array;
    }

    // ignored
    return XVR_OP_EOF;
}

static Xvr_Opcode string(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // handle strings
    switch (parser->previous.type) {
    case XVR_TOKEN_LITERAL_STRING: {
        // unescape valid escaped characters
        int strLength = 0;
        char* buffer = XVR_ALLOCATE(char, parser->previous.length);

        for (int i = 0; i < parser->previous.length; i++) {
            if (parser->previous.lexeme[i] != '\\') {  // copy normally
                buffer[strLength++] = parser->previous.lexeme[i];
                continue;
            }

            // unescape based on the character
            switch (parser->previous.lexeme[++i]) {
            case 'n':
                buffer[strLength++] = '\n';
                break;
            case 't':
                buffer[strLength++] = '\t';
                break;
            case '\\':
                buffer[strLength++] = '\\';
                break;
            case '"':
                buffer[strLength++] = '"';
                break;
            default: {
                char msg[256];
                snprintf(
                    msg, 256,
                    XVR_CC_ERROR
                    "Unrecognized escape character %c in string" XVR_CC_RESET,
                    parser->previous.lexeme[++i]);
                error(parser, parser->previous, msg);
            }
            }
        }

        // for length safety
        if (strLength > XVR_MAX_STRING_LENGTH) {
            strLength = XVR_MAX_STRING_LENGTH;
            char msg[256];
            snprintf(msg, 256,
                     XVR_CC_ERROR
                     "Strings can only be a maximum of %d characters "
                     "long" XVR_CC_RESET,
                     XVR_MAX_STRING_LENGTH);
            error(parser, parser->previous, msg);
        }

        Xvr_Literal literal =
            XVR_TO_STRING_LITERAL(Xvr_createRefStringLength(buffer, strLength));
        XVR_FREE_ARRAY(char, buffer, parser->previous.length);
        Xvr_emitASTNodeLiteral(nodeHandle, literal);
        Xvr_freeLiteral(literal);
        return XVR_OP_EOF;
    }

        // TODO: interpolated strings

    default:
        error(parser, parser->previous,
              "Unexpected token passed to string precedence rule");
        return XVR_OP_EOF;
    }
}

static Xvr_Opcode grouping(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // handle groupings with ()
    switch (parser->previous.type) {
    case XVR_TOKEN_PAREN_LEFT: {
        parsePrecedence(parser, nodeHandle, PREC_TERNARY);
        consume(parser, XVR_TOKEN_PAREN_RIGHT,
                "Expected ')' at end of grouping");

        // process the result without optimisations
        Xvr_emitASTNodeGrouping(nodeHandle);
        return XVR_OP_EOF;
    }

    default:
        error(parser, parser->previous,
              "Unexpected token passed to grouping precedence rule");
        return XVR_OP_EOF;
    }
}

static Xvr_Opcode binary(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    advance(parser);

    // binary() is an infix rule - so only get the RHS of the operator
    switch (parser->previous.type) {
    // arithmetic
    case XVR_TOKEN_PLUS: {
        parsePrecedence(parser, nodeHandle, PREC_TERM);
        return XVR_OP_ADDITION;
    }

    case XVR_TOKEN_MINUS: {
        parsePrecedence(parser, nodeHandle, PREC_TERM);
        return XVR_OP_SUBTRACTION;
    }

    case XVR_TOKEN_MULTIPLY: {
        parsePrecedence(parser, nodeHandle, PREC_FACTOR);
        return XVR_OP_MULTIPLICATION;
    }

    case XVR_TOKEN_DIVIDE: {
        parsePrecedence(parser, nodeHandle, PREC_FACTOR);
        return XVR_OP_DIVISION;
    }

    case XVR_TOKEN_MODULO: {
        parsePrecedence(parser, nodeHandle, PREC_FACTOR);
        return XVR_OP_MODULO;
    }

    // assignment
    case XVR_TOKEN_ASSIGN: {
        parsePrecedence(parser, nodeHandle, PREC_ASSIGNMENT);
        return XVR_OP_VAR_ASSIGN;
    }

    case XVR_TOKEN_PLUS_ASSIGN: {
        parsePrecedence(parser, nodeHandle, PREC_ASSIGNMENT);
        return XVR_OP_VAR_ADDITION_ASSIGN;
    }

    case XVR_TOKEN_MINUS_ASSIGN: {
        parsePrecedence(parser, nodeHandle, PREC_ASSIGNMENT);
        return XVR_OP_VAR_SUBTRACTION_ASSIGN;
    }

    case XVR_TOKEN_MULTIPLY_ASSIGN: {
        parsePrecedence(parser, nodeHandle, PREC_ASSIGNMENT);
        return XVR_OP_VAR_MULTIPLICATION_ASSIGN;
    }

    case XVR_TOKEN_DIVIDE_ASSIGN: {
        parsePrecedence(parser, nodeHandle, PREC_ASSIGNMENT);
        return XVR_OP_VAR_DIVISION_ASSIGN;
    }

    case XVR_TOKEN_MODULO_ASSIGN: {
        parsePrecedence(parser, nodeHandle, PREC_ASSIGNMENT);
        return XVR_OP_VAR_MODULO_ASSIGN;
    }

    // comparison
    case XVR_TOKEN_EQUAL: {
        parsePrecedence(parser, nodeHandle, PREC_COMPARISON);
        return XVR_OP_COMPARE_EQUAL;
    }

    case XVR_TOKEN_NOT_EQUAL: {
        parsePrecedence(parser, nodeHandle, PREC_COMPARISON);
        return XVR_OP_COMPARE_NOT_EQUAL;
    }

    case XVR_TOKEN_LESS: {
        parsePrecedence(parser, nodeHandle, PREC_COMPARISON);
        return XVR_OP_COMPARE_LESS;
    }

    case XVR_TOKEN_LESS_EQUAL: {
        parsePrecedence(parser, nodeHandle, PREC_COMPARISON);
        return XVR_OP_COMPARE_LESS_EQUAL;
    }

    case XVR_TOKEN_GREATER: {
        parsePrecedence(parser, nodeHandle, PREC_COMPARISON);
        return XVR_OP_COMPARE_GREATER;
    }

    case XVR_TOKEN_GREATER_EQUAL: {
        parsePrecedence(parser, nodeHandle, PREC_COMPARISON);
        return XVR_OP_COMPARE_GREATER_EQUAL;
    }

    case XVR_TOKEN_AND: {
        parsePrecedence(parser, nodeHandle, PREC_COMPARISON);
        return XVR_OP_AND;
    }

    case XVR_TOKEN_OR: {
        parsePrecedence(parser, nodeHandle, PREC_COMPARISON);
        return XVR_OP_OR;
    }

    default:
        error(parser, parser->previous,
              "Unexpected token passed to binary precedence rule");
        return XVR_OP_EOF;
    }
}

static Xvr_Opcode unary(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* tmpNode = NULL;

    if (parser->previous.type == XVR_TOKEN_MINUS) {
        // temp handle to potentially negate values
        parsePrecedence(parser, &tmpNode, PREC_TERNARY);  // can be a literal

        // optimisation: check for negative literals
        if (tmpNode != NULL && tmpNode->type == XVR_AST_NODE_LITERAL &&
            (XVR_IS_INTEGER(tmpNode->atomic.literal) ||
             XVR_IS_FLOAT(tmpNode->atomic.literal))) {
            // negate directly, if int or float
            Xvr_Literal lit = tmpNode->atomic.literal;

            if (XVR_IS_INTEGER(lit)) {
                lit = XVR_TO_INTEGER_LITERAL(-XVR_AS_INTEGER(lit));
            }

            if (XVR_IS_FLOAT(lit)) {
                lit = XVR_TO_FLOAT_LITERAL(-XVR_AS_FLOAT(lit));
            }

            tmpNode->atomic.literal = lit;
            *nodeHandle = tmpNode;

            return XVR_OP_EOF;
        }

        // check for negated boolean errors
        if (tmpNode != NULL && tmpNode->type == XVR_AST_NODE_LITERAL &&
            XVR_IS_BOOLEAN(tmpNode->atomic.literal)) {
            error(parser, parser->previous,
                  "Negative booleans are not allowed");
            return XVR_OP_EOF;
        }

        // actually emit the negation node
        Xvr_emitASTNodeUnary(nodeHandle, XVR_OP_NEGATE, tmpNode);
    }

    else if (parser->previous.type == XVR_TOKEN_NOT) {
        // temp handle to potentially negate values
        parsePrecedence(
            parser, &tmpNode,
            PREC_CALL);  // can be a literal, grouping, fn call, etc.

        // optimisation: check for inverted booleans
        if (tmpNode != NULL && tmpNode->type == XVR_AST_NODE_LITERAL &&
            XVR_IS_BOOLEAN(tmpNode->atomic.literal)) {
            // negate directly, if boolean
            Xvr_Literal lit = tmpNode->atomic.literal;

            lit = XVR_TO_BOOLEAN_LITERAL(!XVR_AS_BOOLEAN(lit));

            tmpNode->atomic.literal = lit;
            *nodeHandle = tmpNode;

            return XVR_OP_EOF;
        }

        // actually emit the negation
        Xvr_emitASTNodeUnary(nodeHandle, XVR_OP_INVERT, tmpNode);
    }

    else {
        error(parser, parser->previous,
              "Unexpected token passed to unary precedence rule");
        return XVR_OP_EOF;
    }

    return XVR_OP_EOF;
}

static char* removeChar(const char* lexeme, int length, char c) {
    int resPos = 0;
    char* result = XVR_ALLOCATE(char, length + 1);

    for (int i = 0; i < length; i++) {
        if (lexeme[i] == c) {
            continue;
        }

        result[resPos++] = lexeme[i];
    }

    result[resPos] = '\0';
    return result;
}

static Xvr_Opcode atomic(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    switch (parser->previous.type) {
    case XVR_TOKEN_NULL:
        Xvr_emitASTNodeLiteral(nodeHandle, XVR_TO_NULL_LITERAL);
        return XVR_OP_EOF;

    case XVR_TOKEN_LITERAL_TRUE:
        Xvr_emitASTNodeLiteral(nodeHandle, XVR_TO_BOOLEAN_LITERAL(true));
        return XVR_OP_EOF;

    case XVR_TOKEN_LITERAL_FALSE:
        Xvr_emitASTNodeLiteral(nodeHandle, XVR_TO_BOOLEAN_LITERAL(false));
        return XVR_OP_EOF;

    case XVR_TOKEN_LITERAL_INTEGER: {
        int value = 0;
        const char* lexeme =
            removeChar(parser->previous.lexeme, parser->previous.length, '_');
        sscanf(lexeme, "%d", &value);
        XVR_FREE_ARRAY(char, lexeme, parser->previous.length + 1);
        Xvr_emitASTNodeLiteral(nodeHandle, XVR_TO_INTEGER_LITERAL(value));
        return XVR_OP_EOF;
    }

    case XVR_TOKEN_LITERAL_FLOAT: {
        float value = 0;
        const char* lexeme =
            removeChar(parser->previous.lexeme, parser->previous.length, '_');
        sscanf(lexeme, "%f", &value);
        XVR_FREE_ARRAY(char, lexeme, parser->previous.length + 1);
        Xvr_emitASTNodeLiteral(nodeHandle, XVR_TO_FLOAT_LITERAL(value));
        return XVR_OP_EOF;
    }

    case XVR_TOKEN_TYPE: {
        if (match(parser, XVR_TOKEN_CONST)) {
            Xvr_emitASTNodeLiteral(nodeHandle,
                                   XVR_TO_TYPE_LITERAL(XVR_LITERAL_TYPE, true));
        } else {
            Xvr_emitASTNodeLiteral(
                nodeHandle, XVR_TO_TYPE_LITERAL(XVR_LITERAL_TYPE, false));
        }

        return XVR_OP_EOF;
    }

    default:
        error(parser, parser->previous,
              "Unexpected token passed to atomic precedence rule");
        return XVR_OP_EOF;
    }
}

static Xvr_Opcode identifier(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // make a copy of the string
    Xvr_Token identifierToken = parser->previous;

    if (identifierToken.type != XVR_TOKEN_IDENTIFIER) {
        error(parser, parser->previous, "Expected identifier");
        return XVR_OP_EOF;
    }

    int length = identifierToken.length;

    // for safety
    if (length > 256) {
        length = 256;
        error(parser, parser->previous,
              "Identifiers can only be a maximum of 256 characters long");
    }

    Xvr_Literal identifier = XVR_TO_IDENTIFIER_LITERAL(
        Xvr_createRefStringLength(identifierToken.lexeme, length));
    Xvr_emitASTNodeLiteral(nodeHandle, identifier);
    Xvr_freeLiteral(identifier);

    return XVR_OP_EOF;
}

static Xvr_Opcode castingPrefix(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    switch (parser->previous.type) {
    case XVR_TOKEN_BOOLEAN: {
        Xvr_Literal literal = XVR_TO_TYPE_LITERAL(XVR_LITERAL_BOOLEAN, false);
        Xvr_emitASTNodeLiteral(nodeHandle, literal);
        Xvr_freeLiteral(literal);
    } break;

    case XVR_TOKEN_INTEGER: {
        Xvr_Literal literal = XVR_TO_TYPE_LITERAL(XVR_LITERAL_INTEGER, false);
        Xvr_emitASTNodeLiteral(nodeHandle, literal);
        Xvr_freeLiteral(literal);
    } break;

    case XVR_TOKEN_FLOAT: {
        Xvr_Literal literal = XVR_TO_TYPE_LITERAL(XVR_LITERAL_FLOAT, false);
        Xvr_emitASTNodeLiteral(nodeHandle, literal);
        Xvr_freeLiteral(literal);
    } break;

    case XVR_TOKEN_STRING: {
        Xvr_Literal literal = XVR_TO_TYPE_LITERAL(XVR_LITERAL_STRING, false);
        Xvr_emitASTNodeLiteral(nodeHandle, literal);
        Xvr_freeLiteral(literal);
    } break;

    default:
        error(parser, parser->previous,
              "Unexpected token passed to casting precedence rule");
        return XVR_OP_EOF;
    }

    return XVR_OP_EOF;
}

static Xvr_Opcode castingInfix(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    advance(parser);

    // NOTE: using the precedence rules here
    switch (parser->previous.type) {
    case XVR_TOKEN_IDENTIFIER:
        identifier(parser, nodeHandle);
        break;

    case XVR_TOKEN_LITERAL_TRUE:
    case XVR_TOKEN_LITERAL_FALSE:
        atomic(parser, nodeHandle);
        break;

    case XVR_TOKEN_LITERAL_INTEGER:
        atomic(parser, nodeHandle);
        break;

    case XVR_TOKEN_LITERAL_FLOAT:
        atomic(parser, nodeHandle);
        break;

    case XVR_TOKEN_LITERAL_STRING:
        atomic(parser, nodeHandle);
        break;

    default:
        error(parser, parser->previous,
              "Unexpected token passed to casting infix precedence rule");
        return XVR_OP_EOF;
    }

    return XVR_OP_TYPE_CAST;
}

// TODO: fix these screwy names
static Xvr_Opcode incrementPrefix(Xvr_Parser* parser,
                                  Xvr_ASTNode** nodeHandle) {
    advance(parser);

    Xvr_ASTNode* tmpNode = NULL;
    identifier(parser, &tmpNode);

    Xvr_emitASTNodePrefixIncrement(nodeHandle, tmpNode->atomic.literal);

    Xvr_freeASTNode(tmpNode);

    return XVR_OP_EOF;
}

static Xvr_Opcode incrementInfix(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* tmpNode = NULL;
    identifier(parser, &tmpNode);

    advance(parser);

    Xvr_emitASTNodePostfixIncrement(nodeHandle, tmpNode->atomic.literal);

    Xvr_freeASTNode(tmpNode);

    return XVR_OP_EOF;
}

static Xvr_Opcode decrementPrefix(Xvr_Parser* parser,
                                  Xvr_ASTNode** nodeHandle) {
    advance(parser);

    Xvr_ASTNode* tmpNode = NULL;
    identifier(parser, &tmpNode);  // weird

    Xvr_emitASTNodePrefixDecrement(nodeHandle, tmpNode->atomic.literal);

    Xvr_freeASTNode(tmpNode);

    return XVR_OP_EOF;
}

static Xvr_Opcode decrementInfix(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* tmpNode = NULL;
    identifier(parser, &tmpNode);

    advance(parser);

    Xvr_emitASTNodePostfixDecrement(nodeHandle, tmpNode->atomic.literal);

    Xvr_freeASTNode(tmpNode);

    return XVR_OP_EOF;
}

static Xvr_Opcode fnCall(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    advance(parser);  // skip the left paren

    // binary() is an infix rule - so only get the RHS of the operator
    switch (parser->previous.type) {
    // arithmetic
    case XVR_TOKEN_PAREN_LEFT: {
        Xvr_ASTNode* arguments = NULL;
        Xvr_emitASTNodeFnCollection(&arguments);

        // if there's arguments
        if (!match(parser, XVR_TOKEN_PAREN_RIGHT)) {
            // read each argument
            do {
                // emit the node to the argument list (grow the node if needed)
                if (arguments->fnCollection.capacity <
                    arguments->fnCollection.count + 1) {
                    int oldCapacity = arguments->fnCollection.capacity;

                    arguments->fnCollection.capacity =
                        XVR_GROW_CAPACITY(oldCapacity);
                    arguments->fnCollection.nodes = XVR_GROW_ARRAY(
                        Xvr_ASTNode, arguments->fnCollection.nodes, oldCapacity,
                        arguments->fnCollection.capacity);
                }

                Xvr_ASTNode* tmpNode = NULL;
                parsePrecedence(parser, &tmpNode, PREC_TERNARY);

                if (!tmpNode) {
                    error(parser, parser->previous,
                          "[internal] No token found in fnCall");
                    return XVR_OP_EOF;
                }

                arguments->fnCollection.nodes[arguments->fnCollection.count++] =
                    *tmpNode;
                XVR_FREE(Xvr_ASTNode, tmpNode);  // simply free the tmpNode, so
                                                 // you don't free the children
            } while (match(parser, XVR_TOKEN_COMMA));

            consume(parser, XVR_TOKEN_PAREN_RIGHT,
                    "Expected ')' at end of argument list");
        }

        // emit the call
        Xvr_emitASTNodeFnCall(nodeHandle, arguments);

        return XVR_OP_FN_CALL;
    } break;

    default:
        error(parser, parser->previous,
              "Unexpected token passed to function call precedence rule");
        return XVR_OP_EOF;
    }

    return XVR_OP_EOF;
}

static Xvr_Opcode indexAccess(
    Xvr_Parser* parser,
    Xvr_ASTNode** nodeHandle) {  // TODO: fix indexing signalling
    advance(parser);

    // val[first : second : third]

    Xvr_ASTNode* first = NULL;
    Xvr_ASTNode* second = NULL;
    Xvr_ASTNode* third = NULL;

    // booleans indicate blank slice indexing
    Xvr_emitASTNodeLiteral(&first, XVR_TO_INDEX_BLANK_LITERAL);
    Xvr_emitASTNodeLiteral(&second, XVR_TO_INDEX_BLANK_LITERAL);
    Xvr_emitASTNodeLiteral(&third, XVR_TO_INDEX_BLANK_LITERAL);

    bool readFirst = false;  // pattern matching is bullcrap

    // eat the first
    if (!match(parser, XVR_TOKEN_COLON)) {
        Xvr_freeASTNode(first);
        parsePrecedence(parser, &first, PREC_TERNARY);
        match(parser, XVR_TOKEN_COLON);
        readFirst = true;
    }

    if (match(parser, XVR_TOKEN_BRACKET_RIGHT)) {
        if (readFirst) {
            Xvr_freeASTNode(second);
            second = NULL;
        }

        Xvr_freeASTNode(third);
        third = NULL;

        Xvr_emitASTNodeIndex(nodeHandle, first, second, third);
        return XVR_OP_INDEX;
    }

    // eat the second
    if (!match(parser, XVR_TOKEN_COLON)) {
        Xvr_freeASTNode(second);
        parsePrecedence(parser, &second, PREC_TERNARY);
        match(parser, XVR_TOKEN_COLON);
    }

    if (match(parser, XVR_TOKEN_BRACKET_RIGHT)) {
        Xvr_freeASTNode(third);
        third = NULL;
        Xvr_emitASTNodeIndex(nodeHandle, first, second, third);
        return XVR_OP_INDEX;
    }

    // eat the third
    Xvr_freeASTNode(third);
    parsePrecedence(parser, &third, PREC_TERNARY);
    Xvr_emitASTNodeIndex(nodeHandle, first, second, third);

    consume(parser, XVR_TOKEN_BRACKET_RIGHT, "Expected ']' in index notation");

    return XVR_OP_INDEX;
}

static Xvr_Opcode question(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    advance(parser);  // for the question mark

    Xvr_ASTNode* thenPath = NULL;
    Xvr_ASTNode* elsePath = NULL;

    parsePrecedence(parser, &thenPath, PREC_TERNARY);
    consume(parser, XVR_TOKEN_COLON, "Expected ':' in ternary expression");
    parsePrecedence(parser, &elsePath, PREC_TERNARY);

    Xvr_emitASTNodeTernary(nodeHandle, NULL, thenPath, elsePath);

    return XVR_OP_TERNARY;
}

static Xvr_Opcode dot(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    advance(parser);  // for the dot

    Xvr_ASTNode* tmpNode = NULL;
    parsePrecedence(parser, &tmpNode, PREC_CALL);

    if (tmpNode == NULL || tmpNode->binary.right == NULL) {
        error(parser, parser->previous,
              "Expected function call after dot operator");
        return XVR_OP_EOF;
    }

    (*nodeHandle) = tmpNode;
    return XVR_OP_DOT;  // signal that the function name and arguments are in
                        // the wrong order
}

ParseRule parseRules[] = {
    // must match the token types
    // types
    {atomic, NULL, PREC_PRIMARY},      // TOKEN_NULL,
    {castingPrefix, NULL, PREC_CALL},  // TOKEN_BOOLEAN,
    {castingPrefix, NULL, PREC_CALL},  // TOKEN_INTEGER,
    {castingPrefix, NULL, PREC_CALL},  // TOKEN_FLOAT,
    {castingPrefix, NULL, PREC_CALL},  // TOKEN_STRING,
    {NULL, NULL, PREC_NONE},           // TOKEN_ARRAY,
    {NULL, NULL, PREC_NONE},           // TOKEN_DICTIONARY,
    {NULL, NULL, PREC_NONE},           // TOKEN_FUNCTION,
    {NULL, NULL, PREC_NONE},           // TOKEN_OPAQUE,
    {NULL, NULL, PREC_NONE},           // TOKEN_ANY,

    // keywords and reserved words
    {NULL, NULL, PREC_NONE},       // TOKEN_AS,
    {NULL, NULL, PREC_NONE},       // TOKEN_ASSERT,
    {NULL, NULL, PREC_NONE},       // TOKEN_BREAK,
    {NULL, NULL, PREC_NONE},       // TOKEN_CLASS,
    {NULL, NULL, PREC_NONE},       // TOKEN_CONST,
    {NULL, NULL, PREC_NONE},       // TOKEN_CONTINUE,
    {NULL, NULL, PREC_NONE},       // TOKEN_DO,
    {NULL, NULL, PREC_NONE},       // TOKEN_ELSE,
    {NULL, NULL, PREC_NONE},       // TOKEN_EXPORT,
    {NULL, NULL, PREC_NONE},       // TOKEN_FOR,
    {NULL, NULL, PREC_NONE},       // TOKEN_FOREACH,
    {NULL, NULL, PREC_NONE},       // TOKEN_IF,
    {NULL, NULL, PREC_NONE},       // TOKEN_IMPORT,
    {NULL, NULL, PREC_NONE},       // TOKEN_IN,
    {NULL, NULL, PREC_NONE},       // TOKEN_OF,
    {NULL, NULL, PREC_NONE},       // TOKEN_PRINT,
    {NULL, NULL, PREC_NONE},       // TOKEN_RETURN,
    {atomic, NULL, PREC_PRIMARY},  // TOKEN_TYPE,
    {asType, NULL, PREC_CALL},     // TOKEN_ASTYPE,
    {typeOf, NULL, PREC_CALL},     // TOKEN_TYPEOF,
    {NULL, NULL, PREC_NONE},       // TOKEN_VAR,
    {NULL, NULL, PREC_NONE},       // TOKEN_WHILE,

    // literal values
    {identifier, castingInfix, PREC_PRIMARY},  // TOKEN_IDENTIFIER,
    {atomic, castingInfix, PREC_PRIMARY},      // TOKEN_LITERAL_TRUE,
    {atomic, castingInfix, PREC_PRIMARY},      // TOKEN_LITERAL_FALSE,
    {atomic, castingInfix, PREC_PRIMARY},      // TOKEN_LITERAL_INTEGER,
    {atomic, castingInfix, PREC_PRIMARY},      // TOKEN_LITERAL_FLOAT,
    {string, castingInfix, PREC_PRIMARY},      // TOKEN_LITERAL_STRING,

    // math operators
    {NULL, binary, PREC_TERM},                     // TOKEN_PLUS,
    {unary, binary, PREC_TERM},                    // TOKEN_MINUS,
    {NULL, binary, PREC_FACTOR},                   // TOKEN_MULTIPLY,
    {NULL, binary, PREC_FACTOR},                   // TOKEN_DIVIDE,
    {NULL, binary, PREC_FACTOR},                   // TOKEN_MODULO,
    {NULL, binary, PREC_ASSIGNMENT},               // TOKEN_PLUS_ASSIGN,
    {NULL, binary, PREC_ASSIGNMENT},               // TOKEN_MINUS_ASSIGN,
    {NULL, binary, PREC_ASSIGNMENT},               // TOKEN_MULTIPLY_ASSIGN,
    {NULL, binary, PREC_ASSIGNMENT},               // TOKEN_DIVIDE_ASSIGN,
    {NULL, binary, PREC_ASSIGNMENT},               // TOKEN_MODULO_ASSIGN,
    {incrementPrefix, incrementInfix, PREC_CALL},  // TOKEN_PLUS_PLUS,
    {decrementPrefix, decrementInfix, PREC_CALL},  // TOKEN_MINUS_MINUS,
    {NULL, binary, PREC_ASSIGNMENT},               // TOKEN_ASSIGN,

    // logical operators
    {grouping, fnCall, PREC_CALL},       // TOKEN_PAREN_LEFT,
    {NULL, NULL, PREC_NONE},             // TOKEN_PAREN_RIGHT,
    {compound, indexAccess, PREC_CALL},  // TOKEN_BRACKET_LEFT,
    {NULL, NULL, PREC_NONE},             // TOKEN_BRACKET_RIGHT,
    {NULL, NULL, PREC_NONE},             // TOKEN_BRACE_LEFT,
    {NULL, NULL, PREC_NONE},             // TOKEN_BRACE_RIGHT,
    {unary, NULL, PREC_CALL},            // TOKEN_NOT,
    {NULL, binary, PREC_COMPARISON},     // TOKEN_NOT_EQUAL,
    {NULL, binary, PREC_COMPARISON},     // TOKEN_EQUAL,
    {NULL, binary, PREC_COMPARISON},     // TOKEN_LESS,
    {NULL, binary, PREC_COMPARISON},     // TOKEN_GREATER,
    {NULL, binary, PREC_COMPARISON},     // TOKEN_LESS_EQUAL,
    {NULL, binary, PREC_COMPARISON},     // TOKEN_GREATER_EQUAL,
    {NULL, binary, PREC_AND},            // TOKEN_AND,
    {NULL, binary, PREC_OR},             // TOKEN_OR,

    // other operators
    {NULL, question, PREC_TERNARY},  // TOKEN_QUESTION,
    {NULL, NULL, PREC_NONE},         // TOKEN_COLON,
    {NULL, NULL, PREC_NONE},         // TOKEN_SEMICOLON,
    {NULL, NULL, PREC_NONE},         // TOKEN_COMMA,
    {NULL, dot, PREC_CALL},          // TOKEN_DOT,
    {NULL, NULL, PREC_NONE},         // TOKEN_PIPE,
    {NULL, NULL, PREC_NONE},         // TOKEN_REST,

    // meta tokens
    {NULL, NULL, PREC_NONE},  // TOKEN_PASS,
    {NULL, NULL, PREC_NONE},  // TOKEN_ERROR,
    {NULL, NULL, PREC_NONE},  // TOKEN_EOF,
};

ParseRule* getRule(Xvr_TokenType type) { return &parseRules[type]; }

// optimisation: constant folding
static bool calcStaticBinaryArithmetic(Xvr_Parser* parser,
                                       Xvr_ASTNode** nodeHandle) {
    switch ((*nodeHandle)->binary.opcode) {
    case XVR_OP_ADDITION:
    case XVR_OP_SUBTRACTION:
    case XVR_OP_MULTIPLICATION:
    case XVR_OP_DIVISION:
    case XVR_OP_MODULO:
    case XVR_OP_COMPARE_EQUAL:
    case XVR_OP_COMPARE_NOT_EQUAL:
    case XVR_OP_COMPARE_LESS:
    case XVR_OP_COMPARE_LESS_EQUAL:
    case XVR_OP_COMPARE_GREATER:
    case XVR_OP_COMPARE_GREATER_EQUAL:
        break;

    default:
        return true;
    }

    // recurse to the left and right
    if ((*nodeHandle)->binary.left->type == XVR_AST_NODE_BINARY) {
        calcStaticBinaryArithmetic(parser, &(*nodeHandle)->binary.left);
    }

    if ((*nodeHandle)->binary.right->type == XVR_AST_NODE_BINARY) {
        calcStaticBinaryArithmetic(parser, &(*nodeHandle)->binary.right);
    }

    // make sure left and right are both literals
    if (!((*nodeHandle)->binary.left->type == XVR_AST_NODE_LITERAL &&
          (*nodeHandle)->binary.right->type == XVR_AST_NODE_LITERAL)) {
        return true;
    }

    // evaluate
    Xvr_Literal lhs = (*nodeHandle)->binary.left->atomic.literal;
    Xvr_Literal rhs = (*nodeHandle)->binary.right->atomic.literal;
    Xvr_Literal result = XVR_TO_NULL_LITERAL;

    // special case for string concatenation ONLY
    if (XVR_IS_STRING(lhs) && XVR_IS_STRING(rhs) &&
        (*nodeHandle)->binary.opcode == XVR_OP_ADDITION) {
        // check for overflow
        int totalLength =
            XVR_AS_STRING(lhs)->length + XVR_AS_STRING(rhs)->length;
        if (totalLength > XVR_MAX_STRING_LENGTH) {
            error(parser, parser->previous,
                  "Can't concatenate these strings, result is too long (error "
                  "found in constant folding)\n");
            return false;
        }

        // concat the strings
        char buffer[XVR_MAX_STRING_LENGTH];
        snprintf(buffer, XVR_MAX_STRING_LENGTH, "%s%s",
                 Xvr_toCString(XVR_AS_STRING(lhs)),
                 Xvr_toCString(XVR_AS_STRING(rhs)));
        result = XVR_TO_STRING_LITERAL(
            Xvr_createRefStringLength(buffer, totalLength));
    }

    // type coersion
    if (XVR_IS_FLOAT(lhs) && XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    if (XVR_IS_INTEGER(lhs) && XVR_IS_FLOAT(rhs)) {
        lhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(lhs));
    }

    // maths based on types
    if (XVR_IS_INTEGER(lhs) && XVR_IS_INTEGER(rhs)) {
        switch ((*nodeHandle)->binary.opcode) {
        case XVR_OP_ADDITION:
            result = XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) +
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_SUBTRACTION:
            result = XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) -
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_MULTIPLICATION:
            result = XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) *
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_DIVISION:
            if (XVR_AS_INTEGER(rhs) == 0) {
                error(parser, parser->previous,
                      "Can't divide by zero (error found in constant folding)");
                return false;
            }
            result = XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) /
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_MODULO:
            if (XVR_AS_INTEGER(rhs) == 0) {
                error(parser, parser->previous,
                      "Can't modulo by zero (error found in constant folding)");
                return false;
            }
            result = XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) %
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_COMPARE_EQUAL:
            result = XVR_TO_BOOLEAN_LITERAL(XVR_AS_INTEGER(lhs) ==
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_COMPARE_NOT_EQUAL:
            result = XVR_TO_BOOLEAN_LITERAL(XVR_AS_INTEGER(lhs) !=
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_COMPARE_LESS:
            result = XVR_TO_BOOLEAN_LITERAL(XVR_AS_INTEGER(lhs) <
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_COMPARE_LESS_EQUAL:
            result = XVR_TO_BOOLEAN_LITERAL(XVR_AS_INTEGER(lhs) <=
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_COMPARE_GREATER:
            result = XVR_TO_BOOLEAN_LITERAL(XVR_AS_INTEGER(lhs) >
                                            XVR_AS_INTEGER(rhs));
            break;

        case XVR_OP_COMPARE_GREATER_EQUAL:
            result = XVR_TO_BOOLEAN_LITERAL(XVR_AS_INTEGER(lhs) >=
                                            XVR_AS_INTEGER(rhs));
            break;

        default:
            error(parser, parser->previous,
                  "[internal] bad opcode argument passed to "
                  "calcStaticBinaryArithmetic()");
            return false;
        }
    }

    // catch bad modulo
    if ((XVR_IS_FLOAT(lhs) || XVR_IS_FLOAT(rhs)) &&
        (*nodeHandle)->binary.opcode == XVR_OP_MODULO) {
        error(parser, parser->previous,
              "Bad arithmetic argument (modulo on floats not allowed)");
        return false;
    }

    if (XVR_IS_FLOAT(lhs) && XVR_IS_FLOAT(rhs)) {
        switch ((*nodeHandle)->binary.opcode) {
        case XVR_OP_ADDITION:
            result =
                XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) + XVR_AS_FLOAT(rhs));
            break;

        case XVR_OP_SUBTRACTION:
            result =
                XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) - XVR_AS_FLOAT(rhs));
            break;

        case XVR_OP_MULTIPLICATION:
            result =
                XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) * XVR_AS_FLOAT(rhs));
            break;

        case XVR_OP_DIVISION:
            if (XVR_AS_FLOAT(rhs) == 0) {
                error(parser, parser->previous,
                      "Can't divide by zero (error found in constant folding)");
                return false;
            }
            result =
                XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) / XVR_AS_FLOAT(rhs));
            break;

        case XVR_OP_COMPARE_EQUAL:
            result =
                XVR_TO_BOOLEAN_LITERAL(XVR_AS_FLOAT(lhs) == XVR_AS_FLOAT(rhs));
            break;

        case XVR_OP_COMPARE_NOT_EQUAL:
            result =
                XVR_TO_BOOLEAN_LITERAL(XVR_AS_FLOAT(lhs) != XVR_AS_FLOAT(rhs));
            break;

        case XVR_OP_COMPARE_LESS:
            result =
                XVR_TO_BOOLEAN_LITERAL(XVR_AS_FLOAT(lhs) < XVR_AS_FLOAT(rhs));
            break;

        case XVR_OP_COMPARE_LESS_EQUAL:
            result =
                XVR_TO_BOOLEAN_LITERAL(XVR_AS_FLOAT(lhs) <= XVR_AS_FLOAT(rhs));
            break;

        case XVR_OP_COMPARE_GREATER:
            result =
                XVR_TO_BOOLEAN_LITERAL(XVR_AS_FLOAT(lhs) > XVR_AS_FLOAT(rhs));
            break;

        case XVR_OP_COMPARE_GREATER_EQUAL:
            result =
                XVR_TO_BOOLEAN_LITERAL(XVR_AS_FLOAT(lhs) >= XVR_AS_FLOAT(rhs));
            break;

        default:
            error(parser, parser->previous,
                  "[internal] bad opcode argument passed to "
                  "calcStaticBinaryArithmetic()");
            return false;
        }
    }

    // nothing can be done to optimize
    if (XVR_IS_NULL(result)) {
        return true;
    }

    // optimize by converting this node into a literal node
    Xvr_freeASTNode((*nodeHandle)->binary.left);
    Xvr_freeASTNode((*nodeHandle)->binary.right);

    (*nodeHandle)->type = XVR_AST_NODE_LITERAL;
    (*nodeHandle)->atomic.literal = result;

    return true;
}

static void dottify(
    Xvr_Parser* parser,
    Xvr_ASTNode** nodeHandle) {  // TODO: remove dot from the compiler entirely
    // only if this is chained from a higher binary "fn call"
    if ((*nodeHandle)->type == XVR_AST_NODE_BINARY) {
        if ((*nodeHandle)->binary.opcode == XVR_OP_FN_CALL) {
            (*nodeHandle)->binary.opcode = XVR_OP_DOT;
            (*nodeHandle)->binary.right->fnCall.argumentCount++;
        }
        dottify(parser, &(*nodeHandle)->binary.left);
        dottify(parser, &(*nodeHandle)->binary.right);
    }
}

static void parsePrecedence(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle,
                            PrecedenceRule rule) {
    // every valid expression has a prefix rule
    advance(parser);
    ParseFn prefixRule = getRule(parser->previous.type)->prefix;

    if (prefixRule == NULL) {
        *nodeHandle =
            NULL;  // the handle's value MUST be set to null for error handling
        error(parser, parser->previous, "Expected expression");
        return;
    }

    bool canBeAssigned = rule <= PREC_ASSIGNMENT;
    prefixRule(parser, nodeHandle);  // ignore the returned opcode

    // infix rules are left-recursive
    while (rule <= getRule(parser->current.type)->precedence) {
        ParseFn infixRule = getRule(parser->current.type)->infix;

        if (infixRule == NULL) {
            *nodeHandle = NULL;  // the handle's value MUST be set to null for
                                 // error handling
            error(parser, parser->current, "Expected operator");
            return;
        }

        Xvr_ASTNode* rhsNode = NULL;
        const Xvr_Opcode opcode = infixRule(
            parser, &rhsNode);  // NOTE: infix rule must advance the parser

        if (opcode == XVR_OP_EOF) {
            Xvr_freeASTNode(*nodeHandle);
            *nodeHandle = rhsNode;
            return;  // we're done here
        }

        if (opcode == XVR_OP_DOT) {
            dottify(parser, &rhsNode);
        }

        if (opcode == XVR_OP_TERNARY) {
            rhsNode->ternary.condition = *nodeHandle;
            *nodeHandle = rhsNode;
            continue;
        }

        Xvr_emitASTNodeBinary(nodeHandle, rhsNode, opcode);

        // optimise away the constants
        if (!parser->panic && !calcStaticBinaryArithmetic(parser, nodeHandle)) {
            return;
        }
    }

    // if your precedence is below "assignment"
    if (canBeAssigned && match(parser, XVR_TOKEN_ASSIGN)) {
        error(parser, parser->current, "Invalid assignment target");
    }
}

// expressions
static void expression(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // delegate to the pratt table for expression precedence
    parsePrecedence(parser, nodeHandle, PREC_ASSIGNMENT);
}

// statements
static void blockStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // init
    Xvr_emitASTNodeBlock(nodeHandle);

    // sub-scope, compile it and push it up in a node
    while (!match(parser, XVR_TOKEN_BRACE_RIGHT)) {
        if ((*nodeHandle)->block.capacity < (*nodeHandle)->block.count + 1) {
            int oldCapacity = (*nodeHandle)->block.capacity;

            (*nodeHandle)->block.capacity = XVR_GROW_CAPACITY(oldCapacity);
            (*nodeHandle)->block.nodes =
                XVR_GROW_ARRAY(Xvr_ASTNode, (*nodeHandle)->block.nodes,
                               oldCapacity, (*nodeHandle)->block.capacity);
        }

        Xvr_ASTNode* tmpNode = NULL;

        // process the grammar rule for this line
        declaration(parser, &tmpNode);

        if (parser->panic) {
            return;
        }

        ((*nodeHandle)->block.nodes[(*nodeHandle)->block.count++]) = *tmpNode;
        XVR_FREE(Xvr_ASTNode, tmpNode);  // simply free the tmpNode, so you
                                         // don't free the children
    }
}

static void printStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // set the node info
    Xvr_ASTNode* node = NULL;
    expression(parser, &node);
    Xvr_emitASTNodeUnary(nodeHandle, XVR_OP_PRINT, node);

    consume(parser, XVR_TOKEN_SEMICOLON,
            "Expected ';' at end of print statement");
}

static void assertStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // set the node info
    (*nodeHandle) =
        XVR_ALLOCATE(Xvr_ASTNode, 1);  // special case, because I'm lazy
    (*nodeHandle)->type = XVR_AST_NODE_BINARY;
    (*nodeHandle)->binary.opcode = XVR_OP_ASSERT;

    parsePrecedence(parser, &((*nodeHandle)->binary.left), PREC_TERNARY);
    consume(parser, XVR_TOKEN_COMMA, "Expected ',' in assert statement");
    parsePrecedence(parser, &((*nodeHandle)->binary.right), PREC_TERNARY);

    consume(parser, XVR_TOKEN_SEMICOLON,
            "Expected ';' at end of assert statement");
}

static void ifStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* condition = NULL;
    Xvr_ASTNode* thenPath = NULL;
    Xvr_ASTNode* elsePath = NULL;

    // read the condition
    consume(parser, XVR_TOKEN_PAREN_LEFT,
            "Expected '(' at beginning of if clause");
    parsePrecedence(parser, &condition, PREC_TERNARY);

    // read the then path
    consume(parser, XVR_TOKEN_PAREN_RIGHT, "Expected ')' at end of if clause");
    declaration(parser, &thenPath);

    // read the optional else path
    if (match(parser, XVR_TOKEN_ELSE)) {
        declaration(parser, &elsePath);
    }

    Xvr_emitASTNodeIf(nodeHandle, condition, thenPath, elsePath);
}

static void whileStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* condition = NULL;
    Xvr_ASTNode* thenPath = NULL;

    // read the condition
    consume(parser, XVR_TOKEN_PAREN_LEFT,
            "Expected '(' at beginning of while clause");
    parsePrecedence(parser, &condition, PREC_TERNARY);

    // read the then path
    consume(parser, XVR_TOKEN_PAREN_RIGHT,
            "Expected ')' at end of while clause");
    declaration(parser, &thenPath);

    Xvr_emitASTNodeWhile(nodeHandle, condition, thenPath);
}

static void forStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* preClause = NULL;
    Xvr_ASTNode* condition = NULL;
    Xvr_ASTNode* postClause = NULL;
    Xvr_ASTNode* thenPath = NULL;

    // read the clauses
    consume(parser, XVR_TOKEN_PAREN_LEFT,
            "Expected '(' at beginning of for clause");

    declaration(parser,
                &preClause);  // allow defining variables in the pre-clause

    parsePrecedence(parser, &condition, PREC_TERNARY);
    consume(parser, XVR_TOKEN_SEMICOLON,
            "Expected ';' after condition of for clause");

    parsePrecedence(parser, &postClause, PREC_ASSIGNMENT);
    consume(parser, XVR_TOKEN_PAREN_RIGHT, "Expected ')' at end of for clause");

    // read the path
    declaration(parser, &thenPath);

    Xvr_emitASTNodeFor(nodeHandle, preClause, condition, postClause, thenPath);
}

static void breakStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_emitASTNodeBreak(nodeHandle);

    consume(parser, XVR_TOKEN_SEMICOLON,
            "Expected ';' at end of break statement");
}

static void continueStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_emitASTNodeContinue(nodeHandle);

    consume(parser, XVR_TOKEN_SEMICOLON,
            "Expected ';' at end of continue statement");
}

static void returnStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    Xvr_ASTNode* returnValues = NULL;
    Xvr_emitASTNodeFnCollection(&returnValues);

    if (!match(parser, XVR_TOKEN_SEMICOLON)) {
        do {  // loop for multiple returns (disabled later in the pipeline)
            // append the node to the return list (grow the node if needed)
            if (returnValues->fnCollection.capacity <
                returnValues->fnCollection.count + 1) {
                int oldCapacity = returnValues->fnCollection.capacity;

                returnValues->fnCollection.capacity =
                    XVR_GROW_CAPACITY(oldCapacity);
                returnValues->fnCollection.nodes = XVR_GROW_ARRAY(
                    Xvr_ASTNode, returnValues->fnCollection.nodes, oldCapacity,
                    returnValues->fnCollection.capacity);
            }

            Xvr_ASTNode* node = NULL;
            parsePrecedence(parser, &node, PREC_TERNARY);

            if (!node) {
                error(parser, parser->previous,
                      "[internal] No token found in return");
                return;
            }

            returnValues->fnCollection
                .nodes[returnValues->fnCollection.count++] = *node;
            XVR_FREE(Xvr_ASTNode, node);  // free manually
        } while (match(parser, XVR_TOKEN_COMMA));

        consume(parser, XVR_TOKEN_SEMICOLON,
                "Expected ';' at end of return statement");
    }

    Xvr_emitASTNodeFnReturn(nodeHandle, returnValues);
}

static void importStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // read the identifier
    Xvr_ASTNode* node = NULL;
    advance(parser);
    identifier(parser, &node);

    if (node == NULL) {
        return;
    }

    Xvr_Literal idn = Xvr_copyLiteral(node->atomic.literal);
    Xvr_freeASTNode(node);

    Xvr_Literal alias = XVR_TO_NULL_LITERAL;

    if (match(parser, XVR_TOKEN_AS)) {
        Xvr_ASTNode* node = NULL;
        advance(parser);
        identifier(parser, &node);
        alias = Xvr_copyLiteral(node->atomic.literal);
        Xvr_freeASTNode(node);
    }

    Xvr_emitASTNodeImport(nodeHandle, idn, alias);

    consume(parser, XVR_TOKEN_SEMICOLON,
            "Expected ';' at end of import statement");

    Xvr_freeLiteral(idn);
    Xvr_freeLiteral(alias);
}

// precedence functions
static void expressionStmt(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    if (match(parser, XVR_TOKEN_SEMICOLON)) {
        Xvr_emitASTNodeLiteral(nodeHandle, XVR_TO_NULL_LITERAL);
        return;
    }

    Xvr_ASTNode* ptr = NULL;
    expression(parser, &ptr);

    if (ptr != NULL) {
        *nodeHandle = ptr;
    }

    consume(parser, XVR_TOKEN_SEMICOLON,
            "Expected ';' at the end of expression statement");
}

static void statement(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // block
    if (match(parser, XVR_TOKEN_BRACE_LEFT)) {
        blockStmt(parser, nodeHandle);
        return;
    }

    // print
    if (match(parser, XVR_TOKEN_PRINT)) {
        printStmt(parser, nodeHandle);
        return;
    }

    // assert
    if (match(parser, XVR_TOKEN_ASSERT)) {
        assertStmt(parser, nodeHandle);
        return;
    }

    // if-then-else
    if (match(parser, XVR_TOKEN_IF)) {
        ifStmt(parser, nodeHandle);
        return;
    }

    // while-then
    if (match(parser, XVR_TOKEN_WHILE)) {
        whileStmt(parser, nodeHandle);
        return;
    }

    // for-pre-clause-post-then
    if (match(parser, XVR_TOKEN_FOR)) {
        forStmt(parser, nodeHandle);
        return;
    }

    // break
    if (match(parser, XVR_TOKEN_BREAK)) {
        breakStmt(parser, nodeHandle);
        return;
    }

    // continue
    if (match(parser, XVR_TOKEN_CONTINUE)) {
        continueStmt(parser, nodeHandle);
        return;
    }

    // return
    if (match(parser, XVR_TOKEN_RETURN)) {
        returnStmt(parser, nodeHandle);
        return;
    }

    // import
    if (match(parser, XVR_TOKEN_IMPORT)) {
        importStmt(parser, nodeHandle);
        return;
    }

    // default
    expressionStmt(parser, nodeHandle);
}

// declarations and definitions
static Xvr_Literal readTypeToLiteral(Xvr_Parser* parser) {
    advance(parser);

    Xvr_Literal literal = XVR_TO_TYPE_LITERAL(XVR_LITERAL_NULL, false);

    switch (parser->previous.type) {
    case XVR_TOKEN_NULL:
        // NO-OP
        break;

    case XVR_TOKEN_BOOLEAN:
        XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_BOOLEAN;
        break;

    case XVR_TOKEN_INTEGER:
        XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_INTEGER;
        break;

    case XVR_TOKEN_FLOAT:
        XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_FLOAT;
        break;

    case XVR_TOKEN_STRING:
        XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_STRING;
        break;

    // array, dictionary - read the sub-types
    case XVR_TOKEN_BRACKET_LEFT: {
        Xvr_Literal l = readTypeToLiteral(parser);

        if (match(parser, XVR_TOKEN_COLON)) {
            Xvr_Literal r = readTypeToLiteral(parser);

            XVR_TYPE_PUSH_SUBTYPE(&literal, l);
            XVR_TYPE_PUSH_SUBTYPE(&literal, r);

            XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_DICTIONARY;
        } else {
            XVR_TYPE_PUSH_SUBTYPE(&literal, l);

            XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_ARRAY;
        }

        consume(parser, XVR_TOKEN_BRACKET_RIGHT,
                "Expected ']' at end of type definition");
    } break;

    case XVR_TOKEN_FUNCTION:
        XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_FUNCTION;
        break;

    case XVR_TOKEN_OPAQUE:
        XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_OPAQUE;
        break;

    case XVR_TOKEN_ANY:
        XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_ANY;
        break;

    // wtf
    case XVR_TOKEN_IDENTIFIER: {
        // duplicated from identifier()
        Xvr_Token identifierToken = parser->previous;
        int length = identifierToken.length;
        // for safety
        if (length > 256) {
            length = 256;
            error(parser, parser->previous,
                  "Identifiers can only be a maximum of 256 characters long");
        }
        literal = XVR_TO_IDENTIFIER_LITERAL(
            Xvr_createRefStringLength(identifierToken.lexeme, length));
    } break;

    // WTF
    case XVR_TOKEN_TYPE:
        XVR_AS_TYPE(literal).typeOf = XVR_LITERAL_TYPE;
        break;

    default:
        error(parser, parser->previous, "Bad type signature");
        return XVR_TO_NULL_LITERAL;
    }

    // const follows the type
    if (match(parser, XVR_TOKEN_CONST)) {
        XVR_AS_TYPE(literal).constant = true;
    }

    return literal;
}

static void varDecl(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // read the identifier
    consume(parser, XVR_TOKEN_IDENTIFIER,
            "Expected identifier after var keyword");
    Xvr_Token identifierToken = parser->previous;

    int length = identifierToken.length;

    // for safety
    if (length > 256) {
        length = 256;
        error(parser, parser->previous,
              "Identifiers can only be a maximum of 256 characters long");
    }

    Xvr_Literal identifier = XVR_TO_IDENTIFIER_LITERAL(
        Xvr_createRefStringLength(identifierToken.lexeme, length));

    // read the type, if present
    Xvr_Literal typeLiteral;
    if (match(parser, XVR_TOKEN_COLON)) {
        typeLiteral = readTypeToLiteral(parser);
    } else {
        // default to non-const any
        typeLiteral = XVR_TO_TYPE_LITERAL(XVR_LITERAL_ANY, false);
    }

    // variable definition is an expression
    Xvr_ASTNode* expressionNode = NULL;
    if (match(parser, XVR_TOKEN_ASSIGN)) {
        expression(parser, &expressionNode);
    } else {
        // values are null by default
        Xvr_emitASTNodeLiteral(&expressionNode, XVR_TO_NULL_LITERAL);
    }

    // TODO: static type checking?

    // declare it
    Xvr_emitASTNodeVarDecl(nodeHandle, identifier, typeLiteral, expressionNode);

    consume(parser, XVR_TOKEN_SEMICOLON,
            "Expected ';' at end of var declaration");
}

static void fnDecl(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    // read the identifier
    consume(parser, XVR_TOKEN_IDENTIFIER,
            "Expected identifier after fn keyword");
    Xvr_Token identifierToken = parser->previous;

    int length = identifierToken.length;

    // for safety
    if (length > 256) {
        length = 256;
        error(parser, parser->previous,
              "Identifiers can only be a maximum of 256 characters long");
    }

    Xvr_Literal identifier = XVR_TO_IDENTIFIER_LITERAL(
        Xvr_createRefStringLength(identifierToken.lexeme, length));

    // read the parameters and arity
    consume(parser, XVR_TOKEN_PAREN_LEFT,
            "Expected '(' after function identifier");

    // for holding the array of arguments
    Xvr_ASTNode* argumentNode = NULL;
    Xvr_emitASTNodeFnCollection(&argumentNode);

    // read args
    if (!match(parser, XVR_TOKEN_PAREN_RIGHT)) {
        do {
            // check for rest parameter
            if (match(parser, XVR_TOKEN_REST)) {
                // read the argument identifier
                consume(parser, XVR_TOKEN_IDENTIFIER,
                        "Expected identifier as function argument");
                Xvr_Token argIdentifierToken = parser->previous;

                int length = argIdentifierToken.length;

                // for safety
                if (length > 256) {
                    length = 256;
                    error(parser, parser->previous,
                          "Identifiers can only be a maximum of 256 characters "
                          "long");
                }

                Xvr_Literal argIdentifier =
                    XVR_TO_IDENTIFIER_LITERAL(Xvr_createRefStringLength(
                        argIdentifierToken.lexeme, length));

                // set the type (array of any types)
                Xvr_Literal argTypeLiteral =
                    XVR_TO_TYPE_LITERAL(XVR_LITERAL_FUNCTION_ARG_REST, false);

                // emit the node to the argument list (grow the node if needed)
                if (argumentNode->fnCollection.capacity <
                    argumentNode->fnCollection.count + 1) {
                    int oldCapacity = argumentNode->fnCollection.capacity;

                    argumentNode->fnCollection.capacity =
                        XVR_GROW_CAPACITY(oldCapacity);
                    argumentNode->fnCollection.nodes = XVR_GROW_ARRAY(
                        Xvr_ASTNode, argumentNode->fnCollection.nodes,
                        oldCapacity, argumentNode->fnCollection.capacity);
                }

                // store the arg in the array
                Xvr_ASTNode* literalNode = NULL;
                Xvr_emitASTNodeVarDecl(&literalNode, argIdentifier,
                                       argTypeLiteral, NULL);

                argumentNode->fnCollection
                    .nodes[argumentNode->fnCollection.count++] = *literalNode;
                XVR_FREE(Xvr_ASTNode, literalNode);

                break;
            }

            // read the argument identifier
            consume(parser, XVR_TOKEN_IDENTIFIER,
                    "Expected identifier as function argument");
            Xvr_Token argIdentifierToken = parser->previous;

            int length = argIdentifierToken.length;

            // for safety
            if (length > 256) {
                length = 256;
                error(
                    parser, parser->previous,
                    "Identifiers can only be a maximum of 256 characters long");
            }

            Xvr_Literal argIdentifier = XVR_TO_IDENTIFIER_LITERAL(
                Xvr_createRefStringLength(argIdentifierToken.lexeme, length));

            // read optional type of the identifier
            Xvr_Literal argTypeLiteral;
            if (match(parser, XVR_TOKEN_COLON)) {
                argTypeLiteral = readTypeToLiteral(parser);
            } else {
                // default to non-const any
                argTypeLiteral = XVR_TO_TYPE_LITERAL(XVR_LITERAL_ANY, false);
            }

            // emit the node to the argument list (grow the node if needed)
            if (argumentNode->fnCollection.capacity <
                argumentNode->fnCollection.count + 1) {
                int oldCapacity = argumentNode->fnCollection.capacity;

                argumentNode->fnCollection.capacity =
                    XVR_GROW_CAPACITY(oldCapacity);
                argumentNode->fnCollection.nodes = XVR_GROW_ARRAY(
                    Xvr_ASTNode, argumentNode->fnCollection.nodes, oldCapacity,
                    argumentNode->fnCollection.capacity);
            }

            // store the arg in the array
            Xvr_ASTNode* literalNode = NULL;
            Xvr_emitASTNodeVarDecl(&literalNode, argIdentifier, argTypeLiteral,
                                   NULL);

            argumentNode->fnCollection
                .nodes[argumentNode->fnCollection.count++] = *literalNode;
            XVR_FREE(Xvr_ASTNode, literalNode);

        } while (match(parser, XVR_TOKEN_COMMA));  // if comma is read, continue

        consume(parser, XVR_TOKEN_PAREN_RIGHT,
                "Expected ')' after function argument list");
    }

    // read the return types, if present
    Xvr_ASTNode* returnNode = NULL;
    Xvr_emitASTNodeFnCollection(&returnNode);

    if (match(parser, XVR_TOKEN_COLON)) {
        do {
            if (returnNode->fnCollection.capacity <
                returnNode->fnCollection.count + 1) {
                int oldCapacity = returnNode->fnCollection.capacity;

                returnNode->fnCollection.capacity =
                    XVR_GROW_CAPACITY(oldCapacity);
                returnNode->fnCollection.nodes = XVR_GROW_ARRAY(
                    Xvr_ASTNode, returnNode->fnCollection.nodes, oldCapacity,
                    returnNode->fnCollection.capacity);
            }

            Xvr_ASTNode* literalNode = NULL;
            Xvr_emitASTNodeLiteral(&literalNode, readTypeToLiteral(parser));

            returnNode->fnCollection.nodes[returnNode->fnCollection.count++] =
                *literalNode;
            XVR_FREE(Xvr_ASTNode, literalNode);
        } while (match(parser, XVR_TOKEN_COMMA));
    }

    // read the function body
    consume(parser, XVR_TOKEN_BRACE_LEFT, "Expected '{' after return list");

    Xvr_ASTNode* blockNode = NULL;
    blockStmt(parser, &blockNode);

    // declare it
    Xvr_emitASTNodeFnDecl(nodeHandle, identifier, argumentNode, returnNode,
                          blockNode);
}

static void declaration(Xvr_Parser* parser, Xvr_ASTNode** nodeHandle) {
    if (match(parser, XVR_TOKEN_VAR)) {
        varDecl(parser, nodeHandle);
    } else if (match(parser, XVR_TOKEN_FUNCTION)) {
        fnDecl(parser, nodeHandle);
    } else {
        statement(parser, nodeHandle);
    }
}

// exposed functions
void Xvr_initParser(Xvr_Parser* parser, Xvr_Lexer* lexer) {
    parser->lexer = lexer;
    parser->error = false;
    parser->panic = false;

    parser->previous.type = XVR_TOKEN_NULL;
    parser->current.type = XVR_TOKEN_NULL;
    advance(parser);
}

void Xvr_freeParser(Xvr_Parser* parser) {
    parser->lexer = NULL;
    parser->error = false;
    parser->panic = false;

    parser->previous.type = XVR_TOKEN_NULL;
    parser->current.type = XVR_TOKEN_NULL;
}

Xvr_ASTNode* Xvr_scanParser(Xvr_Parser* parser) {
    // check for EOF
    if (match(parser, XVR_TOKEN_EOF)) {
        return NULL;
    }

    // returns nodes on the heap
    Xvr_ASTNode* node = NULL;

    // process the grammar rule for this line
    declaration(parser, &node);

    if (parser->panic) {
        synchronize(parser);
        // return an error node for this iteration
        Xvr_freeASTNode(node);
        node = XVR_ALLOCATE(Xvr_ASTNode, 1);
        node->type = XVR_AST_NODE_ERROR;
    }

    return node;
}
