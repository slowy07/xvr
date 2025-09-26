#include "xvr_keyword.h"
#include "xvr_token_types.h"
#include <string.h>

const Xvr_KeywordTypeTuple Xvr_private_keywords[] = {

    {XVR_TOKEN_NULL, "null"},

    {XVR_TOKEN_TYPE_TYPE, "type"},
    {XVR_TOKEN_TYPE_BOOLEAN, "bool"},
    {XVR_TOKEN_TYPE_INTEGER, "int"},
    {XVR_TOKEN_TYPE_FLOAT, "float"},
    {XVR_TOKEN_TYPE_STRING, "string"},

    {XVR_TOKEN_TYPE_OPAQUE, "opaque"},
    {XVR_TOKEN_TYPE_ANY, "any"},

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

    {XVR_TOKEN_LITERAL_TRUE, "true"},
    {XVR_TOKEN_LITERAL_FALSE, "false"},

    {XVR_TOKEN_EOF, NULL},
};

const char *Xvr_private_findKeywordByType(const Xvr_TokenType type) {
  if (type == XVR_TOKEN_EOF) {
    return "EOF";
  }

  for (int i = 0; Xvr_private_keywords[i].keyword; i++) {
    if (Xvr_private_keywords[i].type == type) {
      return Xvr_private_keywords[i].keyword;
    }
  }

  return NULL;
}

Xvr_TokenType Xvr_private_findByKeyword(const char *keyword) {
  const int length = strlen(keyword);

  for (int i = 0; Xvr_private_keywords[i].keyword; i++) {
    if (!strncmp(keyword, Xvr_private_keywords[i].keyword, length)) {
      return Xvr_private_keywords[i].type;
    }
  }
  return XVR_TOKEN_EOF;
}
