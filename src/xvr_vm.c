#include "xvr_vm.h"
#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"

#include "xvr_opcodes.h"
#include "xvr_print.h"
#include "xvr_stack.h"
#include "xvr_string.h"
#include "xvr_value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utilities
#define READ_BYTE(vm) vm->routine[vm->routineCounter++]

#define READ_UNSIGNED_INT(vm)                                                  \
  *((unsigned int *)(vm->routine + _read_postfix(&(vm->routineCounter), 4)))

#define READ_INT(vm)                                                           \
  *((int *)(vm->routine + _read_postfix(&(vm->routineCounter), 4)))

#define READ_FLOAT(vm)                                                         \
  *((float *)(vm->routine + _read_postfix(&(vm->routineCounter), 4)))

static inline int _read_postfix(unsigned int *ptr, int amount) {
  int ret = *ptr;
  *ptr += amount;
  return ret;
}

static inline void fix_alignment(Xvr_VM *vm) {
  if (vm->routineCounter % 4 != 0) {
    vm->routineCounter += (4 - vm->routineCounter % 4);
  }
}

// instruction handlers
static void processRead(Xvr_VM *vm) {
  Xvr_ValueType type = READ_BYTE(vm);

  Xvr_Value value = XVR_VALUE_FROM_NULL();

  switch (type) {
  case XVR_VALUE_NULL: {
    // No-op
    break;
  }

  case XVR_VALUE_BOOLEAN: {
    value = XVR_VALUE_FROM_BOOLEAN((bool)READ_BYTE(vm));
    break;
  }

  case XVR_VALUE_INTEGER: {
    fix_alignment(vm);
    value = XVR_VALUE_FROM_INTEGER(READ_INT(vm));
    break;
  }

  case XVR_VALUE_FLOAT: {
    fix_alignment(vm);
    value = XVR_VALUE_FROM_FLOAT(READ_FLOAT(vm));
    break;
  }

  case XVR_VALUE_STRING: {
    fix_alignment(vm);
    unsigned int jump =
        *(unsigned int *)(vm->routine + vm->jumpsAddr + READ_INT(vm));
    char *cstring = (char *)(vm->routine + vm->dataAddr + jump);
    value = XVR_VALUE_FROM_STRING(Xvr_createString(&vm->stringBucket, cstring));
    break;
  }

  case XVR_VALUE_ARRAY: {
    //
    // break;
  }

  case XVR_VALUE_DICTIONARY: {
    //
    // break;
  }

  case XVR_VALUE_FUNCTION: {
    //
    // break;
  }

  case XVR_VALUE_OPAQUE: {
    //
    // break;
  }

  default:
    fprintf(stderr,
            XVR_CC_ERROR
            "ERROR: Invalid value type %d found, exiting\n" XVR_CC_RESET,
            type);
    exit(-1);
  }

  // push onto the stack
  Xvr_pushStack(&vm->stack, value);

  // leave the counter in a good spot
  fix_alignment(vm);
}

static void processArithmetic(Xvr_VM *vm, Xvr_OpcodeType opcode) {
  Xvr_Value right = Xvr_popStack(&vm->stack);
  Xvr_Value left = Xvr_popStack(&vm->stack);

  // check types
  if ((!XVR_VALUE_IS_INTEGER(left) && !XVR_VALUE_IS_FLOAT(left)) ||
      (!XVR_VALUE_IS_INTEGER(right) && !XVR_VALUE_IS_FLOAT(right))) {
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Invalid types %d and %d passed to "
                         "processArithmetic, exiting\n" XVR_CC_RESET,
            left.type, right.type);
    exit(-1);
  }

  // check for divide by zero
  if (opcode == XVR_OPCODE_DIVIDE || opcode == XVR_OPCODE_MODULO) {
    if ((XVR_VALUE_IS_INTEGER(right) && XVR_VALUE_AS_INTEGER(right) == 0) ||
        (XVR_VALUE_IS_FLOAT(right) && XVR_VALUE_AS_FLOAT(right) == 0)) {
      fprintf(stderr, XVR_CC_ERROR
              "ERROR: Can't divide by zero, exiting\n" XVR_CC_RESET);
      exit(-1);
    }
  }

  // check for modulo by a float
  if (opcode == XVR_OPCODE_MODULO && XVR_VALUE_IS_FLOAT(right)) {
    fprintf(stderr, XVR_CC_ERROR
            "ERROR: Can't modulo by a float, exiting\n" XVR_CC_RESET);
    exit(-1);
  }

  // coerce ints into floats if needed
  if (XVR_VALUE_IS_INTEGER(left) && XVR_VALUE_IS_FLOAT(right)) {
    left = XVR_VALUE_FROM_FLOAT((float)XVR_VALUE_AS_INTEGER(left));
  } else if (XVR_VALUE_IS_FLOAT(left) && XVR_VALUE_IS_INTEGER(right)) {
    right = XVR_VALUE_FROM_FLOAT((float)XVR_VALUE_AS_INTEGER(right));
  }

  // apply operation
  Xvr_Value result = XVR_VALUE_FROM_NULL();

  if (opcode == XVR_OPCODE_ADD) {
    result = XVR_VALUE_IS_FLOAT(left)
                 ? XVR_VALUE_FROM_FLOAT(XVR_VALUE_AS_FLOAT(left) +
                                        XVR_VALUE_AS_FLOAT(right))
                 : XVR_VALUE_FROM_INTEGER(XVR_VALUE_AS_INTEGER(left) +
                                          XVR_VALUE_AS_INTEGER(right));
  } else if (opcode == XVR_OPCODE_SUBTRACT) {
    result = XVR_VALUE_IS_FLOAT(left)
                 ? XVR_VALUE_FROM_FLOAT(XVR_VALUE_AS_FLOAT(left) -
                                        XVR_VALUE_AS_FLOAT(right))
                 : XVR_VALUE_FROM_INTEGER(XVR_VALUE_AS_INTEGER(left) -
                                          XVR_VALUE_AS_INTEGER(right));
  } else if (opcode == XVR_OPCODE_MULTIPLY) {
    result = XVR_VALUE_IS_FLOAT(left)
                 ? XVR_VALUE_FROM_FLOAT(XVR_VALUE_AS_FLOAT(left) *
                                        XVR_VALUE_AS_FLOAT(right))
                 : XVR_VALUE_FROM_INTEGER(XVR_VALUE_AS_INTEGER(left) *
                                          XVR_VALUE_AS_INTEGER(right));
  } else if (opcode == XVR_OPCODE_DIVIDE) {
    result = XVR_VALUE_IS_FLOAT(left)
                 ? XVR_VALUE_FROM_FLOAT(XVR_VALUE_AS_FLOAT(left) /
                                        XVR_VALUE_AS_FLOAT(right))
                 : XVR_VALUE_FROM_INTEGER(XVR_VALUE_AS_INTEGER(left) /
                                          XVR_VALUE_AS_INTEGER(right));
  } else if (opcode == XVR_OPCODE_MODULO) {
    result = XVR_VALUE_FROM_INTEGER(XVR_VALUE_AS_INTEGER(left) %
                                    XVR_VALUE_AS_INTEGER(right));
  } else {
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Invalid opcode %d passed to "
                         "processArithmetic, exiting\n" XVR_CC_RESET,
            opcode);
    exit(-1);
  }

  // finally
  Xvr_pushStack(&vm->stack, result);
}

static void processComparison(Xvr_VM *vm, Xvr_OpcodeType opcode) {
  Xvr_Value right = Xvr_popStack(&vm->stack);
  Xvr_Value left = Xvr_popStack(&vm->stack);

  // most things can be equal, so handle it separately
  if (opcode == XVR_OPCODE_COMPARE_EQUAL) {
    bool equal = XVR_VALUES_ARE_EQUAL(left, right);

    // equality has an optional "negate" opcode within it's word
    if (READ_BYTE(vm) != XVR_OPCODE_NEGATE) {
      Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(equal));
    } else {
      Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(!equal));
    }

    return;
  }

  // coerce ints into floats if needed
  if (XVR_VALUE_IS_INTEGER(left) && XVR_VALUE_IS_FLOAT(right)) {
    left = XVR_VALUE_FROM_FLOAT((float)XVR_VALUE_AS_INTEGER(left));
  } else if (XVR_VALUE_IS_FLOAT(left) && XVR_VALUE_IS_INTEGER(right)) {
    right = XVR_VALUE_FROM_FLOAT((float)XVR_VALUE_AS_INTEGER(right));
  }

  // other opcodes
  if (opcode == XVR_OPCODE_COMPARE_LESS) {
    Xvr_pushStack(
        &vm->stack,
        XVR_VALUE_FROM_BOOLEAN(
            XVR_VALUE_IS_FLOAT(left)
                ? XVR_VALUE_AS_FLOAT(left) < XVR_VALUE_AS_FLOAT(right)
                : XVR_VALUE_AS_INTEGER(left) < XVR_VALUE_AS_INTEGER(right)));
  } else if (opcode == XVR_OPCODE_COMPARE_LESS_EQUAL) {
    Xvr_pushStack(
        &vm->stack,
        XVR_VALUE_FROM_BOOLEAN(
            XVR_VALUE_IS_FLOAT(left)
                ? XVR_VALUE_AS_FLOAT(left) <= XVR_VALUE_AS_FLOAT(right)
                : XVR_VALUE_AS_INTEGER(left) <= XVR_VALUE_AS_INTEGER(right)));
  } else if (opcode == XVR_OPCODE_COMPARE_GREATER) {
    Xvr_pushStack(
        &vm->stack,
        XVR_VALUE_FROM_BOOLEAN(
            XVR_VALUE_IS_FLOAT(left)
                ? XVR_VALUE_AS_FLOAT(left) > XVR_VALUE_AS_FLOAT(right)
                : XVR_VALUE_AS_INTEGER(left) > XVR_VALUE_AS_INTEGER(right)));
  } else if (opcode == XVR_OPCODE_COMPARE_GREATER_EQUAL) {
    Xvr_pushStack(
        &vm->stack,
        XVR_VALUE_FROM_BOOLEAN(
            XVR_VALUE_IS_FLOAT(left)
                ? XVR_VALUE_AS_FLOAT(left) >= XVR_VALUE_AS_FLOAT(right)
                : XVR_VALUE_AS_INTEGER(left) >= XVR_VALUE_AS_INTEGER(right)));
  } else {
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Invalid opcode %d passed to "
                         "processComparison, exiting\n" XVR_CC_RESET,
            opcode);
    exit(-1);
  }
}

static void processLogical(Xvr_VM *vm, Xvr_OpcodeType opcode) {
  if (opcode == XVR_OPCODE_AND) {
    Xvr_Value right = Xvr_popStack(&vm->stack);
    Xvr_Value left = Xvr_popStack(&vm->stack);

    Xvr_pushStack(&vm->stack,
                  XVR_VALUE_FROM_BOOLEAN(XVR_VALUE_IS_TRUTHY(left) &&
                                         XVR_VALUE_IS_TRUTHY(right)));
  } else if (opcode == XVR_OPCODE_OR) {
    Xvr_Value right = Xvr_popStack(&vm->stack);
    Xvr_Value left = Xvr_popStack(&vm->stack);

    Xvr_pushStack(&vm->stack,
                  XVR_VALUE_FROM_BOOLEAN(XVR_VALUE_IS_TRUTHY(left) ||
                                         XVR_VALUE_IS_TRUTHY(right)));
  } else if (opcode == XVR_OPCODE_TRUTHY) {
    Xvr_Value top = Xvr_popStack(&vm->stack);

    Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(XVR_VALUE_IS_TRUTHY(top)));
  } else if (opcode == XVR_OPCODE_NEGATE) {
    Xvr_Value top = Xvr_popStack(&vm->stack);

    Xvr_pushStack(&vm->stack,
                  XVR_VALUE_FROM_BOOLEAN(!XVR_VALUE_IS_TRUTHY(top)));
  } else {
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Invalid opcode %d passed to processLogical, "
                         "exiting\n" XVR_CC_RESET,
            opcode);
    exit(-1);
  }
}

static void processPrint(Xvr_VM *vm) {
  Xvr_Value value = Xvr_popStack(&vm->stack);

  switch (value.type) {
  case XVR_VALUE_NULL:
    Xvr_print("null");
    break;

  case XVR_VALUE_BOOLEAN:
    Xvr_print(XVR_VALUE_AS_BOOLEAN(value) ? "true" : "false");
    break;

  case XVR_VALUE_INTEGER: {
    char buffer[16];
    sprintf(buffer, "%d", XVR_VALUE_AS_INTEGER(value));
    Xvr_print(buffer);
    break;
  }

  case XVR_VALUE_FLOAT: {
    char buffer[16];
    sprintf(buffer, "%f", XVR_VALUE_AS_FLOAT(value));
    Xvr_print(buffer);
    break;
  }

  case XVR_VALUE_STRING: {
    Xvr_String *str = XVR_VALUE_AS_STRING(value);
    if (str->type == XVR_STRING_NODE) {
      char *buffer = Xvr_getStringRawBuffer(str);
      Xvr_print(buffer);
      free(buffer);
    } else if (str->type == XVR_STRING_LEAF) {
      Xvr_print(str->as.leaf.data);
    } else if (str->type == XVR_STRING_NAME) {
      Xvr_print(str->as.name.data);
    }
    break;
  }
  case XVR_VALUE_ARRAY:
  case XVR_VALUE_DICTIONARY:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
    fprintf(
        stderr,
        XVR_CC_ERROR
        "Error: unknown value type %d passed to processPrint\n" XVR_CC_RESET,
        value.type);
    exit(-1);
  }
}

static void processConcat(Xvr_VM *vm) {
  Xvr_Value right = Xvr_popStack(&vm->stack);
  Xvr_Value left = Xvr_popStack(&vm->stack);

  if (!XVR_VALUE_IS_STRING(left)) {
    Xvr_error("failed to concatenate a value that is not a string");
    return;
  }

  if (!XVR_VALUE_IS_STRING(left)) {
    Xvr_error("failed to concatenate value that is not a string");
    return;
  }

  Xvr_String *result = Xvr_concatStrings(
      &vm->stringBucket, XVR_VALUE_AS_STRING(left), XVR_VALUE_AS_STRING(right));
  Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_STRING(result));
}

static void process(Xvr_VM *vm) {
  while (true) {
    Xvr_OpcodeType opcode = READ_BYTE(vm);

    switch (opcode) {
    case XVR_OPCODE_READ:
      processRead(vm);
      break;

    case XVR_OPCODE_ADD:
    case XVR_OPCODE_SUBTRACT:
    case XVR_OPCODE_MULTIPLY:
    case XVR_OPCODE_DIVIDE:
    case XVR_OPCODE_MODULO:
      processArithmetic(vm, opcode);
      break;

    case XVR_OPCODE_COMPARE_EQUAL:
    case XVR_OPCODE_COMPARE_LESS:
    case XVR_OPCODE_COMPARE_LESS_EQUAL:
    case XVR_OPCODE_COMPARE_GREATER:
    case XVR_OPCODE_COMPARE_GREATER_EQUAL:
      processComparison(vm, opcode);
      break;

    case XVR_OPCODE_AND:
    case XVR_OPCODE_OR:
    case XVR_OPCODE_TRUTHY:
    case XVR_OPCODE_NEGATE:
      processLogical(vm, opcode);
      break;

    case XVR_OPCODE_RETURN:
      return;

    case XVR_OPCODE_PRINT:
      processPrint(vm);
      break;

    case XVR_OPCODE_CONCAT:
      processConcat(vm);
      break;

    case XVR_OPCODE_LOAD:
    case XVR_OPCODE_LOAD_LONG:
    case XVR_OPCODE_DECLARE:
    case XVR_OPCODE_ASSIGN:
    case XVR_OPCODE_ACCESS:
    case XVR_OPCODE_PASS:
    case XVR_OPCODE_ERROR:
    case XVR_OPCODE_EOF:
      fprintf(stderr,
              XVR_CC_ERROR
              "ERROR: Invalid opcode %d found, exiting\n" XVR_CC_RESET,
              opcode);
      exit(-1);
    }

    // prepare for the next instruction
    fix_alignment(vm);
  }
}

void Xvr_initVM(Xvr_VM *vm) {
  vm->stack = NULL;
  vm->stringBucket = NULL;
  Xvr_resetVM(vm);
}

// exposed functions
void Xvr_bindVM(Xvr_VM *vm, unsigned char *bytecode) {
  if (bytecode[0] != XVR_VERSION_MAJOR || bytecode[1] > XVR_VERSION_MINOR) {
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Wrong bytecode version found: expected "
                         "%d.%d.%d found %d.%d.%d, exiting\n" XVR_CC_RESET,
            XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
            bytecode[0], bytecode[1], bytecode[2]);
    exit(-1);
  }

  if (bytecode[2] != XVR_VERSION_PATCH) {
    fprintf(stderr,
            XVR_CC_WARN "WARNING: Wrong bytecode version found: expected "
                        "%d.%d.%d found %d.%d.%d, continuing\n" XVR_CC_RESET,
            XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
            bytecode[0], bytecode[1], bytecode[2]);
  }

  if (strcmp((char *)(bytecode + 3), XVR_VERSION_BUILD) != 0) {
    fprintf(stderr,
            XVR_CC_WARN "WARNING: Wrong bytecode build info found: expected "
                        "'%s' found '%s', continuing\n" XVR_CC_RESET,
            XVR_VERSION_BUILD, (char *)(bytecode + 3));
  }

  int offset = 3 + strlen(XVR_VERSION_BUILD) + 1;
  if (offset % 4 != 0) {
    offset += 4 - (offset % 4); // ceil
  }

  Xvr_bindVMToRoutine(vm, bytecode + offset);

  vm->bc = bytecode;
}

void Xvr_bindVMToRoutine(Xvr_VM *vm, unsigned char *routine) {
  vm->routine = routine;

  // read the header metadata
  vm->routineSize = READ_UNSIGNED_INT(vm);
  vm->paramSize = READ_UNSIGNED_INT(vm);
  vm->jumpsSize = READ_UNSIGNED_INT(vm);
  vm->dataSize = READ_UNSIGNED_INT(vm);
  vm->subsSize = READ_UNSIGNED_INT(vm);

  // read the header addresses
  if (vm->paramSize > 0) {
    vm->paramAddr = READ_UNSIGNED_INT(vm);
  }

  vm->codeAddr = READ_UNSIGNED_INT(vm); // required

  if (vm->jumpsSize > 0) {
    vm->jumpsAddr = READ_UNSIGNED_INT(vm);
  }

  if (vm->dataSize > 0) {
    vm->dataAddr = READ_UNSIGNED_INT(vm);
  }

  if (vm->subsSize > 0) {
    vm->subsAddr = READ_UNSIGNED_INT(vm);
  }

  vm->stack = Xvr_allocateStack();
  vm->stringBucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
}

void Xvr_runVM(Xvr_VM *vm) {
  vm->routineCounter = vm->codeAddr;

  // begin
  process(vm);
}

void Xvr_freeVM(Xvr_VM *vm) {
  // clear the stack
  Xvr_freeStack(vm->stack);
  free(vm->bc);
  Xvr_resetVM(vm);
}

void Xvr_resetVM(Xvr_VM *vm) {
  vm->bc = NULL;

  vm->routine = NULL;
  vm->routineSize = 0;

  vm->paramSize = 0;
  vm->jumpsSize = 0;
  vm->dataSize = 0;
  vm->subsSize = 0;

  vm->paramAddr = 0;
  vm->codeAddr = 0;
  vm->jumpsAddr = 0;
  vm->dataAddr = 0;
  vm->subsAddr = 0;

  vm->routineCounter = 0;
}
