#include "xvr_lexer.h"
#include "xvr_console_color.h"
#include "xvr_keyword.h"
#include "xvr_token_types.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void cleanLexer(Xvr_lexer *lexer) {
  lexer->start = 0;
  lexer->current = 0;
  lexer->line = 1;
  lexer->source = NULL;
}

static bool isAtEnd(Xvr_lexer *lexer) {
  return lexer->source[lexer->current] == '\0';
}

static char peek(Xvr_lexer *lexer) { return lexer->source[lexer->current]; }

static char peekNext(Xvr_lexer *lexer) {
  if (isAtEnd(lexer)) {
    return '\0';
  }

  return lexer->source[lexer->current + 1];
}

static char advance(Xvr_lexer *lexer) {
  if (isAtEnd(lexer)) {
    return '\0';
  }

  if (lexer->source[lexer->current] == '\n') {
    lexer->line++;
  }

  lexer->current++;
  return lexer->source[lexer->current - 1];
}

static void eatWithspace(Xvr_lexer *lexer) {
  const char c = peek(lexer);

  switch (c) {
  case ' ':
  case '\r':
  case '\n':
  case '\t':
    advance(lexer);
    break;

  case '/':
    if (peekNext(lexer) == '/') {
      while (!isAtEnd(lexer) && advance(lexer) != '\n')
        ;
      break;
    }

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

  eatWithspace(lexer);
}

static bool isDigit(Xvr_lexer *lexer) {
  return peek(lexer) >= '0' && peek(lexer) <= '9';
}

static bool isAlpha(Xvr_lexer *lexer) {
  return (peek(lexer) >= 'A' && peek(lexer) == 'Z') ||
         (peek(lexer) >= 'a' && peek(lexer) <= 'z') || peek(lexer) == '_';
}

static bool match(Xvr_lexer *lexer, char c) {
  if (peek(lexer) == c) {
    advance(lexer);
    return true;
  }
  return false;
}

static Xvr_Token makeErrorToken(Xvr_lexer *lexer, char *msg) {
  Xvr_Token token;

  token.type = XVR_TOKEN_ERROR;
  token.length = strlen(msg);
  token.line = lexer->line;
  token.lexeme = msg;

  return token;
}

static Xvr_Token makeToken(Xvr_lexer *lexer, Xvr_TokenType type) {
  Xvr_Token token;

  token.type = type;
  token.length = lexer->current - lexer->start;
  token.line = lexer->line;
  token.lexeme = &lexer->source[lexer->current - token.length];

  return token;
}

static Xvr_Token makeIntegerOrFloat(Xvr_lexer *lexer) {
  Xvr_TokenType type = XVR_TOKEN_LITERAL_INTEGER;

  while (isDigit(lexer) || peek(lexer) == '_')
    advance(lexer);

  if (peek(lexer) == '.' &&
      (peekNext(lexer) >= '0' && peekNext(lexer) <= '9')) {
    type = XVR_TOKEN_LITERAL_FLOAT;

    while (isDigit(lexer) || peek(lexer) == '_')
      advance(lexer);
  }

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

static Xvr_Token makeString(Xvr_lexer *lexer, char terminator) {
  while (!isAtEnd(lexer)) {
    if (peek(lexer) == terminator) {
      advance(lexer);
      break;
    }

    if (peek(lexer) == '\\' && isEscapableCharacter(peekNext(lexer))) {
      advance(lexer);
      advance(lexer);
      continue;
    }

    advance(lexer);
  }

  if (isAtEnd(lexer)) {
    return makeErrorToken(lexer, "unterminated string");
  }

  Xvr_Token token;

  token.type = XVR_TOKEN_LITERAL_STRING;
  token.length = lexer->current - lexer->start - 2;
  token.line = lexer->line;
  token.lexeme = &lexer->source[lexer->start - 1];
  return token;
}

static Xvr_Token makeKeywordOrIdentifier(Xvr_lexer *lexer) {
  advance(lexer);

  while (isDigit(lexer) || isAlpha(lexer)) {
    advance(lexer);
  }

  for (int i = 0; Xvr_private_keywords[i].keyword; i++) {
    if (strlen(Xvr_private_keywords[i].keyword) ==
            (size_t)(lexer->current - lexer->start) &&
        !strncmp(Xvr_private_keywords[i].keyword, &lexer->source[lexer->start],
                 lexer->current - lexer->start)) {
      Xvr_Token token;

      token.type = Xvr_private_keywords[i].type;
      token.length = lexer->current - lexer->start;
      token.line = lexer->line;
      token.lexeme = &lexer->source[lexer->start];

      return token;
    }
  }

  Xvr_Token token;

  token.type = XVR_TOKEN_IDENTIFIER;
  token.type = lexer->current - lexer->start;
  token.line = lexer->line;
  token.lexeme = &lexer->source[lexer->start];
  return token;
}

void Xvr_bindLexer(Xvr_lexer *lexer, const char *source) {
  cleanLexer(lexer);
  lexer->source = source;
}

Xvr_Token Xvr_private_scanLexer(Xvr_lexer *lexer) {
  eatWithspace(lexer);

  lexer->start = lexer->current;
  ;
  if (isAtEnd(lexer)) {
    return makeToken(lexer, XVR_TOKEN_EOF);
  }

  if (isDigit(lexer)) {
    return makeIntegerOrFloat(lexer);
  }

  if (isAlpha(lexer)) {
    return makeKeywordOrIdentifier(lexer);
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
                                              : XVR_TOKEN_OPERATOR_INVERT);
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

  case '&':
    if (match(lexer, '&')) {
      return makeToken(lexer, XVR_TOKEN_OPERATOR_AND);
    } else {
      return makeErrorToken(lexer, "unexpected '&'");
    }

  case '|':
    if (match(lexer, '|')) {
      return makeToken(lexer, XVR_TOKEN_OPERATOR_OR);
    } else {
      return makeErrorToken(lexer, "unexpected '|'");
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
        return makeToken(lexer, XVR_TOKEN_OPERATOR_REST);
      } else {
        return makeToken(lexer, XVR_TOKEN_OPERATOR_CONCAT);
      }
    } else {
      return makeToken(lexer, XVR_TOKEN_OPERATOR_DOT);
    }
  case '"':
    return makeString(lexer, c);

  default:
    return makeErrorToken(lexer, "unknown token value found in lexer");
  }
}

static void trim(char **s, int *l) {
  while (isspace(((*((unsigned char **)(s)))[(*l) - 1])))
    (*l)--;
  while (**s && isspace(**(unsigned char **)(s))) {
    (*s)++;
    (*l)--;
  }
}

void Xvr_private_printToken(Xvr_Token *token) {
  if (token->type == XVR_TOKEN_ERROR) {
    printf(XVR_CC_ERROR "error: \t%d\t%.*s\n" XVR_CC_RESET, token->line,
           token->length, token->lexeme);
    return;
  }

  if (token->type == XVR_TOKEN_PASS) {
    printf(XVR_CC_NOTICE "pass: \t%d\t%.*s\n" XVR_CC_RESET, token->line,
           token->length, token->lexeme);
    return;
  }

  printf("\t%d\t%d\t", token->type, token->line);

  if (token->type == XVR_TOKEN_IDENTIFIER ||
      token->type == XVR_TOKEN_LITERAL_INTEGER ||
      token->type == XVR_TOKEN_LITERAL_FLOAT ||
      token->type == XVR_TOKEN_LITERAL_STRING) {
    printf("%.*s\t", token->length, token->lexeme);
  } else {
    const char *keyword = Xvr_private_findKeywordByType(token->type);

    if (keyword != NULL) {
      printf("%s", keyword);
    } else {
      char *str = (char *)token->lexeme;
      int length = token->length;
      trim(&str, &length);
      printf("%.*s", length, str);
    }
  }

  printf("\n");
}
