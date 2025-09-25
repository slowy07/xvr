#include "xvr_routine.h"
#include "xvr_ast.h"
#include "xvr_console_color.h"
#include "xvr_memory.h"
#include "xvr_opcodes.h"
#include "xvr_value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EMIT_BYTE(rt, byte)                                                    \
  emitByte((void **)(&((*rt)->code)), &((*rt)->codeCapacity),                  \
           &((*rt)->codeCount), byte);
#define EMIT_INT(rt, code, byte)                                               \
  emitInt((void **)(&((*rt)->code)), &((*rt)->codeCapacity),                   \
          &((*rt)->codeCount), byte);
#define EMIT_FLOAT(rt, code, byte)                                             \
  emitFloat((void **)(&((*rt)->code)), &((*rt)->codeCapacity),                 \
            &((*rt)->codeCount), byte);

static void expand(void **handle, int *capacity, int *count, int amount) {
  while ((*count) + amount > (*capacity)) {
    int oldCapacity = (*capacity);

    (*capacity) = XVR_GROW_CAPACITY(oldCapacity);
    (*handle) =
        XVR_GROW_ARRAY(unsigned char, (*handle), oldCapacity, (*capacity));
  }
}

static void emitByte(void **handle, int *capacity, int *count,
                     unsigned char byte) {
  expand(handle, capacity, count, 1);
  ((unsigned char *)(*handle))[(*count)++] = byte;
}

static void emitInt(void **handle, int *capacity, int *count, int bytes) {
  char *ptr = (char *)&bytes;
  emitByte(handle, capacity, count, *(ptr++));
  emitByte(handle, capacity, count, *(ptr++));
  emitByte(handle, capacity, count, *(ptr++));
  emitByte(handle, capacity, count, *(ptr++));
}

static void emitFloat(void **handle, int *capacity, int *count, float bytes) {
  char *ptr = (char *)&bytes;
  emitByte(handle, capacity, count, *(ptr++));
  emitByte(handle, capacity, count, *(ptr++));
  emitByte(handle, capacity, count, *(ptr++));
  emitByte(handle, capacity, count, *(ptr++));
}

static void writeRoutineCode(Xvr_routine **rt, Xvr_Ast *ast);

static void writeInstructionValue(Xvr_routine **rt, Xvr_AstValue ast) {
  EMIT_BYTE(rt, XVR_OPCODE_READ);
  EMIT_BYTE(rt, ast.value.type);

  if (XVR_VALUE_IS_NULL(ast.value)) {
    // TODO
  } else if (XVR_VALUE_IS_BOOLEAN(ast.value)) {
    EMIT_BYTE(rt, XVR_VALUE_AS_BOOLEAN(ast.value));
  } else if (XVR_VALUE_IS_INTEGER(ast.value)) {
    EMIT_INT(rt, code, XVR_VALUE_AS_INTEGER(ast.value));
  } else if (XVR_VALUE_IS_FLOAT(ast.value)) {
    EMIT_FLOAT(rt, code, XVR_VALUE_AS_FLOAT(ast.value));
  } else {
    fprintf(stderr,
            XVR_CC_ERROR "invalid ast type: unknown type value\n" XVR_CC_RESET);
    exit(-1);
  }
}

static void writeInstructionUnary(Xvr_routine **rt, Xvr_AstUnary ast) {
  writeRoutineCode(rt, ast.child);

  if (ast.flag == XVR_AST_FLAG_NEGATE) {
    EMIT_BYTE(rt, XVR_OPCODE_NEGATE);
  } else {
    fprintf(stderr, XVR_CC_ERROR "invalid AST unary flag found\n" XVR_CC_RESET);
    exit(-1);
  }
}

static void writeInstructionBinary(Xvr_routine **rt, Xvr_AstBinary ast) {
  writeRoutineCode(rt, ast.left);
  writeRoutineCode(rt, ast.right);

  if (ast.flag == XVR_AST_FLAG_ADD) {
    EMIT_BYTE(rt, XVR_OPCODE_ADD);
  }

  if (ast.flag == XVR_AST_FLAG_SUBTRACT) {
    EMIT_BYTE(rt, XVR_OPCODE_SUBTRACT);
  }

  if (ast.flag == XVR_AST_FLAG_MULTIPLY) {
    EMIT_BYTE(rt, XVR_OPCODE_MULTIPLY);
  }

  if (ast.flag == XVR_AST_FLAG_DIVIDE) {
    EMIT_BYTE(rt, XVR_OPCODE_DIVIDE);
  }

  if (ast.flag == XVR_AST_FLAG_MODULO) {
    EMIT_BYTE(rt, XVR_OPCODE_MODULO);
  }

  if (ast.flag == XVR_AST_FLAG_COMPARE_EQUAL) {
    EMIT_BYTE(rt, XVR_OPCODE_COMPARE_EQUAL);
  }

  if (ast.flag == XVR_AST_FLAG_COMPARE_NOT) {
    EMIT_BYTE(rt, XVR_OPCODE_COMPARE_EQUAL);
    EMIT_BYTE(rt, XVR_OPCODE_NEGATE);
  }

  if (ast.flag == XVR_AST_FLAG_COMPARE_LESS) {
    EMIT_BYTE(rt, XVR_OPCODE_COMPARE_LESS);
  }

  if (ast.flag == XVR_AST_FLAG_COMPARE_LESS_EQUAL) {
    EMIT_BYTE(rt, XVR_OPCODE_COMPARE_LESS_EQUAL);
  }

  if (ast.flag == XVR_AST_FLAG_COMPARE_GREATER) {
    EMIT_BYTE(rt, XVR_OPCODE_COMPARE_GREATER)
  }

  if (ast.flag == XVR_AST_FLAG_AND) {
    EMIT_BYTE(rt, XVR_OPCODE_AND);
  } else {
    fprintf(stderr,
            XVR_CC_ERROR "invalid ast binary flag found\n" XVR_CC_RESET);
    exit(-1);
  }
}

static void writeRoutineCode(Xvr_routine **rt, Xvr_Ast *ast) {
  if (ast == NULL) {
    return;
  }

  switch (ast->type) {
  case XVR_AST_BLOCK:
    writeRoutineCode(rt, ast->block.child);
    writeRoutineCode(rt, ast->block.next);
    break;

  case XVR_AST_VALUE:
    writeInstructionValue(rt, ast->value);
    break;

  case XVR_AST_UNARY:
    writeInstructionUnary(rt, ast->unary);
    break;

  case XVR_AST_BINARY:
    writeInstructionBinary(rt, ast->binary);
    break;

  case XVR_AST_GROUP:
    fprintf(stderr, XVR_CC_ERROR
            "invalid AST type found: group shouldn't be used\n" XVR_CC_RESET);
    exit(-1);
    break;

  case XVR_AST_PASS:
    // PASSING point
    break;

  case XVR_AST_ERROR:
    fprintf(stderr, XVR_CC_ERROR
            "invalid AST type found: unknown error\n" XVR_CC_RESET);
    exit(-1);
    break;

  case XVR_AST_END:
    fprintf(stderr,
            XVR_CC_ERROR "invalid AST type found: unknown end\n" XVR_CC_RESET);
    exit(-1);
    break;
  }
}

static void *writeRoutine(Xvr_routine *rt, Xvr_Ast *ast) {
  writeRoutineCode(&rt, ast);
  EMIT_BYTE(&rt, XVR_OPCODE_RETURN);

  void *buffer = XVR_ALLOCATE(unsigned char, 16);
  int capacity = 0, count = 0;
  int codeAddr = 0;

  emitInt(&buffer, &capacity, &count, 0);
  emitInt(&buffer, &capacity, &count, rt->paramCount);
  emitInt(&buffer, &capacity, &count, rt->dataCount);
  emitInt(&buffer, &capacity, &count, rt->subsCount);

  if (rt->paramCount > 0) {
    emitInt((void **)&buffer, &capacity, &count, 0);
  }

  if (rt->codeCount > 0) {
    codeAddr = count;
    emitInt((void **)&buffer, &capacity, &count, 0);
  }

  if (rt->dataCount > 0) {
    emitInt((void **)&buffer, &capacity, &count, 0);
  }

  if (rt->subsCount > 0) {
    emitInt((void **)&buffer, &capacity, &count, 0);
  }

  if (rt->codeCount > 0) {
    expand(&buffer, &capacity, &count, rt->codeCount);
    memcpy((buffer + count), rt->code, rt->codeCount);

    ((int *)buffer)[codeAddr] = count;
    count += rt->codeCount;
  }

  ((int *)buffer)[0] = count;

  return buffer;
}

void *Xvr_compileRoutine(Xvr_Ast *ast) {
  Xvr_routine rt;

  rt.param = NULL;
  rt.paramCapacity = 0;
  rt.paramCount = 0;

  rt.code = NULL;
  rt.codeCapacity = 0;
  rt.codeCount = 0;

  rt.data = NULL;
  rt.dataCapacity = 0;
  rt.dataCount = 0;

  rt.subs = NULL;
  rt.subsCapacity = 0;
  rt.subsCount = 0;

  void *buffer = writeRoutine(&rt, ast);

  XVR_FREE_ARRAY(unsigned char, rt.param, rt.paramCapacity);
  XVR_FREE_ARRAY(unsigned char, rt.code, rt.codeCapacity);
  XVR_FREE_ARRAY(unsigned char, rt.data, rt.dataCapacity);
  XVR_FREE_ARRAY(int, rt.jumps, rt.jumpsCapacity);
  XVR_FREE_ARRAY(unsigned char, rt.data, rt.dataCapacity);
  XVR_FREE_ARRAY(unsigned char, rt.subs, rt.subsCapacity);

  return buffer;
}
