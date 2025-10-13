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
