#include "xvr_parser.h"
#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_string.h"
#include "xvr_token_types.h"
#include "xvr_value.h"

#include <stdio.h>

static void printError(Xvr_Parser *parser, Xvr_Token token,
                       const char *errorMsg) {
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

static void advance(Xvr_Parser *parser) {
  parser->previous = parser->current;
  parser->current = Xvr_private_scanLexer(parser->lexer);

  if (parser->current.type == XVR_TOKEN_ERROR) {
    printError(parser, parser->current, "Can't read the source code");
  }
}

static bool match(Xvr_Parser *parser, Xvr_TokenType tokenType) {
  if (parser->current.type == tokenType) {
    advance(parser);
    return true;
  }
  return false;
}

static void consume(Xvr_Parser *parser, Xvr_TokenType tokenType,
                    const char *msg) {
  if (parser->current.type != tokenType) {
    printError(parser, parser->current, msg);
    return;
  }

  advance(parser);
}

static void synchronize(Xvr_Parser *parser) {
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
  PREC_OR,
  PREC_AND,
  PREC_COMPARISON,
  PREC_TERM,
  PREC_FACTOR,
  PREC_UNARY,
  PREC_CALL,
  PREC_PRIMARY,
} ParsingPrecedence;

typedef Xvr_AstFlag (*ParsingRule)(Xvr_Bucket **bucketHandle,
                                   Xvr_Parser *parser, Xvr_Ast **rootHandle);

typedef struct ParsingTuple {
  ParsingPrecedence precedence;
  ParsingRule prefix;
  ParsingRule infix;
} ParsingTuple;

static void parsePrecedence(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                            Xvr_Ast **rootHandle, ParsingPrecedence precRule);

static Xvr_AstFlag literal(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                           Xvr_Ast **rootHandle);
static Xvr_AstFlag unary(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                         Xvr_Ast **rootHandle);
static Xvr_AstFlag binary(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                          Xvr_Ast **rootHandle);
static Xvr_AstFlag group(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                         Xvr_Ast **rootHandle);

static ParsingTuple parsingRulesetTable[] = {
    {PREC_PRIMARY, literal, NULL}, // XVR_TOKEN_NULL,

    // variable names
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_NAME,

    // types
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_TYPE,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_BOOLEAN,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_INTEGER,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_FLOAT,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_STRING,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_ARRAY,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_DICTIONARY,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_FUNCTION,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_OPAQUE,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_TYPE_ANY,

    // keywords and reserved words
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_AS,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_ASSERT,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_BREAK,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_CLASS,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_CONST,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_CONTINUE,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_DO,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_ELSE,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_EXPORT,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_FOR,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_FOREACH,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_FUNCTION,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_IF,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_IMPORT,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_IN,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_OF,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_PRINT,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_RETURN,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_TYPEAS,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_TYPEOF,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_VAR,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_WHILE,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_KEYWORD_YIELD,

    // literal values
    {PREC_PRIMARY, literal, NULL}, // XVR_TOKEN_LITERAL_TRUE,
    {PREC_PRIMARY, literal, NULL}, // XVR_TOKEN_LITERAL_FALSE,
    {PREC_PRIMARY, literal, NULL}, // XVR_TOKEN_LITERAL_INTEGER,
    {PREC_PRIMARY, literal, NULL}, // XVR_TOKEN_LITERAL_FLOAT,
    {PREC_PRIMARY, literal, NULL}, // XVR_TOKEN_LITERAL_STRING,

    // math operators
    {PREC_TERM, NULL, binary},       // XVR_TOKEN_OPERATOR_ADD,
    {PREC_TERM, unary, binary},      // XVR_TOKEN_OPERATOR_SUBTRACT,
    {PREC_FACTOR, NULL, binary},     // XVR_TOKEN_OPERATOR_MULTIPLY,
    {PREC_FACTOR, NULL, binary},     // XVR_TOKEN_OPERATOR_DIVIDE,
    {PREC_FACTOR, NULL, binary},     // XVR_TOKEN_OPERATOR_MODULO,
    {PREC_ASSIGNMENT, NULL, binary}, // XVR_TOKEN_OPERATOR_ADD_ASSIGN,
    {PREC_ASSIGNMENT, NULL, binary}, // XVR_TOKEN_OPERATOR_SUBTRACT_ASSIGN,
    {PREC_ASSIGNMENT, NULL, binary}, // XVR_TOKEN_OPERATOR_MULTIPLY_ASSIGN,
    {PREC_ASSIGNMENT, NULL, binary}, // XVR_TOKEN_OPERATOR_DIVIDE_ASSIGN,
    {PREC_ASSIGNMENT, NULL, binary}, // XVR_TOKEN_OPERATOR_MODULO_ASSIGN,
    {PREC_NONE, NULL, NULL},         // XVR_TOKEN_OPERATOR_INCREMENT,
    {PREC_NONE, NULL, NULL},         // XVR_TOKEN_OPERATOR_DECREMENT,
    {PREC_ASSIGNMENT, NULL, binary}, // XVR_TOKEN_OPERATOR_ASSIGN,

    // comparator operators
    {PREC_COMPARISON, NULL, binary}, // XVR_TOKEN_OPERATOR_COMPARE_EQUAL,
    {PREC_COMPARISON, NULL, binary}, // XVR_TOKEN_OPERATOR_COMPARE_NOT,
    {PREC_COMPARISON, NULL, binary}, // XVR_TOKEN_OPERATOR_COMPARE_LESS,
    {PREC_COMPARISON, NULL, binary}, // XVR_TOKEN_OPERATOR_COMPARE_LESS_EQUAL,
    {PREC_COMPARISON, NULL, binary}, // XVR_TOKEN_OPERATOR_COMPARE_GREATER,
    {PREC_COMPARISON, NULL,
     binary}, // XVR_TOKEN_OPERATOR_COMPARE_GREATER_EQUAL,

    // structural operators
    {PREC_NONE, group, NULL}, // XVR_TOKEN_OPERATOR_PAREN_LEFT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_PAREN_RIGHT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_BRACKET_LEFT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_BRACKET_RIGHT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_BRACE_LEFT,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_BRACE_RIGHT,

    // other operators
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_AND,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_OR,
    {PREC_NONE, unary, NULL}, // XVR_TOKEN_OPERATOR_NEGATE,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_QUESTION,
    {PREC_NONE, NULL, NULL},  // XVR_TOKEN_OPERATOR_COLON,

    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_OPERATOR_SEMICOLON, // ;
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_OPERATOR_COMMA, // ,

    {PREC_NONE, NULL, NULL},   // XVR_TOKEN_OPERATOR_DOT, // .
    {PREC_CALL, NULL, binary}, // XVR_TOKEN_OPERATOR_CONCAT, // ..
    {PREC_NONE, NULL, NULL},   // XVR_TOKEN_OPERATOR_REST, // ...

    // unused operators
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_OPERATOR_AMPERSAND, // &
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_OPERATOR_PIPE, // |

    // meta tokens
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_PASS,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_ERROR,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_EOF,
};

static Xvr_AstFlag literal(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                           Xvr_Ast **rootHandle) {
  switch (parser->previous.type) {
  case XVR_TOKEN_NULL:
    Xvr_private_emitAstValue(bucketHandle, rootHandle, XVR_VALUE_FROM_NULL());
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
      if (buffer[i] != '_')
        i++;
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
      if (buffer[i] != '_')
        i++;
    } while (parser->previous.lexeme[o++] && i < parser->previous.length);
    buffer[i] = '\0'; // BUGFIX

    float value = 0;
    sscanf(buffer, "%f", &value);
    Xvr_private_emitAstValue(bucketHandle, rootHandle,
                             XVR_VALUE_FROM_FLOAT(value));
    return XVR_AST_FLAG_NONE;
  }

  case XVR_TOKEN_LITERAL_STRING: {
    char buffer[parser->previous.length + 1];

    unsigned int i = 0, o = 0;
    do {
      buffer[i] = parser->previous.lexeme[o];
      if (buffer[i] == '\\' && parser->previous.lexeme[++o]) {
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
    } while (parser->previous.lexeme[o++] && i < parser->previous.length);
    buffer[i] = '\0';
    Xvr_private_emitAstValue(
        bucketHandle, rootHandle,
        XVR_VALUE_FROM_STRING(Xvr_createStringLength(bucketHandle, buffer, i)));
    return XVR_AST_FLAG_NONE;
  }

  default:
    printError(parser, parser->previous,
               "Unexpected token passed to literal precedence rule");
    Xvr_private_emitAstError(bucketHandle, rootHandle);
    return XVR_AST_FLAG_NONE;
  }
}

static Xvr_AstFlag unary(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                         Xvr_Ast **rootHandle) {
  //'subtract' can only be applied to numbers and groups, while 'negate' can
  // only be applied to booleans and groups this function takes the libery of
  // peeking into the uppermost node, to see if it can apply this to it

  if (parser->previous.type == XVR_TOKEN_OPERATOR_SUBTRACT) {

    bool connectedDigit =
        parser->previous.lexeme[1] >= '0' && parser->previous.lexeme[1] <= '9';
    parsePrecedence(bucketHandle, parser, rootHandle, PREC_UNARY);

    if ((*rootHandle)->type == XVR_AST_VALUE &&
        XVR_VALUE_IS_INTEGER((*rootHandle)->value.value) && connectedDigit) {
      (*rootHandle)->value.value = XVR_VALUE_FROM_INTEGER(
          -XVR_VALUE_AS_INTEGER((*rootHandle)->value.value));
    } else if ((*rootHandle)->type == XVR_AST_VALUE &&
               XVR_VALUE_IS_FLOAT((*rootHandle)->value.value) &&
               connectedDigit) {
      (*rootHandle)->value.value =
          XVR_VALUE_FROM_FLOAT(-XVR_VALUE_AS_FLOAT((*rootHandle)->value.value));
    } else {
      Xvr_private_emitAstUnary(bucketHandle, rootHandle, XVR_AST_FLAG_NEGATE);
    }
  }

  else if (parser->previous.type == XVR_TOKEN_OPERATOR_NEGATE) {
    parsePrecedence(bucketHandle, parser, rootHandle, PREC_UNARY);
    Xvr_private_emitAstUnary(bucketHandle, rootHandle, XVR_AST_FLAG_NEGATE);
  }

  else {
    printError(parser, parser->previous,
               "Unexpected token passed to unary precedence rule");
    Xvr_private_emitAstError(bucketHandle, rootHandle);
  }

  return XVR_AST_FLAG_NONE;
}

static Xvr_AstFlag binary(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                          Xvr_Ast **rootHandle) {
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

  // assignment
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

static Xvr_AstFlag group(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                         Xvr_Ast **rootHandle) {
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

static ParsingTuple *getParsingRule(Xvr_TokenType type) {
  return &parsingRulesetTable[type];
}

// grammar rules
static void parsePrecedence(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                            Xvr_Ast **rootHandle, ParsingPrecedence precRule) {
  //'step over' the token to parse
  advance(parser);

  // every valid expression has a prefix rule
  ParsingRule prefix = getParsingRule(parser->previous.type)->prefix;

  if (prefix == NULL) {
    printError(parser, parser->previous, "Expected expression");
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

    Xvr_Ast *ptr = NULL;
    Xvr_AstFlag flag = infix(bucketHandle, parser, &ptr);

    // finished
    if (flag == XVR_AST_FLAG_NONE) {
      (*rootHandle) = ptr;
      return;
    }

    Xvr_private_emitAstBinary(bucketHandle, rootHandle, flag, ptr);
  }

  // can't assign below a certain precedence
  if (precRule <= PREC_ASSIGNMENT && match(parser, XVR_TOKEN_OPERATOR_ASSIGN)) {
    printError(parser, parser->current, "Invalid assignment target");
  }
}

static void makeExpr(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                     Xvr_Ast **rootHandle) {
  parsePrecedence(bucketHandle, parser, rootHandle, PREC_ASSIGNMENT);
}

static void makePrintStmt(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                          Xvr_Ast **rootHandle) {
  makeExpr(bucketHandle, parser, rootHandle);
  Xvr_private_emitAstPrint(bucketHandle, rootHandle);

  consume(parser, XVR_TOKEN_OPERATOR_SEMICOLON,
          "Expected ';' at the end of print statement");
}

static void makeExprStmt(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                         Xvr_Ast **rootHandle) {
  makeExpr(bucketHandle, parser, rootHandle);
  consume(parser, XVR_TOKEN_OPERATOR_SEMICOLON,
          "Expected ';' at the end of expression statement");
}

static void makeVariableDeclarationStmt(Xvr_Bucket **bucketHandle,
                                        Xvr_Parser *parser,
                                        Xvr_Ast **rootHandle) {
  consume(parser, XVR_TOKEN_NAME, "Expecter variable name after `var` keyword");

  if (parser->previous.length > 256) {
    printError(parser, parser->previous,
               "Can't have a variable name longer than 256 characters");
    Xvr_private_emitAstError(bucketHandle, rootHandle);
    return;
  }

  Xvr_Token nameToken = parser->previous;

  Xvr_String *nameStr = Xvr_createNameStringLength(
      bucketHandle, nameToken.lexeme, nameToken.length, XVR_VALUE_NULL);

  Xvr_Ast *expr = NULL;
  if (match(parser, XVR_TOKEN_OPERATOR_ASSIGN)) {
    makeExpr(bucketHandle, parser, &expr);
  } else {
    Xvr_private_emitAstValue(bucketHandle, rootHandle, XVR_VALUE_FROM_NULL());
  }

  Xvr_private_emitAstVariableDeclaration(bucketHandle, rootHandle, nameStr,
                                         expr);
}

static void makeStmt(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                     Xvr_Ast **rootHandle) {
  if (match(parser, XVR_TOKEN_OPERATOR_SEMICOLON)) {
    Xvr_private_emitAstPass(bucketHandle, rootHandle);
    return;
  }

  else if (match(parser, XVR_TOKEN_KEYWORD_PRINT)) {
    makePrintStmt(bucketHandle, parser, rootHandle);
    return;
  }

  else {
    makeExprStmt(bucketHandle, parser, rootHandle);
    return;
  }
}

static void makeDeclarationStmt(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                                Xvr_Ast **rootHandle) {
  if (match(parser, XVR_TOKEN_KEYWORD_VAR)) {
    makeVariableDeclarationStmt(bucketHandle, parser, rootHandle);
  } else {
    makeStmt(bucketHandle, parser, rootHandle);
  }
}

static void makeBlockStmt(Xvr_Bucket **bucketHandle, Xvr_Parser *parser,
                          Xvr_Ast **rootHandle) {
  Xvr_private_initAstBlock(bucketHandle, rootHandle);

  while (!match(parser, XVR_TOKEN_EOF)) {
    Xvr_Ast *stmt = NULL;
    makeDeclarationStmt(bucketHandle, parser, &stmt);

    if (parser->panic) {
      synchronize(parser);

      Xvr_Ast *err = NULL;
      Xvr_private_emitAstError(bucketHandle, &err);
      Xvr_private_appendAstBlock(bucketHandle, *rootHandle, err);

      continue;
    }
    Xvr_private_appendAstBlock(bucketHandle, *rootHandle, stmt);
  }
}

void Xvr_bindParser(Xvr_Parser *parser, Xvr_Lexer *lexer) {
  Xvr_resetParser(parser);
  parser->lexer = lexer;
  advance(parser);
}

Xvr_Ast *Xvr_scanParser(Xvr_Bucket **bucketHandle, Xvr_Parser *parser) {
  Xvr_Ast *rootHandle = NULL;

  // check for EOF
  if (match(parser, XVR_TOKEN_EOF)) {
    Xvr_private_emitAstEnd(bucketHandle, &rootHandle);
    return rootHandle;
  }

  makeBlockStmt(bucketHandle, parser, &rootHandle);

  return rootHandle;
}

void Xvr_resetParser(Xvr_Parser *parser) {
  parser->lexer = NULL;

  parser->current = XVR_BLANK_TOKEN();
  parser->previous = XVR_BLANK_TOKEN();

  parser->error = false;
  parser->panic = false;
}
