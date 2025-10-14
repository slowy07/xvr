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

#ifndef XVR_AST_H
#define XVR_AST_H

#include "xvr_common.h"

#include "xvr_bucket.h"
#include "xvr_string.h"
#include "xvr_value.h"

// each major type
typedef enum Xvr_AstType {
  XVR_AST_BLOCK,

  XVR_AST_VALUE,
  XVR_AST_UNARY,
  XVR_AST_BINARY,
  XVR_AST_COMPARE,
  XVR_AST_GROUP,
  XVR_AST_COMPOUND,

  XVR_AST_ASSERT,
  XVR_AST_IF_THEN_ELSE,
  XVR_AST_WHILE_THEN,
  XVR_AST_BREAK,
  XVR_AST_CONTINUE,
  XVR_AST_PRINT,

  XVR_AST_VAR_DECLARE,
  XVR_AST_VAR_ASSIGN,
  XVR_AST_VAR_ACCESS,

  XVR_AST_PASS,
  XVR_AST_ERROR,
  XVR_AST_END,
} Xvr_AstType;

// flags are handled differently by different types
typedef enum Xvr_AstFlag {
  XVR_AST_FLAG_NONE = 0,

  // binary flags
  XVR_AST_FLAG_ADD = 1,
  XVR_AST_FLAG_SUBTRACT = 2,
  XVR_AST_FLAG_MULTIPLY = 3,
  XVR_AST_FLAG_DIVIDE = 4,
  XVR_AST_FLAG_MODULO = 5,

  XVR_AST_FLAG_ASSIGN = 10,
  XVR_AST_FLAG_ADD_ASSIGN = 11,
  XVR_AST_FLAG_SUBTRACT_ASSIGN = 12,
  XVR_AST_FLAG_MULTIPLY_ASSIGN = 13,
  XVR_AST_FLAG_DIVIDE_ASSIGN = 14,
  XVR_AST_FLAG_MODULO_ASSIGN = 15,

  XVR_AST_FLAG_COMPARE_EQUAL = 20,
  XVR_AST_FLAG_COMPARE_NOT = 21,
  XVR_AST_FLAG_COMPARE_LESS = 22,
  XVR_AST_FLAG_COMPARE_LESS_EQUAL = 23,
  XVR_AST_FLAG_COMPARE_GREATER = 25,
  XVR_AST_FLAG_COMPARE_GREATER_EQUAL = 25,

  XVR_AST_FLAG_COMPOUND_COLLECTION = 30,
  XVR_AST_FLAG_COMPOUND_INDEX = 31,

  XVR_AST_FLAG_AND = 40,
  XVR_AST_FLAG_OR = 41,
  XVR_AST_FLAG_CONCAT = 42,

  // unary flags
  XVR_AST_FLAG_NEGATE = 43,
  XVR_AST_FLAG_INCREMENT = 44,
  XVR_AST_FLAG_DECREMENT = 45,

  // XVR_AST_FLAG_TERNARY,
} Xvr_AstFlag;

// the root AST type
typedef union Xvr_Ast Xvr_Ast;

typedef struct Xvr_AstBlock {
  Xvr_AstType type;
  bool innerScope;
  Xvr_Ast *child; // begin encoding the line
  Xvr_Ast *next;  //'next' is either an AstBlock or null
  Xvr_Ast *tail;  //'tail' - either points to the tail of the current list, or
                  // null; only used by the head of a list as an optimisation
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

typedef struct Xvr_AstCompare {
  Xvr_AstType type;
  Xvr_AstFlag flag;
  Xvr_Ast *left;
  Xvr_Ast *right;
} Xvr_AstCompare;

typedef struct Xvr_AstGroup {
  Xvr_AstType type;
  Xvr_Ast *child;
} Xvr_AstGroup;

typedef struct Xvr_AstCompound {
  Xvr_AstType type;
  Xvr_AstFlag flag;
  Xvr_Ast *left;
  Xvr_Ast *right;
} Xvr_AstCompound;

typedef struct Xvr_AstAssert {
  Xvr_AstType type;
  Xvr_Ast *child;
  Xvr_Ast *message;
} Xvr_AstAssert;

typedef struct Xvr_AstIfThenElse {
  Xvr_AstType type;
  Xvr_Ast *condBranch;
  Xvr_Ast *thenBranch;
  Xvr_Ast *elseBranch;
} Xvr_AstIfThenElse;

typedef struct Xvr_AstWhileThen {
  Xvr_AstType type;
  Xvr_Ast *condBranch;
  Xvr_Ast *thenBranch;
} Xvr_AstWhileThen;

typedef struct Xvr_AstBreak {
  Xvr_AstType type;
} Xvr_AstBreak;

typedef struct Xvr_AstContinue {
  Xvr_AstType type;
} Xvr_AstContinue;

typedef struct Xvr_AstPrint {
  Xvr_AstType type;
  Xvr_Ast *child;
} Xvr_AstPrint;

typedef struct Xvr_AstVarDeclare {
  Xvr_AstType type;
  Xvr_String *name;
  Xvr_Ast *expr;
} Xvr_AstVarDeclare;

typedef struct Xvr_AstVarAssign {
  Xvr_AstType type;
  Xvr_AstFlag flag;
  Xvr_String *name;
  Xvr_Ast *expr;
} Xvr_AstVarAssign;

typedef struct Xvr_AstVarAccess {
  Xvr_AstType type;
  Xvr_String *name;
} Xvr_AstVarAccess;

typedef struct Xvr_AstPass {
  Xvr_AstType type;
} Xvr_AstPass;

typedef struct Xvr_AstError {
  Xvr_AstType type;
} Xvr_AstError;

typedef struct Xvr_AstEnd {
  Xvr_AstType type;
} Xvr_AstEnd;

union Xvr_Ast {                 // 32 | 64 BITNESS
  Xvr_AstType type;             // 4  | 4
  Xvr_AstBlock block;           // 16 | 32
  Xvr_AstValue value;           // 12 | 24
  Xvr_AstUnary unary;           // 12 | 16
  Xvr_AstBinary binary;         // 16 | 24
  Xvr_AstCompare compare;       // 16 | 24
  Xvr_AstGroup group;           // 8  | 16
  Xvr_AstCompound compound;     // 16 | 24
  Xvr_AstVarDeclare varDeclare; // 16 | 24
  Xvr_AstVarAssign varAssign;   // 16 | 24
  Xvr_AstVarAccess varAccess;   // 8 | 16
  Xvr_AstAssert assert;         // 16 | 24
  Xvr_AstIfThenElse ifThenElse; // 16 | 32
  Xvr_AstWhileThen whileThen;   // 16 | 24
  Xvr_AstBreak breakPoint;      // 4 | 4
  Xvr_AstContinue continuePoint;   // 4 | 4
  Xvr_AstPrint print;           // 8 | 16
  Xvr_AstPass pass;             // 4  | 4
  Xvr_AstError error;           // 4  | 4
  Xvr_AstEnd end;               // 4  | 4
}; // 16 | 32

void Xvr_private_initAstBlock(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle);
void Xvr_private_appendAstBlock(Xvr_Bucket **bucketHandle, Xvr_Ast *block,
                                Xvr_Ast *child);

void Xvr_private_emitAstValue(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle,
                              Xvr_Value value);
void Xvr_private_emitAstUnary(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle,
                              Xvr_AstFlag flag);
void Xvr_private_emitAstBinary(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle,
                               Xvr_AstFlag flag, Xvr_Ast *right);

void Xvr_private_emitAstCompare(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle,
                                Xvr_AstFlag flag, Xvr_Ast *right);

void Xvr_private_emitAstGroup(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle);
void Xvr_private_emitAstCompound(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle,
                                 Xvr_AstFlag flag, Xvr_Ast *right);
void Xvr_private_emitAstAssert(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle,
                               Xvr_Ast *child, Xvr_Ast *msg);
void Xvr_private_emitAstIfThenElse(Xvr_Bucket **bucketHandle,
                                   Xvr_Ast **astHandle, Xvr_Ast *condBranch,
                                   Xvr_Ast *thenBranch, Xvr_Ast *elseBranch);
void Xvr_private_emitAstWhileThen(Xvr_Bucket **bucketHandle,
                                  Xvr_Ast **astHandle, Xvr_Ast *condBranch,
                                  Xvr_Ast *thenBranch);
void Xvr_private_emitAstBreak(Xvr_Bucket** bucketHandle, Xvr_Ast** astHandle);
void Xvr_private_emitAstContinue(Xvr_Bucket** bucketHandle, Xvr_Ast** astHandle);
void Xvr_private_emitAstPrint(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle);

void Xvr_private_emitAstVariableDeclaration(Xvr_Bucket **bucketHandle,
                                            Xvr_Ast **astHandle,
                                            Xvr_String *name, Xvr_Ast *expr);

void Xvr_private_emitAstVariableAssignment(Xvr_Bucket **bucketHandle,
                                           Xvr_Ast **astHandle,
                                           Xvr_String *name, Xvr_AstFlag flag,
                                           Xvr_Ast *expr);

void Xvr_private_emitAstVariableAccess(Xvr_Bucket **bucketHandle,
                                       Xvr_Ast **astHandle, Xvr_String *name);

void Xvr_private_emitAstPass(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle);
void Xvr_private_emitAstError(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle);
void Xvr_private_emitAstEnd(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle);

#endif // !XVR_AST_Hxvr_ast
