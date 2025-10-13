#ifndef XVR_ROUTINE_H
#define XVR_ROUTINE_H

#include "xvr_ast.h"
#include "xvr_common.h"

typedef struct Xvr_Routine {
  unsigned char *param; // c-string params in sequence (could be moved below the
                        // jump table?)
  unsigned int paramCapacity;
  unsigned int paramCount;

  unsigned char *code; // the instruction set
  unsigned int codeCapacity;
  unsigned int codeCount;

  unsigned int
      *jumps; // each 'jump' is the starting address of an element within 'data'
  unsigned int jumpsCapacity;
  unsigned int jumpsCount;

  unsigned char *data; //{type,val} tuples of data
  unsigned int dataCapacity;
  unsigned int dataCount;

  unsigned char *subs; // subroutines, recursively
  unsigned int subsCapacity;
  unsigned int subsCount;

  bool panic;
} Xvr_Routine;

XVR_API void *Xvr_compileRoutine(Xvr_Ast *ast);

#endif // !XVR_ROUTINE_H
