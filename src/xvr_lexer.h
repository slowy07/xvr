#ifndef XVR_LEXER_H
#define XVR_LEXER_H

#include "xvr_common.h"
#include "xvr_token_types.h"

typedef struct {
  unsigned int start;   // start of the current token
  unsigned int current; // current position of the lexer
  unsigned int line;    // track this for error handling
  const char *source;
} Xvr_Lexer;

typedef struct {
  Xvr_TokenType type;
  unsigned int length;
  unsigned int line;
  const char *lexeme;
} Xvr_Token;

XVR_API void Xvr_bindLexer(Xvr_Lexer *lexer, const char *source);
XVR_API Xvr_Token Xvr_private_scanLexer(Xvr_Lexer *lexer);

XVR_API const char *Xvr_private_findKeywordByType(const Xvr_TokenType type);
XVR_API Xvr_TokenType Xvr_private_findTypeByKeyword(const char *keyword);
XVR_API void Xvr_private_printToken(Xvr_Token *token);

#endif // !XVR_LEXER_H
