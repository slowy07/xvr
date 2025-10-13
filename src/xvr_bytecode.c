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

  if (module == NULL) {
    return;
  }

  size_t len = (size_t)(((int *)module)[0]);

  expand(bc, len);
  memcpy(bc->ptr + bc->count, module, len);
  bc->count += len;
  bc->moduleCount++;
}

// exposed functions
Xvr_Bytecode Xvr_compileBytecode(Xvr_Ast *ast) {
  // setup
  Xvr_Bytecode bc;

  bc.ptr = NULL;
  bc.capacity = 0;
  bc.count = 0;

  bc.moduleCount = 0;

  // build
  writeBytecodeHeader(&bc);
  writeBytecodeBody(&bc, ast);

  return bc;
}

void Xvr_freeBytecode(Xvr_Bytecode bc) { free(bc.ptr); }
