#include "xvr_parser.h"
#include "xvr_ast.h"
#include "xvr_console_color.h"
#include "xvr_lexer.h"
#include "xvr_memory.h"
#include "xvr_token_types.h"
#include "xvr_value.h"
#include <stddef.h>
#include <stdio.h>

static void printError(Xvr_Parser *parser, Xvr_Token token,
                       const char *errorMsg) {
  if (parser->panic) {
    return;
  }

  fprintf(stderr, XVR_CC_ERROR "[Line %d] Error", token.line);

  if (token.type == XVR_TOKEN_EOF) {
    fprintf(stderr, "at end");
  } else {
    fprintf(stderr, "at '%.*s'", token.length, token.lexeme);
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
  }

  advance(parser);
}

static void synhronize(Xvr_Parser *parser) {
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

static ParsingTuple parseingRulesetTable[] = {
    {PREC_PRIMARY, atomic, NULL},

    // NONE TYPE
    {PREC_NONE, NULL, NULL},
    // BOOLEAN
    {PREC_NONE, NULL, NULL},
    // INTEGER,
    {PREC_NONE, NULL, NULL},
    // FLOAT
    {PREC_NONE, NULL, NULL},
    // STRING
    {PREC_NONE, NULL, NULL},
    // ARRAY
    {PREC_NONE, NULL, NULL},
    // DICTIONARY
    {PREC_NONE, NULL, NULL},
    // OPAQUE
    {PREC_NONE, NULL, NULL},
    // ANY
    {PREC_NONE, NULL, NULL},

    // AS
    {PREC_NONE, NULL, NULL},
    // ASSERT
    {PREC_NONE, NULL, NULL},
    // BREAK
    {PREC_NONE, NULL, NULL},
    // CLASS
    {PREC_NONE, NULL, NULL},
    // CONST
    {PREC_NONE, NULL, NULL},
    // CONTINUE
    {PREC_NONE, NULL, NULL},
    // DO
    {PREC_NONE, NULL, NULL},
    // ELSE
    {PREC_NONE, NULL, NULL},
    // EXPORT
    {PREC_NONE, NULL, NULL},
    // FOR
    {PREC_NONE, NULL, NULL},
    // FOREACH
    {PREC_NONE, NULL, NULL},
    // FUNCTION
    {PREC_NONE, NULL, NULL},
    // IF
    {PREC_NONE, NULL, NULL},
    // IMPORT
    {PREC_NONE, NULL, NULL},
    // IN
    {PREC_NONE, NULL, NULL},
    // OF
    {PREC_NONE, NULL, NULL},
    // PRINT
    {PREC_NONE, NULL, NULL},
    // RETURN
    {PREC_NONE, NULL, NULL},
    // TYPEAS
    {PREC_NONE, NULL, NULL},
    // TYPEOF
    {PREC_NONE, NULL, NULL},
    // VAR
    {PREC_NONE, NULL, NULL},
    // WHILE
    {PREC_NONE, NULL, NULL},
    // WHILE
    {PREC_NONE, NULL, NULL},

    // TRUE
    {PREC_PRIMARY, atomic, NULL},
    // FALSE
    {PREC_PRIMARY, atomic, NULL},
    // INTEGER
    {PREC_PRIMARY, atomic, NULL},
    // FLOAT
    {PREC_PRIMARY, atomic, NULL},
    // STRING
    {PREC_NONE, NULL, NULL},

    // ADD
    {PREC_TERM, NULL, binary},
    // SUBTRACT
    {PREC_TERM, unary, binary},
    // MULTIPLY
    {PREC_FACTOR, NULL, binary},
    // DIVIDE
    {PREC_FACTOR, NULL, binary},
    // MODULO
    {PREC_FACTOR, NULL, binary},
    // ADD_ASSIGN
    {PREC_ASSIGNMENT, NULL, binary},
    // SUBTRACT_ASSIGN
    {PREC_ASSIGNMENT, NULL, binary},
    // MULTIPLY_ASSIGN
    {PREC_ASSIGNMENT, NULL, binary},
    // DIVIDE_ASSIGN
    {PREC_ASSIGNMENT, NULL, binary},
    // MODULO_ASSIGN
    {PREC_ASSIGNMENT, NULL, binary},
    // OPERATOR_INCREMENT
    {PREC_NONE, NULL, NULL},
    // OPERATOR_DECREMENT
    {PREC_NONE, NULL, NULL},
    // OPERATOR_ASSIGN
    {PREC_ASSIGNMENT, NULL, binary},

    // COMPARE_EQUAL
    {PREC_COMPARISON, NULL, binary},
    // COMPARE_NOT
    {PREC_COMPARISON, NULL, binary},
    // COMPARE_LESS
    {PREC_COMPARISON, NULL, binary},
    // COMPARE_LESS_EQUAL
    {PREC_COMPARISON, NULL, binary},
    // COMPARE_GREATER
    {PREC_COMPARISON, NULL, binary},
    // COMPARE_GREATER_EQUAL
    {PREC_COMPARISON, NULL, binary},

    // OPERATOR_PAREN_LEFT
    {PREC_NONE, group, NULL},
    // PAREN_RIGHT
    {PREC_NONE, NULL, NULL},
    // PAREN_LEFT
    {PREC_NONE, NULL, NULL},
    // BRACKET_RIGHT
    {PREC_NONE, NULL, NULL},
    // BRACE_LEFT
    {PREC_NONE, NULL, NULL},
    // BRACE_RIGHT
    {PREC_NONE, NULL, NULL},

    // TOKEN_OPERATOR_ADD
    {PREC_NONE, NULL, NULL},
    // TOKEN_POERATOR_OR
    {PREC_NONE, NULL, NULL},
    // TOKEN_OPERATOR_NEGATE
    {PREC_NONE, unary, NULL},
    // TOKEN_OPERATOR_QUESTION
    {PREC_NONE, NULL, NULL},
    // TOKEN_OPERATOR_COLON
    {PREC_NONE, NULL, NULL},

    // OPERATOR_CONCAT
    {PREC_NONE, NULL, NULL},
    // OPERATOR_REST
    {PREC_NONE, NULL, NULL},

    // OPERATOR_AMPERSAND
    {PREC_NONE, NULL, NULL},
    // OPERATOR_PIPE
    {PREC_NONE, NULL, NULL},

    // TOKEN_PASS
    {PREC_NONE, NULL, NULL},
    // TOKEN_ERROR
    {PREC_NONE, NULL, NULL},
    // TOKEN_EOF
    {PREC_NONE, NULL, NULL},
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
      if (buffer[i] != '_') {
        i++;
      }
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
      if (buffer[i] != '_') {
        i++;
      }
    } while (parser->previous.lexeme[o++] && i < parser->previous.length);
    buffer[i] = '\0';

    float value = 0;
    sscanf(buffer, "%f", &value);
    Xvr_private_emitAstValue(bucket, root, XVR_VALUE_TO_FLOAT(value));
    return XVR_AST_FLAG_NONE;
  }

  default:
    printError(parser, parser->previous,
               "Unexpected token passing to atomic precedence rule");
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
  } else if (parser->previous.type == XVR_TOKEN_OPERATOR_NEGATE) {
    parsePrecedence(bucket, parser, root, PREC_UNARY);

    if ((*root)->type == XVR_AST_VALUE &&
        XVR_VALUE_IS_BOOLEAN((*root)->value.value)) {
      (*root)->value.value =
          XVR_VALUE_TO_BOOLEAN(!XVR_VALUE_AS_BOOLEAN((*root)->value.value));
    } else {
      Xvr_private_emitAstUnary(bucket, root, XVR_AST_FLAG_NEGATE);
    }
  } else {
    printError(parser, parser->previous,
               "Unexpected token passing to unary precedence rule");
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
               "Unexpected token passing to binary precedence rule");
    Xvr_private_emitAstError(bucket, root);
    return XVR_AST_FLAG_NONE;
  }
}

static Xvr_AstFlag group(Xvr_Bucket **bucket, Xvr_Parser *parser,
                         Xvr_Ast **root) {
  if (parser->previous.type == XVR_TOKEN_OPERATOR_PAREN_LEFT) {
    parsePrecedence(bucket, parser, root, PREC_GROUP);
    consume(parser, XVR_TOKEN_OPERATOR_PAREN_RIGHT,
            "expected `)` at end of group");
    Xvr_private_emitAstGroup(bucket, root);
  } else {
    printError(parser, parser->previous,
               "Unexpected token passing to grouping precedence rule");
    Xvr_private_emitAstError(bucket, root);
  }

  return XVR_AST_FLAG_NONE;
}

static ParsingTuple *getParsingRule(Xvr_TokenType type) {
  return &parseingRulesetTable[type];
}

static void parsePrecedence(Xvr_Bucket **bucket, Xvr_Parser *parser,
                            Xvr_Ast **root, ParsingPrecedence precRule) {
  advance(parser);

  ParsingRule prefix = getParsingRule(parser->previous.type)->prefix;
  if (prefix == NULL) {
    printError(parser, parser->previous, "expected expression");
    Xvr_private_emitAstError(bucket, root);
    return;
  }

  prefix(bucket, parser, root);

  while (precRule <= getParsingRule(parser->current.type)->precedence) {
    ParsingRule infix = getParsingRule(parser->current.type)->infix;
    if (infix == NULL) {
      printError(parser, parser->previous, "expected operator");
      Xvr_private_emitAstError(bucket, root);
      return;
    }
    Xvr_Ast *ptr = NULL;
    Xvr_AstFlag flag = infix(bucket, parser, &ptr);

    if (flag == XVR_AST_FLAG_NONE) {
      (*root) = ptr;
    }

    Xvr_private_emitAstBinary(bucket, root, flag, ptr);
  }

  if (precRule <= PREC_ASSIGNMENT && match(parser, XVR_TOKEN_OPERATOR_ASSIGN)) {
    printError(parser, parser->current, "invalid assignment target");
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
          "expected `;` at the end of the expression statement");
}

static void makeStmt(Xvr_Bucket **bucket, Xvr_Parser *parser, Xvr_Ast **root) {
  makeExprStmt(bucket, parser, root);
}

static void makeDeclarationStmt(Xvr_Bucket **buckeet, Xvr_Parser *parser,
                                Xvr_Ast **root) {
  // TODO: implemented
}

static void makeBlockStmt(Xvr_Bucket **bucket, Xvr_Parser *parser,
                          Xvr_Ast **root) {
  Xvr_private_initAstBlock(bucket, root);

  while (!match(parser, XVR_TOKEN_EOF)) {
    Xvr_Ast *stmt = NULL;
    makeDeclarationStmt(bucket, parser, &stmt);

    if (parser->panic) {
      synhronize(parser);

      Xvr_Ast *err = NULL;
      Xvr_private_emitAstError(bucket, &err);
      Xvr_private_appendAstBlock(bucket, root, err);

      continue;
    }

    Xvr_private_appendAstBlock(bucket, root, stmt);
  }
}

void Xvr_bindParser(Xvr_Parser *parser, Xvr_lexer *lexer) {
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
