#include "xvr_parser.h"
#include "xvr_ast.h"
#include "xvr_console_color.h"
#include "xvr_lexer.h"
#include "xvr_memory.h"
#include "xvr_token_types.h"
#include "xvr_value.h"
#include <stddef.h>
#include <stdio.h>

#include "xvr_parser.h"

#include <stdio.h>

static void printError(Xvr_Parser *parser, Xvr_Token token,
                       const char *errorMsg) {
  if (parser->panic) {
    return;
  }

  fprintf(stderr, XVR_CC_ERROR "[Line %d] Error ", token.line);

  if (token.type == XVR_TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else {
    fprintf(stderr, " at '%.*s'", token.length, token.lexeme);
  }

  fprintf(stderr, ": %s\n" XVR_CC_RESET, errorMsg);
  parser->error = true;
  parser->panic = true;
}

static void advance(Xvr_Parser *parser) {
  parser->previous = parser->current;
  parser->current = Xvr_private_scanLexer(parser->lexer);

  if (parser->current.type == XVR_TOKEN_ERROR) {
    printError(parser, parser->current, "Read error");
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
    // these tokens can start a statement
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

typedef Xvr_AstFlag (*ParsingRule)(Xvr_Bucket **bucket, Xvr_Parser *parser,
                                   Xvr_Ast **root);

typedef struct ParsingTuple {
  ParsingPrecedence precedence;
  ParsingRule prefix;
  ParsingRule infix;
} ParsingTuple;

static void parsePrecedence(Xvr_Bucket **bucket, Xvr_Parser *parser,
                            Xvr_Ast **root, ParsingPrecedence precRule);

static Xvr_AstFlag atomic(Xvr_Bucket **bucket, Xvr_Parser *parser,
                          Xvr_Ast **root);
static Xvr_AstFlag unary(Xvr_Bucket **bucket, Xvr_Parser *parser,
                         Xvr_Ast **root);
static Xvr_AstFlag binary(Xvr_Bucket **bucket, Xvr_Parser *parser,
                          Xvr_Ast **root);
static Xvr_AstFlag group(Xvr_Bucket **bucket, Xvr_Parser *parser,
                         Xvr_Ast **root);

static ParsingTuple parsingRulesetTable[] = {
    {PREC_PRIMARY, atomic, NULL}, // XVR_TOKEN_NULL,

    // variable names
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_IDENTIFIER,

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
    {PREC_PRIMARY, atomic, NULL}, // XVR_TOKEN_LITERAL_TRUE,
    {PREC_PRIMARY, atomic, NULL}, // XVR_TOKEN_LITERAL_FALSE,
    {PREC_PRIMARY, atomic, NULL}, // XVR_TOKEN_LITERAL_INTEGER,
    {PREC_PRIMARY, atomic, NULL}, // XVR_TOKEN_LITERAL_FLOAT,
    {PREC_NONE, NULL, NULL},      // XVR_TOKEN_LITERAL_STRING,

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

    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_OPERATOR_CONCAT, // ..
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_OPERATOR_REST, // ...

    // unused operators
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_OPERATOR_AMPERSAND, // &
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_OPERATOR_PIPE, // |

    // meta tokens
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_PASS,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_ERROR,
    {PREC_NONE, NULL, NULL}, // XVR_TOKEN_EOF,
};

static Xvr_AstFlag atomic(Xvr_Bucket **bucket, Xvr_Parser *parser,
                          Xvr_Ast **root) {
  switch (parser->previous.type) {
  case XVR_TOKEN_NULL:
    Xvr_private_emitAstValue(bucket, root, XVR_VALUE_TO_NULL());
    return XVR_AST_FLAG_NONE;

  case XVR_TOKEN_LITERAL_TRUE:
    Xvr_private_emitAstValue(bucket, root, XVR_VALUE_TO_BOOLEAN(true));
    return XVR_AST_FLAG_NONE;

  case XVR_TOKEN_LITERAL_FALSE:
    Xvr_private_emitAstValue(bucket, root, XVR_VALUE_TO_BOOLEAN(false));
    return XVR_AST_FLAG_NONE;

  case XVR_TOKEN_LITERAL_INTEGER: {
    char buffer[parser->previous.length];

    int i = 0, o = 0;
    do {
      buffer[i] = parser->previous.lexeme[o];
      if (buffer[i] != '_')
        i++;
    } while (parser->previous.lexeme[o++] && i < parser->previous.length);
    buffer[i] = '\0';

    int value = 0;
    sscanf(buffer, "%d", &value);
    Xvr_private_emitAstValue(bucket, root, XVR_VALUE_TO_INTEGER(value));
    return XVR_AST_FLAG_NONE;
  }

  case XVR_TOKEN_LITERAL_FLOAT: {
    char buffer[parser->previous.length];

    int i = 0, o = 0;
    do {
      buffer[i] = parser->previous.lexeme[o];
      if (buffer[i] != '_')
        i++;
    } while (parser->previous.lexeme[o++] && i < parser->previous.length);
    buffer[i] = '\0';

    float value = 0;
    sscanf(buffer, "%f", &value);
    Xvr_private_emitAstValue(bucket, root, XVR_VALUE_TO_FLOAT(value));
    return XVR_AST_FLAG_NONE;
  }

  default:
    printError(parser, parser->previous,
               "Unexpected token passed to atomic precedence rule");
    Xvr_private_emitAstError(bucket, root);
    return XVR_AST_FLAG_NONE;
  }
}

static Xvr_AstFlag unary(Xvr_Bucket **bucket, Xvr_Parser *parser,
                         Xvr_Ast **root) {

  if (parser->previous.type == XVR_TOKEN_OPERATOR_SUBTRACT) {

    bool connectedDigit =
        parser->previous.lexeme[1] >= '0' && parser->previous.lexeme[1] <= '9';
    parsePrecedence(bucket, parser, root, PREC_UNARY);

    if ((*root)->type == XVR_AST_VALUE &&
        XVR_VALUE_IS_INTEGER((*root)->value.value) && connectedDigit) {
      (*root)->value.value =
          XVR_VALUE_TO_INTEGER(-XVR_VALUE_AS_INTEGER((*root)->value.value));
    } else if ((*root)->type == XVR_AST_VALUE &&
               XVR_VALUE_IS_FLOAT((*root)->value.value) && connectedDigit) {
      (*root)->value.value =
          XVR_VALUE_TO_FLOAT(-XVR_VALUE_AS_FLOAT((*root)->value.value));
    } else {
      Xvr_private_emitAstUnary(bucket, root, XVR_AST_FLAG_NEGATE);
    }
  }

  else if (parser->previous.type == XVR_TOKEN_OPERATOR_NEGATE) {
    parsePrecedence(bucket, parser, root, PREC_UNARY);

    if ((*root)->type == XVR_AST_VALUE &&
        XVR_VALUE_IS_BOOLEAN((*root)->value.value)) {
      (*root)->value.value =
          XVR_VALUE_TO_BOOLEAN(!XVR_VALUE_AS_BOOLEAN((*root)->value.value));
    } else {
      Xvr_private_emitAstUnary(bucket, root, XVR_AST_FLAG_NEGATE);
    }
  }

  else {
    printError(parser, parser->previous,
               "Unexpected token passed to unary precedence rule");
    Xvr_private_emitAstError(bucket, root);
  }

  return XVR_AST_FLAG_NONE;
}

static Xvr_AstFlag binary(Xvr_Bucket **bucket, Xvr_Parser *parser,
                          Xvr_Ast **root) {
  advance(parser);

  switch (parser->previous.type) {
  case XVR_TOKEN_OPERATOR_ADD: {
    parsePrecedence(bucket, parser, root, PREC_TERM + 1);
    return XVR_AST_FLAG_ADD;
  }

  case XVR_TOKEN_OPERATOR_SUBTRACT: {
    parsePrecedence(bucket, parser, root, PREC_TERM + 1);
    return XVR_AST_FLAG_SUBTRACT;
  }

  case XVR_TOKEN_OPERATOR_MULTIPLY: {
    parsePrecedence(bucket, parser, root, PREC_FACTOR + 1);
    return XVR_AST_FLAG_MULTIPLY;
  }

  case XVR_TOKEN_OPERATOR_DIVIDE: {
    parsePrecedence(bucket, parser, root, PREC_FACTOR + 1);
    return XVR_AST_FLAG_DIVIDE;
  }

  case XVR_TOKEN_OPERATOR_MODULO: {
    parsePrecedence(bucket, parser, root, PREC_FACTOR + 1);
    return XVR_AST_FLAG_MODULO;
  }

  // assignment
  case XVR_TOKEN_OPERATOR_ASSIGN: {
    parsePrecedence(bucket, parser, root, PREC_ASSIGNMENT + 1);
    return XVR_AST_FLAG_ASSIGN;
  }

  case XVR_TOKEN_OPERATOR_ADD_ASSIGN: {
    parsePrecedence(bucket, parser, root, PREC_ASSIGNMENT + 1);
    return XVR_AST_FLAG_ADD_ASSIGN;
  }

  case XVR_TOKEN_OPERATOR_SUBTRACT_ASSIGN: {
    parsePrecedence(bucket, parser, root, PREC_ASSIGNMENT + 1);
    return XVR_AST_FLAG_SUBTRACT_ASSIGN;
  }

  case XVR_TOKEN_OPERATOR_MULTIPLY_ASSIGN: {
    parsePrecedence(bucket, parser, root, PREC_ASSIGNMENT + 1);
    return XVR_AST_FLAG_MULTIPLY_ASSIGN;
  }

  case XVR_TOKEN_OPERATOR_DIVIDE_ASSIGN: {
    parsePrecedence(bucket, parser, root, PREC_ASSIGNMENT + 1);
    return XVR_AST_FLAG_DIVIDE_ASSIGN;
  }

  case XVR_TOKEN_OPERATOR_MODULO_ASSIGN: {
    parsePrecedence(bucket, parser, root, PREC_ASSIGNMENT + 1);
    return XVR_AST_FLAG_MODULO_ASSIGN;
  }

  case XVR_TOKEN_OPERATOR_COMPARE_EQUAL: {
    parsePrecedence(bucket, parser, root, PREC_COMPARISON + 1);
    return XVR_AST_FLAG_COMPARE_EQUAL;
  }

  case XVR_TOKEN_OPERATOR_COMPARE_NOT: {
    parsePrecedence(bucket, parser, root, PREC_COMPARISON + 1);
    return XVR_AST_FLAG_COMPARE_NOT;
  }

  case XVR_TOKEN_OPERATOR_COMPARE_LESS: {
    parsePrecedence(bucket, parser, root, PREC_COMPARISON + 1);
    return XVR_AST_FLAG_COMPARE_LESS;
  }

  case XVR_TOKEN_OPERATOR_COMPARE_LESS_EQUAL: {
    parsePrecedence(bucket, parser, root, PREC_COMPARISON + 1);
    return XVR_AST_FLAG_COMPARE_LESS_EQUAL;
  }

  case XVR_TOKEN_OPERATOR_COMPARE_GREATER: {
    parsePrecedence(bucket, parser, root, PREC_COMPARISON + 1);
    return XVR_AST_FLAG_COMPARE_GREATER;
  }

  case XVR_TOKEN_OPERATOR_COMPARE_GREATER_EQUAL: {
    parsePrecedence(bucket, parser, root, PREC_COMPARISON + 1);
    return XVR_AST_FLAG_COMPARE_GREATER_EQUAL;
  }

  default:
    printError(parser, parser->previous,
               "Unexpected token passed to binary precedence rule");
    Xvr_private_emitAstError(bucket, root);
    return XVR_AST_FLAG_NONE;
  }
}

static Xvr_AstFlag group(Xvr_Bucket **bucket, Xvr_Parser *parser,
                         Xvr_Ast **root) {
  if (parser->previous.type == XVR_TOKEN_OPERATOR_PAREN_LEFT) {
    parsePrecedence(bucket, parser, root, PREC_GROUP);
    consume(parser, XVR_TOKEN_OPERATOR_PAREN_RIGHT,
            "Expected ')' at end of group");
  }

  else {
    printError(parser, parser->previous,
               "Unexpected token passed to grouping precedence rule");
    Xvr_private_emitAstError(bucket, root);
  }

  return XVR_AST_FLAG_NONE;
}

static ParsingTuple *getParsingRule(Xvr_TokenType type) {
  return &parsingRulesetTable[type];
}

static void parsePrecedence(Xvr_Bucket **bucket, Xvr_Parser *parser,
                            Xvr_Ast **root, ParsingPrecedence precRule) {
  advance(parser);
  ParsingRule prefix = getParsingRule(parser->previous.type)->prefix;

  if (prefix == NULL) {
    printError(parser, parser->previous, "Expected expression");
    Xvr_private_emitAstError(bucket, root);
    return;
  }

  prefix(bucket, parser, root);
  while (precRule <= getParsingRule(parser->current.type)->precedence) {
    ParsingRule infix = getParsingRule(parser->current.type)->infix;

    if (infix == NULL) {
      printError(parser, parser->previous, "Expected operator");
      Xvr_private_emitAstError(bucket, root);
      return;
    }

    Xvr_Ast *ptr = NULL;
    Xvr_AstFlag flag = infix(bucket, parser, &ptr);

    if (flag == XVR_AST_FLAG_NONE) {
      (*root) = ptr;
      return;
    }

    Xvr_private_emitAstBinary(bucket, root, flag, ptr);
  }

  if (precRule <= PREC_ASSIGNMENT && match(parser, XVR_TOKEN_OPERATOR_ASSIGN)) {
    printError(parser, parser->current, "Invalid assignment target");
  }
}

static void makeExpr(Xvr_Bucket **bucket, Xvr_Parser *parser, Xvr_Ast **root) {
  parsePrecedence(bucket, parser, root, PREC_ASSIGNMENT);
}

static void makeExprStmt(Xvr_Bucket **bucket, Xvr_Parser *parser,
                         Xvr_Ast **root) {
  if (match(parser, XVR_TOKEN_OPERATOR_SEMICOLON)) {
    Xvr_private_emitAstPass(bucket, root);
    return;
  }

  makeExpr(bucket, parser, root);
  consume(parser, XVR_TOKEN_OPERATOR_SEMICOLON,
          "Expected ';' at the end of expression statement");
}

static void makeStmt(Xvr_Bucket **bucket, Xvr_Parser *parser, Xvr_Ast **root) {

  makeExprStmt(bucket, parser, root);
}

static void makeDeclarationStmt(Xvr_Bucket **bucket, Xvr_Parser *parser,
                                Xvr_Ast **root) {
  makeStmt(bucket, parser, root);
}

static void makeBlockStmt(Xvr_Bucket **bucket, Xvr_Parser *parser,
                          Xvr_Ast **root) {

  Xvr_private_initAstBlock(bucket, root);

  while (!match(parser, XVR_TOKEN_EOF)) {
    Xvr_Ast *stmt = NULL;
    makeDeclarationStmt(bucket, parser, &stmt);

    if (parser->panic) {
      synchronize(parser);

      Xvr_Ast *err = NULL;
      Xvr_private_emitAstError(bucket, &err);
      Xvr_private_appendAstBlock(bucket, root, err);

      continue;
    }
    Xvr_private_appendAstBlock(bucket, root, stmt);
  }
}

void Xvr_bindParser(Xvr_Parser *parser, Xvr_Lexer *lexer) {
  Xvr_resetParser(parser);
  parser->lexer = lexer;
  advance(parser);
}

Xvr_Ast *Xvr_scanParser(Xvr_Bucket **bucket, Xvr_Parser *parser) {
  Xvr_Ast *root = NULL;

  if (match(parser, XVR_TOKEN_EOF)) {
    Xvr_private_emitAstEnd(bucket, &root);
    return root;
  }

  makeBlockStmt(bucket, parser, &root);

  return root;
}

void Xvr_resetParser(Xvr_Parser *parser) {
  parser->lexer = NULL;

  parser->current = XVR_BLANK_TOKEN();
  parser->previous = XVR_BLANK_TOKEN();

  parser->error = false;
  parser->panic = false;
}
