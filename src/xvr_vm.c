#include "xvr_vm.h"
#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"

#include "xvr_opcodes.h"
#include "xvr_print.h"
#include "xvr_scope.h"
#include "xvr_stack.h"
#include "xvr_string.h"
#include "xvr_value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utilities
#define READ_BYTE(vm) vm->routine[vm->routineCounter++]

#define READ_UNSIGNED_INT(vm)                                                  \
  *((unsigned int *)(vm->routine + readPostfixUtil(&(vm->routineCounter), 4)))

#define READ_INT(vm)                                                           \
  *((int *)(vm->routine + readPostfixUtil(&(vm->routineCounter), 4)))

#define READ_FLOAT(vm)                                                         \
  *((float *)(vm->routine + readPostfixUtil(&(vm->routineCounter), 4)))

static inline int readPostfixUtil(unsigned int *ptr, int amount) {
  int ret = *ptr;
  *ptr += amount;
  return ret;
}

static inline void fixAlignment(Xvr_VM *vm) {
  if (vm->routineCounter % 4 != 0) {
    vm->routineCounter = (vm->routineCounter + 3) & ~0b11;
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
    fixAlignment(vm);
    value = XVR_VALUE_FROM_INTEGER(READ_INT(vm));
    break;
  }

  case XVR_VALUE_FLOAT: {
    fixAlignment(vm);
    value = XVR_VALUE_FROM_FLOAT(READ_FLOAT(vm));
    break;
  }

  case XVR_VALUE_STRING: {
    enum Xvr_StringType stringType = READ_BYTE(vm);
    int len = (int)READ_BYTE(vm);
    unsigned int jump = vm->routine[vm->jumpsAddr + READ_INT(vm)];
    char *cstring = (char *)(vm->routine + vm->dataAddr + jump);

    if (stringType == XVR_STRING_LEAF) {
      value =
          XVR_VALUE_FROM_STRING(Xvr_createString(&vm->stringBucket, cstring));
    } else if (stringType == XVR_STRING_NAME) {
      Xvr_ValueType valueType = XVR_VALUE_UNKNOWN;
      value = XVR_VALUE_FROM_STRING(Xvr_createNameStringLength(
          &vm->stringBucket, cstring, len, valueType));
    } else {
      Xvr_error("invalid string type found");
    }

    break;
  }

  case XVR_VALUE_ARRAY: {
    //
    // break;
  }

  case XVR_VALUE_TABLE: {
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

  case XVR_VALUE_TYPE: {
    //
    // berak;
  }

  case XVR_VALUE_ANY: {
    //
    // break;
  }

  case XVR_VALUE_UNKNOWN: {
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
  fixAlignment(vm);
}

static void processDeclare(Xvr_VM *vm) {
  Xvr_ValueType type = READ_BYTE(vm); // variable type
  unsigned int len = READ_BYTE(vm);   // name length
  fixAlignment(vm);                   // one spare byte

  unsigned int jump =
      *(unsigned int *)(vm->routine + vm->jumpsAddr + READ_INT(vm));
  char *cstring = (char *)(vm->routine + vm->dataAddr + jump);
  Xvr_String *name =
      Xvr_createNameStringLength(&vm->stringBucket, cstring, len, type);
  Xvr_Value value = Xvr_popStack(&vm->stack);
  Xvr_declareScope(vm->scope, name, value);
  Xvr_freeString(name);
}

static void processAssign(Xvr_VM *vm) {
  Xvr_Value value = Xvr_popStack(&vm->stack);
  Xvr_Value name = Xvr_popStack(&vm->stack);

  if (!XVR_VALUE_IS_STRING(name) &&
      XVR_VALUE_AS_STRING(name)->type != XVR_STRING_NAME) {
    Xvr_error("invalid assignment target");
    return;
  }

  Xvr_assignScope(vm->scope, XVR_VALUE_AS_STRING(name), value);
  Xvr_freeValue(name);
}

static void processAccess(Xvr_VM *vm) {
  Xvr_Value name = Xvr_popStack(&vm->stack);

  if (!XVR_VALUE_IS_STRING(name) &&
      XVR_VALUE_AS_STRING(name)->type != XVR_STRING_NAME) {
    Xvr_error("invalid access target");
    return;
  }

  Xvr_Value value = Xvr_accessScope(vm->scope, XVR_VALUE_AS_STRING(name));
  Xvr_pushStack(&vm->stack, value);

  Xvr_freeValue(name);
}

static void processDuplicate(Xvr_VM *vm) {
  Xvr_Value value = Xvr_copyValue(Xvr_peekStack(&vm->stack));
  Xvr_pushStack(&vm->stack, value);
  Xvr_freeValue(value);

  Xvr_OpcodeType squeezed = READ_BYTE(vm);
  if (squeezed == XVR_OPCODE_ACCESS) {
    processAccess(vm);
  }
}

static void processArithmetic(Xvr_VM *vm, Xvr_OpcodeType opcode) {
  Xvr_Value right = Xvr_popStack(&vm->stack);
  Xvr_Value left = Xvr_popStack(&vm->stack);

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

  Xvr_OpcodeType squeezed = READ_BYTE(vm);
  if (squeezed == XVR_OPCODE_ASSIGN) {
    processAssign(vm);
  }
}

static void processComparison(Xvr_VM *vm, Xvr_OpcodeType opcode) {
  Xvr_Value right = Xvr_popStack(&vm->stack);
  Xvr_Value left = Xvr_popStack(&vm->stack);

  // most things can be equal, so handle it separately
  if (opcode == XVR_OPCODE_COMPARE_EQUAL) {
    bool equal = Xvr_checkValuesAreEqual(left, right);

    // equality has an optional "negate" opcode within it's word
    if (READ_BYTE(vm) != XVR_OPCODE_NEGATE) {
      Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(equal));
    } else {
      Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(!equal));
    }

    return;
  }

  if (Xvr_checkValuesAreCompareable(left, right) == false) {
    fprintf(stderr,
            XVR_CC_ERROR
            "Error: can't compare value types %d and %d\n" XVR_CC_RESET,
            left.type, right.type);
    exit(-1);
  }

  int comparison = Xvr_compareValues(left, right);

  if (opcode == XVR_OPCODE_COMPARE_LESS && comparison < 0) {
    Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(true));
  } else if (opcode == XVR_OPCODE_COMPARE_LESS_EQUAL &&
             (comparison < 0 || comparison == 0)) {
    Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(true));
  } else if (opcode == XVR_OPCODE_COMPARE_GREATER && comparison > 0) {
    Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(true));
  } else if (opcode == XVR_OPCODE_COMPARE_GREATER_EQUAL &&
             (comparison > 0 || comparison == 0)) {
    Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(true));
  } else {
    Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(false));
  }
}

static void processLogical(Xvr_VM *vm, Xvr_OpcodeType opcode) {
  if (opcode == XVR_OPCODE_AND) {
    Xvr_Value right = Xvr_popStack(&vm->stack);
    Xvr_Value left = Xvr_popStack(&vm->stack);

    Xvr_pushStack(&vm->stack,
                  XVR_VALUE_FROM_BOOLEAN(Xvr_checkValueIsTruthy(left) &&
                                         Xvr_checkValueIsTruthy(right)));
  } else if (opcode == XVR_OPCODE_OR) {
    Xvr_Value right = Xvr_popStack(&vm->stack);
    Xvr_Value left = Xvr_popStack(&vm->stack);

    Xvr_pushStack(&vm->stack,
                  XVR_VALUE_FROM_BOOLEAN(Xvr_checkValueIsTruthy(left) ||
                                         Xvr_checkValueIsTruthy(right)));
  } else if (opcode == XVR_OPCODE_TRUTHY) {
    Xvr_Value top = Xvr_popStack(&vm->stack);

    Xvr_pushStack(&vm->stack,
                  XVR_VALUE_FROM_BOOLEAN(!Xvr_checkValueIsTruthy(top)));
  } else if (opcode == XVR_OPCODE_NEGATE) {
    Xvr_Value top = Xvr_popStack(&vm->stack);

    Xvr_pushStack(&vm->stack,
                  XVR_VALUE_FROM_BOOLEAN(!Xvr_checkValueIsTruthy(top)));
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
  case XVR_VALUE_TABLE:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
  case XVR_VALUE_TYPE:
  case XVR_VALUE_ANY:
  case XVR_VALUE_UNKNOWN:
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Unknown value type %d passed to processPrint, "
                         "exiting\n" XVR_CC_RESET,
            value.type);
    exit(-1);
  }
}

static void processConcat(Xvr_VM *vm) {
  Xvr_Value right = Xvr_popStack(&vm->stack);
  Xvr_Value left = Xvr_popStack(&vm->stack);

  if (!XVR_VALUE_IS_STRING(left) || !XVR_VALUE_IS_STRING(right)) {
    Xvr_error("failed to concatenate a value that is not a string");
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

    case XVR_OPCODE_DECLARE:
      processDeclare(vm);
      break;

    case XVR_OPCODE_ASSIGN:
      processAssign(vm);
      break;

    case XVR_OPCODE_ACCESS:
      processAccess(vm);
      break;

    case XVR_OPCODE_DUPLICATE:
      processDuplicate(vm);
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

    case XVR_OPCODE_SCOPE_PUSH:
      vm->scope = Xvr_pushScope(&vm->scopeBucket, vm->scope);
      break;

    case XVR_OPCODE_SCOPE_POP:
      vm->scope = Xvr_popScope(vm->scope);
      break;

    case XVR_OPCODE_PRINT:
      processPrint(vm);
      break;

    case XVR_OPCODE_CONCAT:
      processConcat(vm);
      break;

    case XVR_OPCODE_PASS:
    case XVR_OPCODE_ERROR:
    case XVR_OPCODE_EOF:
      fprintf(stderr,
              XVR_CC_ERROR
              "ERROR: Invalid opcode %d found, exiting\n" XVR_CC_RESET,
              opcode);
      exit(-1);
    }

    fixAlignment(vm);
  }
}

void Xvr_initVM(Xvr_VM *vm) {
  vm->stringBucket = NULL;
  vm->scopeBucket = NULL;
  vm->stack = NULL;
  vm->scope = NULL;

  Xvr_resetVM(vm);
}

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
    offset += 4 - (offset % 4);
  }

  Xvr_bindVMToRoutine(vm, bytecode + offset);

  vm->bc = bytecode;
}

void Xvr_bindVMToRoutine(Xvr_VM *vm, unsigned char *routine) {
  vm->routine = routine;

  vm->routineSize = READ_UNSIGNED_INT(vm);
  vm->paramSize = READ_UNSIGNED_INT(vm);
  vm->jumpsSize = READ_UNSIGNED_INT(vm);
  vm->dataSize = READ_UNSIGNED_INT(vm);
  vm->subsSize = READ_UNSIGNED_INT(vm);

  if (vm->paramSize > 0) {
    vm->paramAddr = READ_UNSIGNED_INT(vm);
  }

  vm->codeAddr = READ_UNSIGNED_INT(vm);

  if (vm->jumpsSize > 0) {
    vm->jumpsAddr = READ_UNSIGNED_INT(vm);
  }

  if (vm->dataSize > 0) {
    vm->dataAddr = READ_UNSIGNED_INT(vm);
  }

  if (vm->subsSize > 0) {
    vm->subsAddr = READ_UNSIGNED_INT(vm);
  }

  vm->stringBucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
  vm->scopeBucket = Xvr_allocateBucket(XVR_BUCKET_SMALL);
  vm->stack = Xvr_allocateStack();
  if (vm->scope == NULL) {
    vm->scope = Xvr_pushScope(&vm->scopeBucket, NULL);
  }
}

void Xvr_runVM(Xvr_VM *vm) {
  vm->routineCounter = vm->codeAddr;

  process(vm);
}

void Xvr_freeVM(Xvr_VM *vm) {
  Xvr_freeStack(vm->stack);
  Xvr_popScope(vm->scope);
  Xvr_freeBucket(&vm->stringBucket);
  Xvr_freeBucket(&vm->scopeBucket);

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
