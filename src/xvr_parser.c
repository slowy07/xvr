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

#include "xvr_parser.h"

#include <stddef.h>
#include <stdio.h>

#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_string.h"
#include "xvr_token_types.h"
#include "xvr_value.h"

static void printError(Xvr_Parser* parser, Xvr_Token token,
                       const char* errorMsg) {
    // keep going while panicking
    if (parser->panic) {
        return;
    }

    fprintf(stderr, XVR_CC_ERROR "[Line %d] Error ", (int)token.line);

    // check type
    if (token.type == XVR_TOKEN_EOF) {
        fprintf(stderr, "at end");
    } else {
        fprintf(stderr, "at '%.*s'", (int)token.length, token.lexeme);
    }

    // finally
    fprintf(stderr, ": %s\n" XVR_CC_RESET, errorMsg);
    parser->error = true;
    parser->panic = true;
}

static void advance(Xvr_Parser* parser) {
    parser->previous = parser->current;
    parser->current = Xvr_private_scanLexer(parser->lexer);

    if (parser->current.type == XVR_TOKEN_ERROR) {
        printError(parser, parser->current, "Can't read the source code");
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
        printError(parser, parser->current, msg);
        return;
    }

    advance(parser);
}

static void synchronize(Xvr_Parser* parser) {
    while (parser->current.type != XVR_TOKEN_EOF) {
        switch (parser->current.type) {
        case XVR_TOKEN_KEYWORD_ASSERT:
        case XVR_TOKEN_KEYWORD_BREAK:
        case XVR_TOKEN_KEYWORD_CLASS:
        case XVR_TOKEN_KEYWORD_CONTINUE:
        case XVR_TOKEN_KEYWORD_DO:
        case XVR_TOKEN_KEYWORD_EXPORT:
        case XVR_TOKEN_KEYWORD_FOR:
        case XVR_TOKEN_KEYWORD_FOREACH:
        case XVR_TOKEN_KEYWORD_FUNCTION:
        case XVR_TOKEN_KEYWORD_IF:
        case XVR_TOKEN_KEYWORD_IMPORT:
        case XVR_TOKEN_KEYWORD_PRINT:
        case XVR_TOKEN_KEYWORD_RETURN:
        case XVR_TOKEN_KEYWORD_VAR:
        case XVR_TOKEN_KEYWORD_WHILE:
        case XVR_TOKEN_KEYWORD_YIELD:
            parser->error = true;
            parser->panic = false;
            return;

        default:
            advance(parser);
        }
    }
}

// precedence declarations
typedef enum ParsingPrecedence {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_GROUP,
    PREC_TERNARY,
    PREC_NEGATE,
    PREC_OR,
    PREC_AND,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY,
} ParsingPrecedence;

typedef Xvr_AstFlag (*ParsingRule)(Xvr_Bucket** bucketHandle,
                                   Xvr_Parser* parser, Xvr_Ast** rootHandle);

typedef struct ParsingTuple {
    ParsingPrecedence precedence;
    ParsingRule prefix;
    ParsingRule infix;
} ParsingTuple;

static void parsePrecedence(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                            Xvr_Ast** rootHandle, ParsingPrecedence precRule);

static Xvr_AstFlag nameString(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                              Xvr_Ast** rootHandle);
static Xvr_AstFlag literal(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                           Xvr_Ast** rootHandle);
static Xvr_AstFlag unary(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                         Xvr_Ast** rootHandle);
static Xvr_AstFlag binary(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                          Xvr_Ast** rootHandle);
static Xvr_AstFlag group(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                         Xvr_Ast** rootHandle);
static Xvr_AstFlag compound(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                            Xvr_Ast** rootHandle);
static Xvr_AstFlag aggregate(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                             Xvr_Ast** rootHandle);
static Xvr_AstFlag unaryPostfix(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                                Xvr_Ast** rootHandle);

static ParsingTuple parsingRulesetTable[] = {
    {PREC_PRIMARY, literal, NULL},  // XVR_TOKEN_NULL,

    {PREC_PRIMARY, nameString, NULL},  // XVR_TOKEN_NAME,

    // types
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_TYPE_BOOLEAN,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_TYPE_INTEGER,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_TYPE_FLOAT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_TYPE_STRING,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_TYPE_ARRAY,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_TYPE_TABLE,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_TYPE_FUNCTION,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_TYPE_OPAQUE,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_TYPE_ANY,

    // keywords and reserved words
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_AS,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_ASSERT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_BREAK,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_CLASS,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_CONST,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_CONTINUE,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_DO,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_ELSE,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_EXPORT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_FOR,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_FOREACH,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_FUNCTION,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_IF,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_IMPORT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_IN,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_OF,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_PASS,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_PRINT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_RETURN,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_VAR,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_WHILE,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_KEYWORD_YIELD,

    // literal values
    {PREC_PRIMARY, literal, NULL},  // XVR_TOKEN_LITERAL_TRUE,
    {PREC_PRIMARY, literal, NULL},  // XVR_TOKEN_LITERAL_FALSE,
    {PREC_PRIMARY, literal, NULL},  // XVR_TOKEN_LITERAL_INTEGER,
    {PREC_PRIMARY, literal, NULL},  // XVR_TOKEN_LITERAL_FLOAT,
    {PREC_PRIMARY, literal, NULL},  // XVR_TOKEN_LITERAL_STRING,

    // math operators
    {PREC_TERM, NULL, binary},         // XVR_TOKEN_OPERATOR_ADD,
    {PREC_TERM, unary, binary},        // XVR_TOKEN_OPERATOR_SUBTRACT,
    {PREC_FACTOR, NULL, binary},       // XVR_TOKEN_OPERATOR_MULTIPLY,
    {PREC_FACTOR, NULL, binary},       // XVR_TOKEN_OPERATOR_DIVIDE,
    {PREC_FACTOR, NULL, binary},       // XVR_TOKEN_OPERATOR_MODULO,
    {PREC_ASSIGNMENT, NULL, binary},   // XVR_TOKEN_OPERATOR_ADD_ASSIGN,
    {PREC_ASSIGNMENT, NULL, binary},   // XVR_TOKEN_OPERATOR_SUBTRACT_ASSIGN,
    {PREC_ASSIGNMENT, NULL, binary},   // XVR_TOKEN_OPERATOR_MULTIPLY_ASSIGN,
    {PREC_ASSIGNMENT, NULL, binary},   // XVR_TOKEN_OPERATOR_DIVIDE_ASSIGN,
    {PREC_ASSIGNMENT, NULL, binary},   // XVR_TOKEN_OPERATOR_MODULO_ASSIGN,
    {PREC_CALL, unary, unaryPostfix},  // XVR_TOKEN_OPERATOR_INCREMENT,
    {PREC_CALL, unary, unaryPostfix},  // XVR_TOKEN_OPERATOR_DECREMENT,
    {PREC_ASSIGNMENT, NULL, binary},   // XVR_TOKEN_OPERATOR_ASSIGN,

    // comparator operators
    {PREC_COMPARISON, NULL, binary},  // XVR_TOKEN_OPERATOR_COMPARE_EQUAL,
    {PREC_COMPARISON, NULL, binary},  // XVR_TOKEN_OPERATOR_COMPARE_NOT,
    {PREC_COMPARISON, NULL, binary},  // XVR_TOKEN_OPERATOR_COMPARE_LESS,
    {PREC_COMPARISON, NULL, binary},  // XVR_TOKEN_OPERATOR_COMPARE_LESS_EQUAL,
    {PREC_COMPARISON, NULL, binary},  // XVR_TOKEN_OPERATOR_COMPARE_GREATER,
    {PREC_COMPARISON, NULL,
     binary},  // XVR_TOKEN_OPERATOR_COMPARE_GREATER_EQUAL,

    // structural operators
    {PREC_GROUP, group, NULL},          // XVR_TOKEN_OPERATOR_PAREN_LEFT,
    {PREC_NONE, NULL, NULL},            // XVR_TOKEN_OPERATOR_PAREN_RIGHT,
    {PREC_GROUP, compound, aggregate},  // XVR_TOKEN_OPERATOR_BRACKET_LEFT,
    {PREC_NONE, compound, aggregate},   // XVR_TOKEN_OPERATOR_BRACKET_RIGHT,
    {PREC_NONE, NULL, NULL},            // XVR_TOKEN_OPERATOR_BRACE_LEFT,
    {PREC_NONE, NULL, NULL},            // XVR_TOKEN_OPERATOR_BRACE_RIGHT,

    // other operators
    {PREC_AND, NULL, binary},           // XVR_TOKEN_OPERATOR_AND,
    {PREC_OR, NULL, binary},            // XVR_TOKEN_OPERATOR_OR,
    {PREC_NEGATE, unary, NULL},         // XVR_TOKEN_OPERATOR_NEGATE,
    {PREC_NONE, NULL, NULL},            // XVR_TOKEN_OPERATOR_QUESTION,
    {PREC_GROUP, compound, aggregate},  // XVR_TOKEN_OPERATOR_COLON,

    {PREC_NONE, NULL, NULL},        // XVR_TOKEN_OPERATOR_SEMICOLON, // ;
    {PREC_GROUP, NULL, aggregate},  // XVR_TOKEN_OPERATOR_COMMA, // ,

    {PREC_NONE, NULL, NULL},    // XVR_TOKEN_OPERATOR_DOT, // .
    {PREC_CALL, NULL, binary},  // XVR_TOKEN_OPERATOR_CONCAT, // ..
    {PREC_NONE, NULL, NULL},    // XVR_TOKEN_OPERATOR_REST, // ...

    // unused operators
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_AMPERSAND, // &
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_PIPE, // |

    // meta tokens
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_ERROR,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_EOF,
};

static ParsingTuple* getParsingRule(Xvr_TokenType type) {
    return &parsingRulesetTable[type];
}

static Xvr_ValueType readType(Xvr_Parser* parser) {
    advance(parser);

    switch (parser->previous.type) {
    case XVR_TOKEN_TYPE_BOOLEAN:
        return XVR_VALUE_BOOLEAN;

    case XVR_TOKEN_TYPE_INTEGER:
        return XVR_VALUE_INTEGER;

    case XVR_TOKEN_TYPE_FLOAT:
        return XVR_VALUE_FLOAT;

    case XVR_TOKEN_TYPE_STRING:
        return XVR_VALUE_STRING;

    case XVR_TOKEN_TYPE_ARRAY:
        return XVR_VALUE_ARRAY;

    case XVR_TOKEN_TYPE_TABLE:
        return XVR_VALUE_TABLE;

    case XVR_TOKEN_TYPE_FUNCTION:
        return XVR_VALUE_FUNCTION;

    case XVR_TOKEN_TYPE_OPAQUE:
        return XVR_VALUE_OPAQUE;

    case XVR_TOKEN_TYPE_ANY:
        return XVR_VALUE_ANY;

    default:
        printError(parser, parser->previous, "Expected type identifier");
        return XVR_VALUE_UNKNOWN;
    }
}

static Xvr_AstFlag nameString(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                              Xvr_Ast** rootHandle) {
    Xvr_String* name = Xvr_createNameStringLength(
        bucketHandle, parser->previous.lexeme, parser->previous.length,
        XVR_VALUE_UNKNOWN, false);
    Xvr_Value value = XVR_VALUE_FROM_STRING(name);
    Xvr_private_emitAstValue(bucketHandle, rootHandle, value);

    Xvr_AstFlag flag = XVR_AST_FLAG_NONE;

    if (match(parser, XVR_TOKEN_OPERATOR_ASSIGN)) {
        flag = XVR_AST_FLAG_ASSIGN;
    } else if (match(parser, XVR_TOKEN_OPERATOR_ADD_ASSIGN)) {
        flag = XVR_AST_FLAG_ADD_ASSIGN;
    } else if (match(parser, XVR_TOKEN_OPERATOR_SUBTRACT_ASSIGN)) {
        flag = XVR_AST_FLAG_SUBTRACT_ASSIGN;
    } else if (match(parser, XVR_TOKEN_OPERATOR_MULTIPLY_ASSIGN)) {
        flag = XVR_AST_FLAG_MULTIPLY_ASSIGN;
    } else if (match(parser, XVR_TOKEN_OPERATOR_DIVIDE_ASSIGN)) {
        flag = XVR_AST_FLAG_DIVIDE_ASSIGN;
    } else if (match(parser, XVR_TOKEN_OPERATOR_MODULO_ASSIGN)) {
        flag = XVR_AST_FLAG_MODULO_ASSIGN;
    }

    if (flag != XVR_AST_FLAG_NONE) {
        Xvr_Ast* expr = NULL;
        parsePrecedence(bucketHandle, parser, &expr, PREC_ASSIGNMENT);
        Xvr_private_emitAstVariableAssignment(bucketHandle, rootHandle, flag,
                                              expr);
        return XVR_AST_FLAG_NONE;
    }

    Xvr_private_emitAstVariableAccess(bucketHandle, rootHandle);
    return XVR_AST_FLAG_NONE;
}

static Xvr_AstFlag literal(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                           Xvr_Ast** rootHandle) {
    switch (parser->previous.type) {
    case XVR_TOKEN_NULL:
        Xvr_private_emitAstValue(bucketHandle, rootHandle,
                                 XVR_VALUE_FROM_NULL());
        return XVR_AST_FLAG_NONE;

    case XVR_TOKEN_LITERAL_TRUE:
        Xvr_private_emitAstValue(bucketHandle, rootHandle,
                                 XVR_VALUE_FROM_BOOLEAN(true));
        return XVR_AST_FLAG_NONE;

    case XVR_TOKEN_LITERAL_FALSE:
        Xvr_private_emitAstValue(bucketHandle, rootHandle,
                                 XVR_VALUE_FROM_BOOLEAN(false));
        return XVR_AST_FLAG_NONE;

    case XVR_TOKEN_LITERAL_INTEGER: {
        char buffer[parser->previous.length];

        unsigned int i = 0, o = 0;
        do {
            buffer[i] = parser->previous.lexeme[o];
            if (buffer[i] != '_') i++;
        } while (parser->previous.lexeme[o++] && i < parser->previous.length);
        buffer[i] = '\0';

        int value = 0;
        sscanf(buffer, "%d", &value);
        Xvr_private_emitAstValue(bucketHandle, rootHandle,
                                 XVR_VALUE_FROM_INTEGER(value));
        return XVR_AST_FLAG_NONE;
    }

    case XVR_TOKEN_LITERAL_FLOAT: {
        char buffer[parser->previous.length];

        unsigned int i = 0, o = 0;
        do {
            buffer[i] = parser->previous.lexeme[o];
            if (buffer[i] != '_') i++;
        } while (parser->previous.lexeme[o++] && i < parser->previous.length);
        buffer[i] = '\0';  // BUGFIX

        float value = 0;
        sscanf(buffer, "%f", &value);
        Xvr_private_emitAstValue(bucketHandle, rootHandle,
                                 XVR_VALUE_FROM_FLOAT(value));
        return XVR_AST_FLAG_NONE;
    }

    case XVR_TOKEN_LITERAL_STRING: {
        char buffer[parser->previous.length + 1];
        unsigned int escapeCounter = 0;
        unsigned int i = 0, o = 0;

        if (parser->previous.length > 0) {
            do {
                buffer[i] = parser->previous.lexeme[o];
                if (buffer[i] == '\\' && parser->previous.lexeme[++o]) {
                    escapeCounter++;

                    switch (parser->previous.lexeme[o]) {
                    case 'n':
                        buffer[i] = '\n';
                        break;
                    case 't':
                        buffer[i] = '\t';
                        break;
                    case '\\':
                        buffer[i] = '\\';
                        break;
                    case '"':
                        buffer[i] = '"';
                        break;
                    }
                }
                i++;
            } while (parser->previous.lexeme[o++] &&
                     i < parser->previous.length);
        }

        buffer[i] = '\0';
        unsigned int len = i - escapeCounter;
        Xvr_private_emitAstValue(bucketHandle, rootHandle,
                                 XVR_VALUE_FROM_STRING(Xvr_createStringLength(
                                     bucketHandle, buffer, len)));

        return XVR_AST_FLAG_NONE;
    }

    default:
        printError(parser, parser->previous,
                   "Unexpected token passed to literal precedence rule");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return XVR_AST_FLAG_NONE;
    }
}

static Xvr_AstFlag unary(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                         Xvr_Ast** rootHandle) {
    if (parser->previous.type == XVR_TOKEN_OPERATOR_SUBTRACT) {
        bool connectedDigit =
            parser->previous.lexeme[1] >= '0' &&
            parser->previous.lexeme[1] <=
                '9';  // BUGFIX: '- 1' should not be optimised into a negative
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_UNARY);

        // negative numbers
        if ((*rootHandle)->type == XVR_AST_VALUE &&
            XVR_VALUE_IS_INTEGER((*rootHandle)->value.value) &&
            connectedDigit) {
            (*rootHandle)->value.value = XVR_VALUE_FROM_INTEGER(
                -XVR_VALUE_AS_INTEGER((*rootHandle)->value.value));
        } else if ((*rootHandle)->type == XVR_AST_VALUE &&
                   XVR_VALUE_IS_FLOAT((*rootHandle)->value.value) &&
                   connectedDigit) {
            (*rootHandle)->value.value = XVR_VALUE_FROM_FLOAT(
                -XVR_VALUE_AS_FLOAT((*rootHandle)->value.value));
        } else {
            Xvr_private_emitAstUnary(bucketHandle, rootHandle,
                                     XVR_AST_FLAG_NEGATE);
        }
    }

    else if (parser->previous.type == XVR_TOKEN_OPERATOR_NEGATE) {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_UNARY);
        Xvr_private_emitAstUnary(bucketHandle, rootHandle, XVR_AST_FLAG_NEGATE);
    }

    else if (parser->previous.type == XVR_TOKEN_OPERATOR_INCREMENT ||
             parser->previous.type == XVR_TOKEN_OPERATOR_DECREMENT) {
        Xvr_AstFlag flag = parser->previous.type == XVR_TOKEN_OPERATOR_INCREMENT
                               ? XVR_AST_FLAG_PREFIX_INCREMENT
                               : XVR_AST_FLAG_PREFIX_DECREMENT;
        Xvr_Ast* primary = NULL;

        parsePrecedence(bucketHandle, parser, &primary, PREC_PRIMARY);

        if (primary->type != XVR_AST_VAR_ACCESS ||
            primary->varAccess.child->type != XVR_AST_VALUE ||
            XVR_VALUE_IS_STRING(primary->varAccess.child->value.value) !=
                true ||
            XVR_VALUE_AS_STRING(primary->varAccess.child->value.value)
                    ->info.type != XVR_STRING_NAME) {
            printError(
                parser, parser->previous,
                "Unexpected non-name-string token in unary-prefixs operator "
                "increment precedence rule");
            Xvr_private_emitAstError(bucketHandle, rootHandle);
        } else {
            *rootHandle = primary->varAccess.child;
            Xvr_private_emitAstUnary(bucketHandle, rootHandle, flag);
        }
    }

    else {
        printError(parser, parser->previous,
                   "Unexpected token passed to unary precedence rule");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
    }

    return XVR_AST_FLAG_NONE;
}

static Xvr_AstFlag binary(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                          Xvr_Ast** rootHandle) {
    // infix must advance
    advance(parser);

    switch (parser->previous.type) {
    // arithmetic
    case XVR_TOKEN_OPERATOR_ADD: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_TERM + 1);
        return XVR_AST_FLAG_ADD;
    }

    case XVR_TOKEN_OPERATOR_SUBTRACT: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_TERM + 1);
        return XVR_AST_FLAG_SUBTRACT;
    }

    case XVR_TOKEN_OPERATOR_MULTIPLY: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_FACTOR + 1);
        return XVR_AST_FLAG_MULTIPLY;
    }

    case XVR_TOKEN_OPERATOR_DIVIDE: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_FACTOR + 1);
        return XVR_AST_FLAG_DIVIDE;
    }

    case XVR_TOKEN_OPERATOR_MODULO: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_FACTOR + 1);
        return XVR_AST_FLAG_MODULO;
    }

    case XVR_TOKEN_OPERATOR_ASSIGN: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_ASSIGNMENT + 1);
        return XVR_AST_FLAG_ASSIGN;
    }

    case XVR_TOKEN_OPERATOR_ADD_ASSIGN: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_ASSIGNMENT + 1);
        return XVR_AST_FLAG_ADD_ASSIGN;
    }

    case XVR_TOKEN_OPERATOR_SUBTRACT_ASSIGN: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_ASSIGNMENT + 1);
        return XVR_AST_FLAG_SUBTRACT_ASSIGN;
    }

    case XVR_TOKEN_OPERATOR_MULTIPLY_ASSIGN: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_ASSIGNMENT + 1);
        return XVR_AST_FLAG_MULTIPLY_ASSIGN;
    }

    case XVR_TOKEN_OPERATOR_DIVIDE_ASSIGN: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_ASSIGNMENT + 1);
        return XVR_AST_FLAG_DIVIDE_ASSIGN;
    }

    case XVR_TOKEN_OPERATOR_MODULO_ASSIGN: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_ASSIGNMENT + 1);
        return XVR_AST_FLAG_MODULO_ASSIGN;
    }

    case XVR_TOKEN_OPERATOR_COMPARE_EQUAL: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_COMPARISON + 1);
        return XVR_AST_FLAG_COMPARE_EQUAL;
    }

    case XVR_TOKEN_OPERATOR_COMPARE_NOT: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_COMPARISON + 1);
        return XVR_AST_FLAG_COMPARE_NOT;
    }

    case XVR_TOKEN_OPERATOR_COMPARE_LESS: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_COMPARISON + 1);
        return XVR_AST_FLAG_COMPARE_LESS;
    }

    case XVR_TOKEN_OPERATOR_COMPARE_LESS_EQUAL: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_COMPARISON + 1);
        return XVR_AST_FLAG_COMPARE_LESS_EQUAL;
    }

    case XVR_TOKEN_OPERATOR_COMPARE_GREATER: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_COMPARISON + 1);
        return XVR_AST_FLAG_COMPARE_GREATER;
    }

    case XVR_TOKEN_OPERATOR_COMPARE_GREATER_EQUAL: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_COMPARISON + 1);
        return XVR_AST_FLAG_COMPARE_GREATER_EQUAL;
    }

    case XVR_TOKEN_OPERATOR_AND: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_AND + 1);
        return XVR_AST_FLAG_AND;
    }

    case XVR_TOKEN_OPERATOR_OR: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_OR + 1);
        return XVR_AST_FLAG_OR;
    }

    case XVR_TOKEN_OPERATOR_CONCAT: {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_CALL + 1);
        return XVR_AST_FLAG_CONCAT;
    }

    default:
        printError(parser, parser->previous,
                   "Unexpected token passed to binary precedence rule");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return XVR_AST_FLAG_NONE;
    }
}

static Xvr_AstFlag group(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                         Xvr_Ast** rootHandle) {
    if (parser->previous.type == XVR_TOKEN_OPERATOR_PAREN_LEFT) {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_GROUP);
        consume(parser, XVR_TOKEN_OPERATOR_PAREN_RIGHT,
                "Expected ')' at end of group");

        Xvr_private_emitAstGroup(bucketHandle, rootHandle);
    }

    else {
        printError(parser, parser->previous,
                   "Unexpected token passed to grouping precedence rule");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
    }

    return XVR_AST_FLAG_NONE;
}

static Xvr_AstFlag compound(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                            Xvr_Ast** rootHandle) {
    if (parser->previous.type == XVR_TOKEN_OPERATOR_BRACKET_LEFT) {
        if (match(parser, XVR_TOKEN_OPERATOR_BRACKET_RIGHT)) {
            Xvr_private_emitAstPass(bucketHandle, rootHandle);
            Xvr_private_emitAstCompound(bucketHandle, rootHandle,
                                        XVR_AST_FLAG_COMPOUND_ARRAY);
            return XVR_AST_FLAG_NONE;
        }

        if (match(parser, XVR_TOKEN_OPERATOR_COLON)) {
            consume(parser, XVR_TOKEN_OPERATOR_BRACKET_RIGHT,
                    "Expected ']' at the end of empty table");
            Xvr_private_emitAstPass(bucketHandle, rootHandle);
            Xvr_private_emitAstAggregate(bucketHandle, rootHandle,
                                         XVR_AST_FLAG_PAIR, *rootHandle);
            Xvr_private_emitAstCompound(bucketHandle, rootHandle,
                                        XVR_AST_FLAG_COMPOUND_TABLE);
            return XVR_AST_FLAG_NONE;
        }

        parsePrecedence(bucketHandle, parser, rootHandle, PREC_GROUP);

        Xvr_AstFlag flag = XVR_AST_FLAG_NONE;
        if ((*rootHandle)->type == XVR_AST_AGGREGATE) {
            flag = (*rootHandle)->aggregate.flag;
            if (flag == XVR_AST_FLAG_COLLECTION &&
                (*rootHandle)->aggregate.right->type == XVR_AST_AGGREGATE) {
                flag = (*rootHandle)->aggregate.right->aggregate.flag;
            }
        }

        if (parser->previous.type == XVR_TOKEN_OPERATOR_BRACKET_RIGHT &&
            parser->current.type != XVR_TOKEN_OPERATOR_BRACKET_RIGHT) {
            Xvr_private_emitAstCompound(bucketHandle, rootHandle,
                                        flag == XVR_AST_FLAG_PAIR
                                            ? XVR_AST_FLAG_COMPOUND_TABLE
                                            : XVR_AST_FLAG_COMPOUND_ARRAY);
            return XVR_AST_FLAG_NONE;
        }
        consume(parser, XVR_TOKEN_OPERATOR_BRACKET_RIGHT,
                "Expected ']' at the end of compound expression");
        Xvr_private_emitAstCompound(bucketHandle, rootHandle,
                                    flag == XVR_AST_FLAG_PAIR
                                        ? XVR_AST_FLAG_COMPOUND_TABLE
                                        : XVR_AST_FLAG_COMPOUND_ARRAY);
        return XVR_AST_FLAG_NONE;

    } else if (parser->previous.type == XVR_TOKEN_OPERATOR_BRACKET_RIGHT) {
        Xvr_private_emitAstPass(bucketHandle, rootHandle);
        return XVR_AST_FLAG_NONE;
    } else {
        printError(parser, parser->previous,
                   "Unexpected token passed to compound precedence rule");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return XVR_AST_FLAG_NONE;
    }
}

static Xvr_AstFlag aggregate(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                             Xvr_Ast** rootHandle) {
    advance(parser);

    if (parser->previous.type == XVR_TOKEN_OPERATOR_COMMA) {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_GROUP);
        return XVR_AST_FLAG_COLLECTION;
    } else if (parser->previous.type == XVR_TOKEN_OPERATOR_COLON) {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_GROUP);
        return XVR_AST_FLAG_PAIR;
    } else if (parser->previous.type == XVR_TOKEN_OPERATOR_BRACKET_LEFT) {
        parsePrecedence(bucketHandle, parser, rootHandle, PREC_GROUP);
        consume(parser, XVR_TOKEN_OPERATOR_BRACKET_RIGHT,
                "Expected ']' at the end of index expression");
        return XVR_AST_FLAG_INDEX;
    } else if (parser->previous.type == XVR_TOKEN_OPERATOR_BRACKET_RIGHT) {
        Xvr_private_emitAstPass(bucketHandle, rootHandle);
        return XVR_AST_FLAG_NONE;
    } else {
        printError(parser, parser->previous,
                   "Unexpected token passed to aggregate precedence rule");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return XVR_AST_FLAG_NONE;
    }
}

static Xvr_AstFlag unaryPostfix(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                                Xvr_Ast** rootHandle) {
    if (parser->previous.type != XVR_TOKEN_NAME) {
        printError(
            parser, parser->previous,
            "Unexpected parameter passing to unary-postfix precedence rule");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return XVR_AST_FLAG_NONE;
    }

    Xvr_Ast* primary = NULL;
    ParsingRule nameRule = getParsingRule(parser->previous.type)->prefix;
    nameRule(bucketHandle, parser, &primary);

    if (primary->type != XVR_AST_VAR_ACCESS ||
        primary->varAccess.child->type != XVR_AST_VALUE ||
        XVR_VALUE_IS_STRING(primary->varAccess.child->value.value) != true ||
        XVR_VALUE_AS_STRING(primary->varAccess.child->value.value)->info.type !=
            XVR_STRING_NAME) {
        printError(parser, parser->previous,
                   "Unexpected non-name-string token in unary-postfix operator "
                   "precedence rule");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return XVR_AST_FLAG_NONE;
    }

    (*rootHandle) = primary->varAccess.child;

    if (match(parser, XVR_TOKEN_OPERATOR_INCREMENT)) {
        Xvr_private_emitAstUnary(bucketHandle, rootHandle,
                                 XVR_AST_FLAG_POSTFIX_INCREMENT);
        return XVR_AST_FLAG_POSTFIX_INCREMENT;
    } else if (match(parser, XVR_TOKEN_OPERATOR_DECREMENT)) {
        Xvr_private_emitAstUnary(bucketHandle, rootHandle,
                                 XVR_AST_FLAG_POSTFIX_DECREMENT);
        return XVR_AST_FLAG_POSTFIX_DECREMENT;
    } else {
        printError(parser, parser->previous,
                   "Unexpected token passing to unary-postfix precedence rule");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return XVR_AST_FLAG_NONE;
    }
}

static void parsePrecedence(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                            Xvr_Ast** rootHandle, ParsingPrecedence precRule) {
    advance(parser);

    ParsingRule prefix = getParsingRule(parser->previous.type)->prefix;

    if (prefix == NULL) {
        if (Xvr_private_findKeywordByType(parser->previous.type)) {
            printError(parser, parser->previous,
                       "Found reserved keyword instead");
        } else {
            printError(parser, parser->previous, "Expected expression");
        }
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return;
    }

    prefix(bucketHandle, parser, rootHandle);

    // infix rules are left-recursive
    while (precRule <= getParsingRule(parser->current.type)->precedence) {
        ParsingRule infix = getParsingRule(parser->current.type)->infix;

        if (infix == NULL) {
            printError(parser, parser->previous, "Expected operator");
            Xvr_private_emitAstError(bucketHandle, rootHandle);
            return;
        }

        Xvr_Ast* ptr = NULL;
        Xvr_AstFlag flag = infix(bucketHandle, parser, &ptr);

        if (flag == XVR_AST_FLAG_NONE) {
            (*rootHandle) = ptr;
            return;
        } else if (flag >= 10 && flag <= 19) {
            Xvr_private_emitAstVariableAssignment(bucketHandle, rootHandle,
                                                  flag, ptr);
        } else if (flag >= 20 && flag <= 29) {
            Xvr_private_emitAstCompare(bucketHandle, rootHandle, flag, ptr);
        } else if (flag >= 30 && flag <= 39) {
            Xvr_private_emitAstAggregate(bucketHandle, rootHandle, flag, ptr);
        } else if (flag >= 40 && flag <= 49) {
            (*rootHandle) = ptr;
            continue;
        } else {
            if (flag == XVR_AST_FLAG_AND || flag == XVR_AST_FLAG_OR) {
                Xvr_private_emitAstBinaryShortCircuit(bucketHandle, rootHandle,
                                                      flag, ptr);
            } else {
                Xvr_private_emitAstBinary(bucketHandle, rootHandle, flag, ptr);
            }
        }
    }

    if (precRule <= PREC_ASSIGNMENT &&
        match(parser, XVR_TOKEN_OPERATOR_ASSIGN)) {
        printError(parser, parser->current, "Invalid assignment target");
    }
}

static void makeExpr(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                     Xvr_Ast** rootHandle) {
    parsePrecedence(bucketHandle, parser, rootHandle, PREC_ASSIGNMENT);
}

static void makeBlockStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                          Xvr_Ast** rootHandle);
static void makeDeclarationStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                                Xvr_Ast** rootHandle, bool errorOnEmpty);

static void makeAssertStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                           Xvr_Ast** rootHandle) {
    Xvr_Ast* ast = NULL;
    makeExpr(bucketHandle, parser, &ast);

    if (parser->removeAssert) {
        Xvr_private_emitAstPass(bucketHandle, rootHandle);
    } else {
        if (ast->type == XVR_AST_AGGREGATE) {
            Xvr_private_emitAstAssert(bucketHandle, rootHandle,
                                      ast->aggregate.left,
                                      ast->aggregate.right);
        } else {
            Xvr_private_emitAstAssert(bucketHandle, rootHandle, ast, NULL);
        }
    }

    consume(parser, XVR_TOKEN_OPERATOR_SEMICOLON,
            "Expected ';' at the end of assert statement");
}

static void makeIfThenElseStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                               Xvr_Ast** rootHandle) {
    Xvr_Ast* condBranch = NULL;
    Xvr_Ast* thenBranch = NULL;
    Xvr_Ast* elseBranch = NULL;

    consume(parser, XVR_TOKEN_OPERATOR_PAREN_LEFT,
            "Expected '(' after 'if' keyword");
    makeExpr(bucketHandle, parser, &condBranch);
    consume(parser, XVR_TOKEN_OPERATOR_PAREN_RIGHT,
            "Expected ')' after 'if' condition");

    makeDeclarationStmt(bucketHandle, parser, &thenBranch, true);

    if (match(parser, XVR_TOKEN_KEYWORD_ELSE)) {
        makeDeclarationStmt(bucketHandle, parser, &elseBranch, true);
    }

    Xvr_private_emitAstIfThenElse(bucketHandle, rootHandle, condBranch,
                                  thenBranch, elseBranch);
}

static void makeWhileStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                          Xvr_Ast** rootHandle) {
    Xvr_Ast* condBranch = NULL;
    Xvr_Ast* thenBranch = NULL;

    consume(parser, XVR_TOKEN_OPERATOR_PAREN_LEFT,
            "Expected '(' after 'while' keyword");
    makeExpr(bucketHandle, parser, &condBranch);
    consume(parser, XVR_TOKEN_OPERATOR_PAREN_RIGHT,
            "Expected ')' after 'while' condition");

    makeDeclarationStmt(bucketHandle, parser, &thenBranch, true);

    Xvr_private_emitAstWhileThen(bucketHandle, rootHandle, condBranch,
                                 thenBranch);
}

static void makeBreakStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                          Xvr_Ast** rootHandle) {
    Xvr_private_emitAstBreak(bucketHandle, rootHandle);
    consume(parser, XVR_TOKEN_OPERATOR_SEMICOLON,
            "Expected ';' at the end of break statement");
}

static void makeContinueStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                             Xvr_Ast** rootHandle) {
    Xvr_private_emitAstContinue(bucketHandle, rootHandle);
    consume(parser, XVR_TOKEN_OPERATOR_SEMICOLON,
            "Expected ';' at the end of continue statement");
}

static void makePrintStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                          Xvr_Ast** rootHandle) {
    makeExpr(bucketHandle, parser, rootHandle);
    Xvr_private_emitAstPrint(bucketHandle, rootHandle);

    consume(parser, XVR_TOKEN_OPERATOR_SEMICOLON,
            "Expected ';' at the end of print statement");
}

static void makeExprStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                         Xvr_Ast** rootHandle) {
    makeExpr(bucketHandle, parser, rootHandle);
    consume(parser, XVR_TOKEN_OPERATOR_SEMICOLON,
            "Expected ';' at the end of expression statement");
}

static void makeVariableDeclarationStmt(Xvr_Bucket** bucketHandle,
                                        Xvr_Parser* parser,
                                        Xvr_Ast** rootHandle) {
    consume(parser, XVR_TOKEN_NAME,
            "Expected variable name after 'var' keyword");

    if (parser->previous.length > 255) {
        printError(parser, parser->previous,
                   "Can't have a variable name longer than 255 characters");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return;
    }

    Xvr_Token nameToken = parser->previous;

    Xvr_ValueType varType = XVR_VALUE_ANY;
    bool constant = false;

    if (match(parser, XVR_TOKEN_OPERATOR_COLON)) {
        varType = readType(parser);

        if (match(parser, XVR_TOKEN_KEYWORD_CONST)) {
            constant = true;
        }
    }

    Xvr_String* nameStr = Xvr_createNameStringLength(
        bucketHandle, nameToken.lexeme, nameToken.length, varType, constant);

    Xvr_Ast* expr = NULL;
    if (match(parser, XVR_TOKEN_OPERATOR_ASSIGN)) {
        makeExpr(bucketHandle, parser, &expr);
    } else {
        Xvr_private_emitAstValue(bucketHandle, &expr, XVR_VALUE_FROM_NULL());
    }

    Xvr_private_emitAstVariableDeclaration(bucketHandle, rootHandle, nameStr,
                                           expr);
}

static void makeStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                     Xvr_Ast** rootHandle) {
    if (match(parser, XVR_TOKEN_OPERATOR_BRACE_LEFT)) {
        makeBlockStmt(bucketHandle, parser, rootHandle);
        consume(parser, XVR_TOKEN_OPERATOR_BRACE_RIGHT,
                "Expected '}' at the end of block scope");
        (*rootHandle)->block.innerScope = true;
        return;
    }

    else if (match(parser, XVR_TOKEN_KEYWORD_ASSERT)) {
        makeAssertStmt(bucketHandle, parser, rootHandle);
        return;
    }

    else if (match(parser, XVR_TOKEN_KEYWORD_IF)) {
        makeIfThenElseStmt(bucketHandle, parser, rootHandle);
        return;
    }

    else if (match(parser, XVR_TOKEN_KEYWORD_WHILE)) {
        makeWhileStmt(bucketHandle, parser, rootHandle);
        return;
    } else if (match(parser, XVR_TOKEN_KEYWORD_BREAK)) {
        makeBreakStmt(bucketHandle, parser, rootHandle);
        return;
    }

    else if (match(parser, XVR_TOKEN_KEYWORD_CONTINUE)) {
        makeContinueStmt(bucketHandle, parser, rootHandle);
        return;
    }

    else if (match(parser, XVR_TOKEN_KEYWORD_PRINT)) {
        makePrintStmt(bucketHandle, parser, rootHandle);
        return;
    }

    else if (match(parser, XVR_TOKEN_OPERATOR_SEMICOLON) ||
             match(parser, XVR_TOKEN_KEYWORD_PASS)) {
        Xvr_private_emitAstPass(bucketHandle, rootHandle);
        return;
    }

    else {
        makeExprStmt(bucketHandle, parser, rootHandle);
        return;
    }
}

static void makeDeclarationStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                                Xvr_Ast** rootHandle, bool errorOnEmpty) {
    if (errorOnEmpty && match(parser, XVR_TOKEN_OPERATOR_SEMICOLON)) {
        printError(
            parser, parser->previous,
            "Empty control flow bodies are dissalowed, use the 'pass' keyword");
        Xvr_private_emitAstError(bucketHandle, rootHandle);
        return;
    }

    else if (match(parser, XVR_TOKEN_KEYWORD_VAR)) {
        makeVariableDeclarationStmt(bucketHandle, parser, rootHandle);
    } else {
        makeStmt(bucketHandle, parser, rootHandle);
    }
}

static void makeBlockStmt(Xvr_Bucket** bucketHandle, Xvr_Parser* parser,
                          Xvr_Ast** rootHandle) {
    // begin the block
    Xvr_private_initAstBlock(bucketHandle, rootHandle);

    // read a series of statements into the block
    while (parser->current.type != XVR_TOKEN_OPERATOR_BRACE_RIGHT &&
           !match(parser, XVR_TOKEN_EOF)) {
        // process the grammar rules
        Xvr_Ast* stmt = NULL;
        makeDeclarationStmt(bucketHandle, parser, &stmt, false);

        // if something went wrong
        if (parser->panic) {
            synchronize(parser);

            Xvr_Ast* err = NULL;
            Xvr_private_emitAstError(bucketHandle, &err);
            Xvr_private_appendAstBlock(bucketHandle, *rootHandle, err);

            continue;
        }
        Xvr_private_appendAstBlock(bucketHandle, *rootHandle, stmt);
    }
}

void Xvr_bindParser(Xvr_Parser* parser, Xvr_Lexer* lexer) {
    Xvr_resetParser(parser);
    parser->lexer = lexer;
    advance(parser);
}

Xvr_Ast* Xvr_scanParser(Xvr_Bucket** bucketHandle, Xvr_Parser* parser) {
    Xvr_Ast* rootHandle = NULL;

    // check for EOF
    if (match(parser, XVR_TOKEN_EOF)) {
        Xvr_private_emitAstEnd(bucketHandle, &rootHandle);
        return rootHandle;
    }

    makeBlockStmt(bucketHandle, parser, &rootHandle);

    if (parser->panic != true && parser->previous.type != XVR_TOKEN_EOF) {
        printError(parser, parser->previous,
                   "Expected 'EOF' and the end of the parser scan (possibly an "
                   "extra '}' was found)");
    }

    return rootHandle;
}

void Xvr_resetParser(Xvr_Parser* parser) {
    parser->lexer = NULL;

    parser->current = ((Xvr_Token){XVR_TOKEN_NULL, 0, 0, NULL});
    parser->previous = ((Xvr_Token){XVR_TOKEN_NULL, 0, 0, NULL});

    parser->error = false;
    parser->panic = false;

    parser->removeAssert = false;
}

void Xvr_configureParser(Xvr_Parser* parser, bool removeAssert) {
    parser->removeAssert = removeAssert;
}
