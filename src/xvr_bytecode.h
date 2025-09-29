#ifndef XVR_BYTECODE_H
#define XVR_BYTECODE_H

#include "xvr_ast.h"
#include "xvr_common.h"

typedef struct Xvr_Bytecode {
  unsigned char *ptr;
  unsigned int capacity;
  unsigned int count;
} Xvr_Bytecode;

XVR_API Xvr_Bytecode Xvr_compileBytecode(Xvr_Ast *ast);
XVR_API void Xvr_freeBytecode(Xvr_Bytecode bc);

#endif // !XVR_BYTECODE_H
