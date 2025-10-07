#include "xvr_bytecode.h"
#include "xvr_console_colors.h"

#include "xvr_routine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expand(Xvr_Bytecode *bc, unsigned int amount) {
  if (bc->count + amount > bc->capacity) {

    while (bc->count + amount > bc->capacity) { // expand as much as needed
      bc->capacity = bc->capacity < 8 ? 8 : bc->capacity * 2;
    }

    bc->ptr = realloc(bc->ptr, bc->capacity);

    if (bc->ptr == NULL) {
      fprintf(stderr,
              XVR_CC_ERROR "ERROR: Failed to allocate a 'Xvr_Bytecode' of %d "
                           "capacity\n" XVR_CC_RESET,
              (int)(bc->capacity));
      exit(1);
    }
  }
}


static void emitByte(Xvr_Bytecode *bc, unsigned char byte) {
  expand(bc, 1);
  bc->ptr[bc->count++] = byte;
}

// bytecode
static void writeBytecodeHeader(Xvr_Bytecode *bc) {
  emitByte(bc, XVR_VERSION_MAJOR);
  emitByte(bc, XVR_VERSION_MINOR);
  emitByte(bc, XVR_VERSION_PATCH);

  // check strlen for the build string
  const char *build = Xvr_private_version_build();
  size_t len = strlen(build) + 1;

  if (len % 4 != 1) {         // 1 to fill the 4th byte above
    len += 4 - (len % 4) + 1; // ceil
  }

  expand(bc, len);
  memcpy(bc->ptr + bc->count, build, len);
  bc->count += len;

  bc->ptr[bc->count] = '\0';
}

static void writeBytecodeBody(Xvr_Bytecode *bc, Xvr_Ast *ast) {
  void *module = Xvr_compileRoutine(ast);

  size_t len = (size_t)(((int *)module)[0]);

  expand(bc, len);
  memcpy(bc->ptr + bc->count, module, len);
  bc->count += len;
}

// exposed functions
Xvr_Bytecode Xvr_compileBytecode(Xvr_Ast *ast) {
  // setup
  Xvr_Bytecode bc;

  bc.ptr = NULL;
  bc.capacity = 0;
  bc.count = 0;

  // build
  writeBytecodeHeader(&bc);
  writeBytecodeBody(&bc, ast);

  return bc;
}

void Xvr_freeBytecode(Xvr_Bytecode bc) { free(bc.ptr); }
