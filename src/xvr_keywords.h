#ifndef XVR_KEYWORDS_H
#define XVR_KEYWORDS_H

#include "xvr_common.h"
#include "xvr_token_types.h"

typedef struct {
  const Xvr_TokenType type;
  const char *keyword;
} Xvr_KeywordTypeTuple;

extern const Xvr_KeywordTypeTuple Xvr_private_keywords[];

// access
const char *Xvr_private_findKeywordByType(const Xvr_TokenType type);
Xvr_TokenType Xvr_private_findTypeByKeyword(const char *keyword);

#endif // !XVR_KEYWORDS_H
