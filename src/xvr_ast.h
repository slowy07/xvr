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
**/

#ifndef XVR_AST_H
#define XVR_AST_H

#include "xvr_common.h"
#include "xvr_memory.h"
#include "xvr_value.h"

typedef enum Xvr_AstType {
  XVR_AST_BLOCK,

  XVR_AST_VALUE,
  XVR_AST_UNARY,
  XVR_AST_BINARY,
  XVR_AST_GROUP,

  XVR_AST_PASS,
  XVR_AST_ERROR,
  XVR_AST_END,
} Xvr_AstType;

typedef enum Xvr_AstFlag {
  XVR_AST_FLAG_NONE,

  XVR_AST_FLAG_ADD,
  XVR_AST_FLAG_SUBTRACT,
  XVR_AST_FLAG_MULTIPLY,
  XVR_AST_FLAG_DIVIDE,
  XVR_AST_FLAG_MODULO,
  XVR_AST_FLAG_ASSIGN,
  XVR_AST_FLAG_ADD_ASSIGN,
  XVR_AST_FLAG_SUBTRACT_ASSIGN,
  XVR_AST_FLAG_MULTIPLY_ASSIGN,
  XVR_AST_FLAG_DIVIDE_ASSIGN,
  XVR_AST_FLAG_MODULO_ASSIGN,
  XVR_AST_FLAG_COMPARE_EQUAL,
  XVR_AST_FLAG_COMPARE_NOT,
  XVR_AST_FLAG_COMPARE_LESS,
  XVR_AST_FLAG_COMPARE_LESS_EQUAL,
  XVR_AST_FLAG_COMPARE_GREATER,
  XVR_AST_FLAG_COMPARE_GREATER_EQUAL,
  XVR_AST_FLAG_AND,
  XVR_AST_FLA_OR,

  XVR_AST_FLAG_NEGATE,
  XVR_AST_FLAG_INCREMENT,
  XVR_AST_FLAG_DECREMENT,
} Xvr_AstFlag;

typedef union Xvr_Ast Xvr_Ast;

void Xvr_private_initAstBlock(Xvr_Bucket **bucket, Xvr_Ast **handle);
void Xvr_private_appendAstBlock(Xvr_Bucket **bucket, Xvr_Ast **handle,
                                Xvr_Ast *child);

void Xvr_private_emitAstValue(Xvr_Bucket **bucket, Xvr_Ast **handle,
                              Xvr_Value value);
void Xvr_private_emitAstUnary(Xvr_Bucket **bucket, Xvr_Ast **handle,
                              Xvr_AstFlag flag);
void Xvr_private_emitAstBinary(Xvr_Bucket **bucket, Xvr_Ast **handle,
                               Xvr_AstFlag flag, Xvr_Ast *right);
void Xvr_private_emitAstGroup(Xvr_Bucket **bucket, Xvr_Ast **handle);
void Xvr_private_emitAstEnd(Xvr_Bucket **bucket, Xvr_Ast **handle);

void Xvr_private_emitAstPass(Xvr_Bucket **bucket, Xvr_Ast **handle);
void Xvr_private_emitAstError(Xvr_Bucket **bucket, Xvr_Ast **handle);

typedef struct Xvr_AstBlock {
  Xvr_AstType type;
  Xvr_Ast *child;
  Xvr_Ast *next;
  Xvr_Ast *tail;
} Xvr_AstBlock;

typedef struct Xvr_AstValue {
  Xvr_AstType type;
  Xvr_Value value;
} Xvr_AstValue;

typedef struct Xvr_AstUnary {
  Xvr_AstType type;
  Xvr_AstFlag flag;
  Xvr_Ast *child;
} Xvr_AstUnary;

typedef struct Xvr_AstBinary {
  Xvr_AstType type;
  Xvr_AstFlag flag;
  Xvr_Ast *left;
  Xvr_Ast *right;
} Xvr_AstBinary;

typedef struct Xvr_AstGroup {
  Xvr_AstType type;
  Xvr_Ast *child;
} Xvr_AstGroup;

typedef struct Xvr_AstPass {
  Xvr_AstType type;
} Xvr_AstPass;

typedef struct Xvr_AstError {
  Xvr_AstType type;
} Xvr_AstError;

typedef struct Xvr_AstEnd {
  Xvr_AstType type;
} Xvr_AstEnd;

union Xvr_Ast {
  Xvr_AstType type;
  Xvr_AstBlock block;
  Xvr_AstValue value;
  Xvr_AstUnary unary;
  Xvr_AstBinary binary;
  Xvr_AstGroup group;
  Xvr_AstPass pass;
  Xvr_AstError error;
  Xvr_AstEnd end;
};

#endif // !XVR_AST_H
