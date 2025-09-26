#include "xvr_bytecode.h"
#include "xvr_ast.h"
#include "xvr_common.h"
#include "xvr_memory.h"
#include "xvr_routine.h"
#include <stdio.h>
#include <string.h>

static void expand(Xvr_Bytecode *bc, int amount) {
  if (bc->count + amount > bc->capacity) {
    int oldCapacity = bc->capacity;

    while (bc->count + amount > bc->capacity) {
      bc->capacity = XVR_GROW_CAPACITY(bc->capacity);
    }

    bc->ptr = XVR_GROW_ARRAY(unsigned char, bc->ptr, oldCapacity, bc->capacity);
  }
}

static void emitByte(Xvr_Bytecode *bc, unsigned char byte) {
  expand(bc, 1);
  bc->ptr[bc->count++] = byte;
}

static void writeBytecodeHeader(Xvr_Bytecode *bc) {
  emitByte(bc, XVR_VERSION_MAJOR);
  emitByte(bc, XVR_VERSION_MINOR);
  emitByte(bc, XVR_VERSION_PATCH);

  const char *build = Xvr_private_version_build();
  int len = (int)strlen(build) + 1;

  if (len % 4 != 1) {
    len += 4 - (len % 4) + 1;
  }

  expand(bc, len);
  sprintf((char *)(bc->ptr + bc->count), "%.*s", len, build);
  bc->count += len;
}

static void writeBytecodeBody(Xvr_Bytecode *bc, Xvr_Ast *ast) {
  void *module = Xvr_compileRoutine(ast);

  int len = ((int *)module)[0];
  expand(bc, len);
  memcpy(bc->ptr + bc->count, module, len);
  bc->count += len;
}

Xvr_Bytecode Xvr_compileBytecode(Xvr_Ast *ast) {
  Xvr_Bytecode bc;

  bc.ptr = NULL;
  bc.capacity = 0;
  bc.count = 0;

  writeBytecodeHeader(&bc);
  writeBytecodeBody(&bc, ast);

  return bc;
}

void Xvr_freeBytecode(Xvr_Bytecode bc) {
  XVR_FREE_ARRAY(unsigned char, bc.ptr, bc.capacity);
}
