#ifndef XVR_AST_H
#define XVR_AST_H

#include "xvr_common.h"

#include "xvr_bucket.h"
#include "xvr_value.h"

//each major type
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

//flags are handled differently by different types
typedef enum Xvr_AstFlag {
	XVR_AST_FLAG_NONE,

	//binary flags
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
	XVR_AST_FLAG_OR,

	//unary flags
	XVR_AST_FLAG_NEGATE,
	XVR_AST_FLAG_INCREMENT,
	XVR_AST_FLAG_DECREMENT,

	// XVR_AST_FLAG_TERNARY,
} Xvr_AstFlag;

//the root AST type
typedef union Xvr_Ast Xvr_Ast;

typedef struct Xvr_AstBlock {
	Xvr_AstType type;
	Xvr_Ast* child; //begin encoding the line
	Xvr_Ast* next; //'next' is either an AstBlock or null
	Xvr_Ast* tail; //'tail' - either points to the tail of the current list, or null; only used by the head of a list as an optimisation
} Xvr_AstBlock;

typedef struct Xvr_AstValue {
	Xvr_AstType type;
	Xvr_Value value;
} Xvr_AstValue;

typedef struct Xvr_AstUnary {
	Xvr_AstType type;
	Xvr_AstFlag flag;
	Xvr_Ast* child;
} Xvr_AstUnary;

typedef struct Xvr_AstBinary {
	Xvr_AstType type;
	Xvr_AstFlag flag;
	Xvr_Ast* left;
	Xvr_Ast* right;
} Xvr_AstBinary;

typedef struct Xvr_AstGroup {
	Xvr_AstType type;
	Xvr_Ast* child;
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

union Xvr_Ast {             //32 | 64 BITNESS
	Xvr_AstType type;       //4  | 4
	Xvr_AstBlock block;     //16 | 32
	Xvr_AstValue value;     //12 | 12
	Xvr_AstUnary unary;     //12 | 16
	Xvr_AstBinary binary;   //16 | 24
	Xvr_AstGroup group;     //8  | 16
	Xvr_AstPass pass;       //4  | 4
	Xvr_AstError error;     //4  | 4
	Xvr_AstEnd end;         //4  | 4
};                          //16 | 32

void Xvr_private_initAstBlock(Xvr_Bucket** bucket, Xvr_Ast** handle);
void Xvr_private_appendAstBlock(Xvr_Bucket** bucket, Xvr_Ast* block, Xvr_Ast* child);

void Xvr_private_emitAstValue(Xvr_Bucket** bucket, Xvr_Ast** handle, Xvr_Value value);
void Xvr_private_emitAstUnary(Xvr_Bucket** bucket, Xvr_Ast** handle, Xvr_AstFlag flag);
void Xvr_private_emitAstBinary(Xvr_Bucket** bucket, Xvr_Ast** handle,Xvr_AstFlag flag, Xvr_Ast* right);
void Xvr_private_emitAstGroup(Xvr_Bucket** bucket, Xvr_Ast** handle);

void Xvr_private_emitAstPass(Xvr_Bucket** bucket, Xvr_Ast** handle);
void Xvr_private_emitAstError(Xvr_Bucket** bucket, Xvr_Ast** handle);
void Xvr_private_emitAstEnd(Xvr_Bucket** bucket, Xvr_Ast** handle);

#endif // !XVR_AST_Hxvr_ast
