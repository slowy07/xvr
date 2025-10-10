#ifndef XVR_PARSER_H
#define XVR_PARSER_H

#include "xvr_common.h"

#include "xvr_ast.h"
#include "xvr_lexer.h"

typedef struct Xvr_Parser {
  Xvr_Lexer *lexer;

  // last two outputs
  Xvr_Token current;
  Xvr_Token previous;

  bool error;
  bool panic; // currently processing an error
  bool removeAssert;
} Xvr_Parser;

XVR_API void Xvr_bindParser(Xvr_Parser *parser, Xvr_Lexer *lexer);
XVR_API Xvr_Ast *Xvr_scanParser(Xvr_Bucket **bucket, Xvr_Parser *parser);
XVR_API void Xvr_resetParser(Xvr_Parser *parser);

XVR_API void Xvr_configureParser(Xvr_Parser *parser, bool removeAssert);

#endif // !XVR_PARSER_H
