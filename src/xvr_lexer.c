#include "xvr_lexer.h"
#include "xvr_console_colors.h"
#include "xvr_token_types.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  const Xvr_TokenType type;
  const char *keyword;
} Xvr_KeywordTypeTuple;

const Xvr_KeywordTypeTuple keywordTuples[] = {
    // null
    {XVR_TOKEN_NULL, "null"},

    // types
    {XVR_TOKEN_TYPE_TYPE, "type"},
    {XVR_TOKEN_TYPE_BOOLEAN, "bool"},
    {XVR_TOKEN_TYPE_INTEGER, "int"},
    {XVR_TOKEN_TYPE_FLOAT, "float"},
    {XVR_TOKEN_TYPE_STRING, "string"},

    {XVR_TOKEN_TYPE_ARRAY, "array"},
    {XVR_TOKEN_TYPE_TABLE, "table"},
    {XVR_TOKEN_TYPE_FUNCTION, "function"},

    {XVR_TOKEN_TYPE_OPAQUE, "opaque"},
    {XVR_TOKEN_TYPE_ANY, "any"},

    // keywords and reserved words
    {XVR_TOKEN_KEYWORD_AS, "as"},
    {XVR_TOKEN_KEYWORD_ASSERT, "assert"},
    {XVR_TOKEN_KEYWORD_BREAK, "break"},
    {XVR_TOKEN_KEYWORD_CLASS, "class"},
    {XVR_TOKEN_KEYWORD_CONST, "const"},
    {XVR_TOKEN_KEYWORD_CONTINUE, "continue"},
    {XVR_TOKEN_KEYWORD_DO, "do"},
    {XVR_TOKEN_KEYWORD_ELSE, "else"},
    {XVR_TOKEN_KEYWORD_EXPORT, "export"},
    {XVR_TOKEN_KEYWORD_FOR, "for"},
    {XVR_TOKEN_KEYWORD_FOREACH, "foreach"},
    {XVR_TOKEN_KEYWORD_FUNCTION, "fn"},
    {XVR_TOKEN_KEYWORD_IF, "if"},
    {XVR_TOKEN_KEYWORD_IMPORT, "import"},
    {XVR_TOKEN_KEYWORD_IN, "in"},
    {XVR_TOKEN_KEYWORD_OF, "of"},
    {XVR_TOKEN_KEYWORD_PRINT, "print"},
    {XVR_TOKEN_KEYWORD_RETURN, "return"},
    {XVR_TOKEN_KEYWORD_TYPEAS, "typeas"},
    {XVR_TOKEN_KEYWORD_TYPEOF, "typeof"},
    {XVR_TOKEN_KEYWORD_VAR, "var"},
    {XVR_TOKEN_KEYWORD_WHILE, "while"},
    {XVR_TOKEN_KEYWORD_YIELD, "yield"},

    // literal values
    {XVR_TOKEN_LITERAL_TRUE, "true"},
    {XVR_TOKEN_LITERAL_FALSE, "false"},

    {XVR_TOKEN_EOF, NULL},
};

const char *Xvr_private_findKeywordByType(const Xvr_TokenType type) {
  if (type == XVR_TOKEN_EOF) {
    return "EOF";
  }

  for (int i = 0; keywordTuples[i].keyword; i++) {
    if (keywordTuples[i].type == type) {
      return keywordTuples[i].keyword;
    }
  }

  return NULL;
}

Xvr_TokenType Xvr_private_findTypeByKeyword(const char *keyword) {
  const int length = strlen(keyword);

  for (int i = 0; keywordTuples[i].keyword; i++) {
    if (!strncmp(keyword, keywordTuples[i].keyword, length)) {
      return keywordTuples[i].type;
    }
  }

  return XVR_TOKEN_EOF;
}

static void cleanLexer(Xvr_Lexer *lexer) {
  lexer->start = 0;
  lexer->current = 0;
  lexer->line = 1;
  lexer->source = NULL;
}

static bool isAtEnd(Xvr_Lexer *lexer) {
  return lexer->source[lexer->current] == '\0';
}

static char peek(Xvr_Lexer *lexer) { return lexer->source[lexer->current]; }

static char peekNext(Xvr_Lexer *lexer) {
  if (isAtEnd(lexer))
    return '\0';
  return lexer->source[lexer->current + 1];
}

static char advance(Xvr_Lexer *lexer) {
  if (isAtEnd(lexer)) {
    return '\0';
  }

  // new line
  if (lexer->source[lexer->current] == '\n') {
    lexer->line++;
  }

  lexer->current++;
  return lexer->source[lexer->current - 1];
}

static void eatWhitespace(Xvr_Lexer *lexer) {
  const char c = peek(lexer);

  switch (c) {
  case ' ':
  case '\r':
  case '\n':
  case '\t':
    advance(lexer);
    break;

  // comments
  case '/':
    // eat the line
    if (peekNext(lexer) == '/') {
      while (!isAtEnd(lexer) && advance(lexer) != '\n')
        ;
      break;
    }

    // eat the block
    if (peekNext(lexer) == '*') {
      advance(lexer);
      advance(lexer);
      while (!isAtEnd(lexer) && !(peek(lexer) == '*' && peekNext(lexer) == '/'))
        advance(lexer);
      advance(lexer);
      advance(lexer);
      break;
    }
    return;

  default:
    return;
  }

  // tail recursion
  eatWhitespace(lexer);
}

static bool isDigit(Xvr_Lexer *lexer) {
  return peek(lexer) >= '0' && peek(lexer) <= '9';
}

static bool isAlpha(Xvr_Lexer *lexer) {
  return (peek(lexer) >= 'A' && peek(lexer) <= 'Z') ||
         (peek(lexer) >= 'a' && peek(lexer) <= 'z') || peek(lexer) == '_';
}

static bool match(Xvr_Lexer *lexer, char c) {
  if (peek(lexer) == c) {
    advance(lexer);
    return true;
  }

  return false;
}

// token generators
static Xvr_Token makeErrorToken(Xvr_Lexer *lexer, char *msg) {
  Xvr_Token token;

  token.type = XVR_TOKEN_ERROR;
  token.length = strlen(msg);
  token.line = lexer->line;
  token.lexeme = msg;

  return token;
}

static Xvr_Token makeToken(Xvr_Lexer *lexer, Xvr_TokenType type) {
  Xvr_Token token;

  token.type = type;
  token.length = lexer->current - lexer->start;
  token.line = lexer->line;
  token.lexeme = &lexer->source[lexer->current - token.length];

  return token;
}

static Xvr_Token makeIntegerOrFloat(Xvr_Lexer *lexer) {
  Xvr_TokenType type =
      XVR_TOKEN_LITERAL_INTEGER; // assume we're reading an integer

  // the character '_' can be inserted into numbers as a separator
  while (isDigit(lexer) || peek(lexer) == '_')
    advance(lexer);

  if (peek(lexer) == '.' &&
      (peekNext(lexer) >= '0' &&
       peekNext(lexer) <= '9')) {   // peekNext(lexer) == digit
    type = XVR_TOKEN_LITERAL_FLOAT; // change the assumption to reading a float
    advance(lexer);                 // eat the '.'

    //'_' again
    while (isDigit(lexer) || peek(lexer) == '_')
      advance(lexer);
  }

  // make the token
  Xvr_Token token;

  token.type = type;
  token.length = lexer->current - lexer->start;
  token.line = lexer->line;
  token.lexeme = &lexer->source[lexer->start];

  return token;
}

static bool isEscapableCharacter(char c) {
  switch (c) {
  case 'n':
  case 't':
  case '\\':
  case '"':
    return true;

  default:
    return false;
  }
}

static Xvr_Token makeString(Xvr_Lexer *lexer, char terminator) {
  while (!isAtEnd(lexer)) {
    // stop if you've hit the terminator
    if (peek(lexer) == terminator) {
      advance(lexer); // eat the terminator
      break;
    }

    // skip escaped control characters
    if (peek(lexer) == '\\' && isEscapableCharacter(peekNext(lexer))) {
      advance(lexer);
      advance(lexer);
      continue;
    }

    // otherwise
    advance(lexer);
  }

  if (isAtEnd(lexer)) {
    return makeErrorToken(lexer, "Unterminated string");
  }

  // make the token
  Xvr_Token token;

  token.type = XVR_TOKEN_LITERAL_STRING;
  token.length = lexer->current - lexer->start - 2; //-1 to omit the quotes
  token.line = lexer->line;
  token.lexeme = &lexer->source[lexer->start + 1]; //+1 to omit the first quote

  return token;
}

static Xvr_Token makeKeywordOrName(Xvr_Lexer *lexer) {
  advance(lexer); // first letter can only be alpha

  while (isDigit(lexer) || isAlpha(lexer)) {
    advance(lexer);
  }

  // scan for a keyword
  for (int i = 0; keywordTuples[i].keyword; i++) {
    // WONTFIX: could squeeze miniscule performance gain from this, but ROI
    // isn't worth it
    if (strlen(keywordTuples[i].keyword) == (lexer->current - lexer->start) &&
        !strncmp(keywordTuples[i].keyword, &lexer->source[lexer->start],
                 lexer->current - lexer->start)) {
      // make token (keyword)
      Xvr_Token token;

      token.type = keywordTuples[i].type;
      token.length = lexer->current - lexer->start;
      token.line = lexer->line;
      token.lexeme = &lexer->source[lexer->start];

      return token;
    }
  }

  // make token (name)
  Xvr_Token token;

  token.type = XVR_TOKEN_NAME;
  token.length = lexer->current - lexer->start;
  token.line = lexer->line;
  token.lexeme = &lexer->source[lexer->start];

  return token;
}

// exposed functions
void Xvr_bindLexer(Xvr_Lexer *lexer, const char *source) {
  cleanLexer(lexer);
  lexer->source = source;
}

Xvr_Token Xvr_private_scanLexer(Xvr_Lexer *lexer) {
  if (lexer->source == NULL) {
    return makeErrorToken(lexer, "Missing source code in lexer");
  }

  eatWhitespace(lexer);

  lexer->start = lexer->current;

  if (isAtEnd(lexer)) {
    return makeToken(lexer, XVR_TOKEN_EOF);
  }

  if (isDigit(lexer)) {
    return makeIntegerOrFloat(lexer);
  }
  if (isAlpha(lexer)) {
    return makeKeywordOrName(lexer);
  }

  char c = advance(lexer);

  switch (c) {
  case '(':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_PAREN_LEFT);
  case ')':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_PAREN_RIGHT);
  case '[':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_BRACKET_LEFT);
  case ']':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_BRACKET_RIGHT);
  case '{':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_BRACE_LEFT);
  case '}':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_BRACE_RIGHT);

  case '+':
    return makeToken(lexer, match(lexer, '=')   ? XVR_TOKEN_OPERATOR_ADD_ASSIGN
                            : match(lexer, '+') ? XVR_TOKEN_OPERATOR_INCREMENT
                                                : XVR_TOKEN_OPERATOR_ADD);
  case '-':
    return makeToken(lexer, match(lexer, '=')
                                ? XVR_TOKEN_OPERATOR_SUBTRACT_ASSIGN
                            : match(lexer, '-') ? XVR_TOKEN_OPERATOR_DECREMENT
                                                : XVR_TOKEN_OPERATOR_SUBTRACT);
  case '*':
    return makeToken(lexer, match(lexer, '=')
                                ? XVR_TOKEN_OPERATOR_MULTIPLY_ASSIGN
                                : XVR_TOKEN_OPERATOR_MULTIPLY);
  case '/':
    return makeToken(lexer, match(lexer, '=') ? XVR_TOKEN_OPERATOR_DIVIDE_ASSIGN
                                              : XVR_TOKEN_OPERATOR_DIVIDE);
  case '%':
    return makeToken(lexer, match(lexer, '=') ? XVR_TOKEN_OPERATOR_MODULO_ASSIGN
                                              : XVR_TOKEN_OPERATOR_MODULO);

  case '!':
    return makeToken(lexer, match(lexer, '=') ? XVR_TOKEN_OPERATOR_COMPARE_NOT
                                              : XVR_TOKEN_OPERATOR_NEGATE);
  case '=':
    return makeToken(lexer, match(lexer, '=') ? XVR_TOKEN_OPERATOR_COMPARE_EQUAL
                                              : XVR_TOKEN_OPERATOR_ASSIGN);

  case '<':
    return makeToken(lexer, match(lexer, '=')
                                ? XVR_TOKEN_OPERATOR_COMPARE_LESS_EQUAL
                                : XVR_TOKEN_OPERATOR_COMPARE_LESS);
  case '>':
    return makeToken(lexer, match(lexer, '=')
                                ? XVR_TOKEN_OPERATOR_COMPARE_GREATER_EQUAL
                                : XVR_TOKEN_OPERATOR_COMPARE_GREATER);

  case '&': // XVR_TOKEN_OPERATOR_AMPERSAND is unused
    if (match(lexer, '&')) {
      return makeToken(lexer, XVR_TOKEN_OPERATOR_AND);
    } else {
      return makeErrorToken(lexer, "Unexpected '&'");
    }

  case '|': // XVR_TOKEN_OPERATOR_PIPE is unused
    if (match(lexer, '|')) {
      return makeToken(lexer, XVR_TOKEN_OPERATOR_OR);
    } else {
      return makeErrorToken(lexer, "Unexpected '|'");
    }

  case '?':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_QUESTION);
  case ':':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_COLON);
  case ';':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_SEMICOLON);
  case ',':
    return makeToken(lexer, XVR_TOKEN_OPERATOR_COMMA);

  case '.':
    if (match(lexer, '.')) {
      if (match(lexer, '.')) {
        return makeToken(lexer, XVR_TOKEN_OPERATOR_REST); // three dots
      } else {
        return makeToken(lexer, XVR_TOKEN_OPERATOR_CONCAT); // two dots
      }
    } else {
      return makeToken(lexer, XVR_TOKEN_OPERATOR_DOT); // one dot
    }

  case '"':
    return makeString(lexer, c);

  default: {
    return makeErrorToken(lexer, "Unknown token value found in lexer");
  }
  }
}

static void trim(char **s, unsigned int *l) { // util
  while (isspace(((*((unsigned char **)(s)))[(*l) - 1])))
    (*l)--;
  while (**s && isspace(**(unsigned char **)(s))) {
    (*s)++;
    (*l)--;
  }
}

// for debugging
void Xvr_private_printToken(Xvr_Token *token) {
  // print errors
  if (token->type == XVR_TOKEN_ERROR) {
    printf(XVR_CC_ERROR "ERROR: \t%d\t%.*s\n" XVR_CC_RESET, (int)token->line,
           (int)token->length, token->lexeme);
    return;
  }

  // read pass token, even though it isn't generated
  if (token->type == XVR_TOKEN_PASS) {
    printf(XVR_CC_NOTICE "PASS: \t%d\t%.*s\n" XVR_CC_RESET, (int)token->line,
           (int)token->length, token->lexeme);
    return;
  }

  // print the line number
  printf("\t%d\t%d\t", token->type, (int)token->line);

  // print based on type
  if (token->type == XVR_TOKEN_NAME ||
      token->type == XVR_TOKEN_LITERAL_INTEGER ||
      token->type == XVR_TOKEN_LITERAL_FLOAT ||
      token->type == XVR_TOKEN_LITERAL_STRING) {
    printf("%.*s\t", (int)token->length, token->lexeme);
  } else {
    const char *keyword = Xvr_private_findKeywordByType(token->type);

    if (keyword != NULL) {
      printf("%s", keyword);
    } else {
      char *str = (char *)token->lexeme; // strip const-ness for trimming
      unsigned int length = token->length;
      trim(&str, &length);
      printf("%.*s", (int)length, str);
    }
  }

  printf("\n");
}
