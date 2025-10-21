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

#include "xvr_vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_array.h"
#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_bytecode.h"
#include "xvr_console_colors.h"
#include "xvr_opcodes.h"
#include "xvr_print.h"
#include "xvr_scope.h"
#include "xvr_stack.h"
#include "xvr_string.h"
#include "xvr_table.h"
#include "xvr_value.h"

// utilities
#define READ_BYTE(vm) vm->module[vm->programCounter++]

#define READ_UNSIGNED_INT(vm) \
    *((unsigned int*)(vm->module + readPostfixUtil(&(vm->programCounter), 4)))

#define READ_INT(vm) \
    *((int*)(vm->module + readPostfixUtil(&(vm->programCounter), 4)))

#define READ_FLOAT(vm) \
    *((float*)(vm->module + readPostfixUtil(&(vm->programCounter), 4)))

static inline int readPostfixUtil(unsigned int* ptr, int amount) {
    int ret = *ptr;
    *ptr += amount;
    return ret;
}

static inline void fixAlignment(Xvr_VM* vm) {
    vm->programCounter = (vm->programCounter + 3) & ~3;
}

// instruction handlers
static void processRead(Xvr_VM* vm) {
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

        unsigned int jump =
            *((int*)(vm->module + vm->jumpsAddr + READ_INT(vm)));

        char* cstring = (char*)(vm->module + vm->dataAddr + jump);

        if (stringType == XVR_STRING_LEAF) {
            value = XVR_VALUE_FROM_STRING(
                Xvr_createString(&vm->stringBucket, cstring));
        } else if (stringType == XVR_STRING_NAME) {
            Xvr_ValueType valueType = XVR_VALUE_UNKNOWN;

            value = XVR_VALUE_FROM_STRING(Xvr_createNameStringLength(
                &vm->stringBucket, cstring, len, valueType, false));
        } else {
            Xvr_error("Invalid string type found");
        }

        break;
    }

    case XVR_VALUE_ARRAY: {
        fixAlignment(vm);

        unsigned int count = (unsigned int)READ_INT(vm);
        unsigned int capacity = count > XVR_ARRAY_INITIAL_CAPACITY
                                    ? count
                                    : XVR_ARRAY_INITIAL_CAPACITY;
        capacity--;
        capacity |= capacity >> 1;
        capacity |= capacity >> 2;
        capacity |= capacity >> 4;
        capacity |= capacity >> 8;
        capacity |= capacity >> 16;
        capacity++;

        Xvr_Array* array = Xvr_resizeArray(NULL, capacity);
        array->capacity = capacity;
        array->count = count;

        for (int i = count - 1; i >= 0; i--) {
            array->data[i] = Xvr_popStack(&vm->stack);
        }

        value = XVR_VALUE_FROM_ARRAY(array);

        break;
    }

    case XVR_VALUE_TABLE: {
        fixAlignment(vm);

        unsigned int count = (unsigned int)READ_INT(vm);

        unsigned int capacity = count / 2;
        capacity = capacity > XVR_TABLE_INITIAL_CAPACITY
                       ? capacity
                       : XVR_TABLE_INITIAL_CAPACITY;

        capacity--;
        capacity |= capacity >> 1;
        capacity |= capacity >> 2;
        capacity |= capacity >> 4;
        capacity |= capacity >> 8;
        capacity |= capacity >> 16;
        capacity++;

        Xvr_Table* table = Xvr_private_adjustTableCapacity(NULL, capacity);

        for (unsigned int i = 0; i < count / 2; i++) {
            Xvr_Value v = Xvr_popStack(&vm->stack);
            Xvr_Value k = Xvr_popStack(&vm->stack);

            Xvr_insertTable(&table, k, v);
        }

        value = XVR_VALUE_FROM_TABLE(table);
        break;
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
        // break;
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

    fixAlignment(vm);
}

static void processDeclare(Xvr_VM* vm) {
    Xvr_ValueType type = READ_BYTE(vm);  // variable type
    unsigned int len = READ_BYTE(vm);    // name length
    bool constant = READ_BYTE(vm);

    unsigned int jump =
        *(unsigned int*)(vm->module + vm->jumpsAddr + READ_INT(vm));

    char* cstring = (char*)(vm->module + vm->dataAddr + jump);

    Xvr_String* name = Xvr_createNameStringLength(&vm->stringBucket, cstring,
                                                  len, type, constant);

    Xvr_Value value = Xvr_popStack(&vm->stack);

    Xvr_declareScope(vm->scope, name, value);

    Xvr_freeString(name);
}

static void processAssign(Xvr_VM* vm) {
    Xvr_Value value = Xvr_popStack(&vm->stack);
    Xvr_Value name = Xvr_popStack(&vm->stack);

    if (!XVR_VALUE_IS_STRING(name) ||
        XVR_VALUE_AS_STRING(name)->info.type != XVR_STRING_NAME) {
        Xvr_error("Invalid assignment target");
        Xvr_freeValue(name);
        Xvr_freeValue(value);
        return;
    }

    Xvr_assignScope(vm->scope, XVR_VALUE_AS_STRING(name), value);
    Xvr_freeValue(name);
}

static void processAssignCompound(Xvr_VM* vm) {
    Xvr_Value value = Xvr_popStack(&vm->stack);
    Xvr_Value key = Xvr_popStack(&vm->stack);
    Xvr_Value target = Xvr_popStack(&vm->stack);

    if (XVR_VALUE_IS_STRING(target) &&
        XVR_VALUE_AS_STRING(target)->info.type == XVR_STRING_NAME) {
        Xvr_Value* valuePtr =
            Xvr_accessScopeAsPointer(vm->scope, XVR_VALUE_AS_STRING(target));
        Xvr_freeValue(target);
        if (valuePtr == NULL) {
            return;
        }
        target = XVR_REFERENCE_FROM_POINTER(valuePtr);
    }

    if (XVR_VALUE_IS_ARRAY(target)) {
        if (XVR_VALUE_IS_INTEGER(key) != true) {
            Xvr_error("bad key type of assignment target");
            Xvr_freeValue(target);
            Xvr_freeValue(key);
            Xvr_freeValue(value);
            return;
        }

        Xvr_Array* array = XVR_VALUE_AS_ARRAY(target);
        int index = XVR_VALUE_AS_INTEGER(key);

        if (index < 0 || (unsigned int)index >= array->count) {
            Xvr_error("index of assignment target out of bounds");
            Xvr_freeValue(target);
            Xvr_freeValue(key);
            Xvr_freeValue(value);
        }

        array->data[index] = Xvr_copyValue(Xvr_unwrapValue(value));
        Xvr_freeValue(value);
    } else if (XVR_VALUE_IS_TABLE(target)) {
        Xvr_Table* table = XVR_VALUE_AS_TABLE(target);

        Xvr_insertTable(&table, Xvr_copyValue(Xvr_unwrapValue(key)),
                        Xvr_copyValue(Xvr_unwrapValue(value)));
        Xvr_freeValue(value);
    } else {
        Xvr_error("invalid assignment target");
        Xvr_freeValue(key);
        Xvr_freeValue(value);
        return;
    }
}

static void processAccess(Xvr_VM* vm) {
    Xvr_Value name = Xvr_popStack(&vm->stack);

    if (!XVR_VALUE_IS_STRING(name) &&
        XVR_VALUE_AS_STRING(name)->info.type != XVR_STRING_NAME) {
        Xvr_error("Invalid access target");
        return;
    }

    Xvr_Value* valuePtr =
        Xvr_accessScopeAsPointer(vm->scope, XVR_VALUE_AS_STRING(name));

    if (valuePtr == NULL) {
        Xvr_freeValue(name);
        return;
    }

    if (XVR_VALUE_IS_REFERENCE(*valuePtr) || XVR_VALUE_IS_ARRAY(*valuePtr) ||
        XVR_VALUE_IS_TABLE(*valuePtr)) {
        Xvr_Value ref = XVR_REFERENCE_FROM_POINTER(valuePtr);

        Xvr_pushStack(&vm->stack, ref);
    }

    else {
        Xvr_pushStack(&vm->stack, Xvr_copyValue(*valuePtr));
    }

    Xvr_freeValue(name);
}

static void processDuplicate(Xvr_VM* vm) {
    Xvr_Value value = Xvr_copyValue(Xvr_peekStack(&vm->stack));
    Xvr_pushStack(&vm->stack, value);

    Xvr_OpcodeType squeezed = READ_BYTE(vm);
    if (squeezed == XVR_OPCODE_ACCESS) {
        processAccess(vm);
    }
}

static void processArithmetic(Xvr_VM* vm, Xvr_OpcodeType opcode) {
    Xvr_Value right = Xvr_popStack(&vm->stack);
    Xvr_Value left = Xvr_popStack(&vm->stack);

    // check types
    if ((!XVR_VALUE_IS_INTEGER(left) && !XVR_VALUE_IS_FLOAT(left)) ||
        (!XVR_VALUE_IS_INTEGER(right) && !XVR_VALUE_IS_FLOAT(right))) {
        char buffer[256];
        snprintf(buffer, 256,
                 "Invalid types '%s' and '%s' passed in arithmetic",
                 Xvr_private_getValueTypeAsCString(left.type),
                 Xvr_private_getValueTypeAsCString(right.type));
        Xvr_error(buffer);

        Xvr_freeValue(left);
        Xvr_freeValue(right);

        return;
    }

    if (opcode == XVR_OPCODE_DIVIDE || opcode == XVR_OPCODE_MODULO) {
        if ((XVR_VALUE_IS_INTEGER(right) && XVR_VALUE_AS_INTEGER(right) == 0) ||
            (XVR_VALUE_IS_FLOAT(right) && XVR_VALUE_AS_FLOAT(right) == 0)) {
            Xvr_error("Can't divide or modulo by zero");
            Xvr_freeValue(left);
            Xvr_freeValue(right);
            return;
        }
    }

    if (opcode == XVR_OPCODE_MODULO && XVR_VALUE_IS_FLOAT(right)) {
        Xvr_error("Can't modulo by a float");
        Xvr_freeValue(left);
        Xvr_freeValue(right);
        return;
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
                XVR_CC_ERROR
                "ERROR: Invalid opcode %d passed to processArithmetic, "
                "exiting\n" XVR_CC_RESET,
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

static void processComparison(Xvr_VM* vm, Xvr_OpcodeType opcode) {
    Xvr_Value right = Xvr_popStack(&vm->stack);
    Xvr_Value left = Xvr_popStack(&vm->stack);

    if (opcode == XVR_OPCODE_COMPARE_EQUAL) {
        bool equal = Xvr_checkValuesAreEqual(left, right);

        if (READ_BYTE(vm) != XVR_OPCODE_NEGATE) {
            Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(equal));
        } else {
            Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(!equal));
        }

        Xvr_freeValue(left);
        Xvr_freeValue(right);
        return;
    }

    if (Xvr_checkValuesAreComparable(left, right) != true) {
        char buffer[256];
        snprintf(buffer, 256, "Can't compare value types '%s' and '%s'",
                 Xvr_private_getValueTypeAsCString(left.type),
                 Xvr_private_getValueTypeAsCString(right.type));
        Xvr_error(buffer);

        Xvr_freeValue(left);
        Xvr_freeValue(right);

        return;
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
    }

    else {
        Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_BOOLEAN(false));
    }

    Xvr_freeValue(left);
    Xvr_freeValue(right);
}

static void processLogical(Xvr_VM* vm, Xvr_OpcodeType opcode) {
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
                      XVR_VALUE_FROM_BOOLEAN(Xvr_checkValueIsTruthy(top)));
    } else if (opcode == XVR_OPCODE_NEGATE) {
        Xvr_Value top = Xvr_popStack(&vm->stack);

        Xvr_pushStack(&vm->stack,
                      XVR_VALUE_FROM_BOOLEAN(!Xvr_checkValueIsTruthy(top)));
    } else {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Invalid opcode %d passed to processLogical, "
                "exiting\n" XVR_CC_RESET,
                opcode);
        exit(-1);
    }
}

static void processJump(Xvr_VM* vm) {
    Xvr_OpJumpType type = READ_BYTE(vm);
    Xvr_OpParamJumpConditional cond = READ_BYTE(vm);
    fixAlignment(vm);

    int param = READ_INT(vm);

    switch (cond) {
    case XVR_OP_PARAM_JUMP_ALWAYS:
        break;

    case XVR_OP_PARAM_JUMP_IF_TRUE: {
        Xvr_Value value = Xvr_popStack(&vm->stack);
        if (Xvr_checkValueIsTruthy(value) == true) {
            Xvr_freeValue(value);
            break;
        }

        Xvr_freeValue(value);
        return;
    }

    case XVR_OP_PARAM_JUMP_IF_FALSE: {
        Xvr_Value value = Xvr_popStack(&vm->stack);
        if (Xvr_checkValueIsTruthy(value) != true) {
            Xvr_freeValue(value);
            break;
        }
        Xvr_freeValue(value);
        return;
    }
    }

    switch (type) {
    case XVR_OP_PARAM_JUMP_ABSOLUTE:
        vm->programCounter = vm->codeAddr + param;
        return;

    case XVR_OP_PARAM_JUMP_RELATIVE:
        vm->programCounter += param;
        return;
    }
}

static void processAssert(Xvr_VM* vm) {
    unsigned int count = READ_BYTE(vm);

    Xvr_Value value = XVR_VALUE_FROM_NULL();
    Xvr_Value message = XVR_VALUE_FROM_NULL();

    if (count == 1) {
        message = XVR_VALUE_FROM_STRING(
            Xvr_createString(&vm->stringBucket, "assertion failed"));
        value = Xvr_popStack(&vm->stack);
    } else if (count == 2) {
        message = Xvr_popStack(&vm->stack);
        value = Xvr_popStack(&vm->stack);
    } else {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Invalid assert argument count %d found, "
                "exiting\n" XVR_CC_RESET,
                (int)count);
        exit(-1);
    }

    // do the check
    if (XVR_VALUE_IS_NULL(value) || Xvr_checkValueIsTruthy(value) != true) {
        Xvr_String* string = Xvr_stringifyValue(&vm->stringBucket, message);
        char* buffer = Xvr_getStringRawBuffer(string);

        Xvr_assertFailure(buffer);

        free(buffer);
        Xvr_freeString(string);
        return;
    }

    Xvr_freeValue(value);
    Xvr_freeValue(message);
}

static void processPrint(Xvr_VM* vm) {
    Xvr_Value value = Xvr_popStack(&vm->stack);
    Xvr_String* string = Xvr_stringifyValue(&vm->stringBucket, value);
    char* buffer = Xvr_getStringRawBuffer(string);

    Xvr_print(buffer);

    free(buffer);
    Xvr_freeString(string);

    Xvr_freeValue(value);
}

static void processConcat(Xvr_VM* vm) {
    Xvr_Value right = Xvr_popStack(&vm->stack);
    Xvr_Value left = Xvr_popStack(&vm->stack);

    if (!XVR_VALUE_IS_STRING(left) || !XVR_VALUE_IS_STRING(right)) {
        Xvr_error("Failed to concatenate a value that is not a string");

        Xvr_freeValue(left);
        Xvr_freeValue(right);
        return;
    }

    Xvr_String* result =
        Xvr_concatStrings(&vm->stringBucket, XVR_VALUE_AS_STRING(left),
                          XVR_VALUE_AS_STRING(right));
    Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_STRING(result));
}

static void processIndex(Xvr_VM* vm) {
    unsigned char count = READ_BYTE(vm);
    Xvr_Value value = XVR_VALUE_FROM_NULL();
    Xvr_Value index = XVR_VALUE_FROM_NULL();
    Xvr_Value length = XVR_VALUE_FROM_NULL();

    if (count == 3) {
        length = Xvr_popStack(&vm->stack);
        index = Xvr_popStack(&vm->stack);
        value = Xvr_popStack(&vm->stack);
    } else if (count == 2) {
        index = Xvr_popStack(&vm->stack);
        value = Xvr_popStack(&vm->stack);
    } else {
        Xvr_error("Incorrect number of elements found in index");
        return;
    }

    if (XVR_VALUE_IS_STRING(value)) {
        if (!XVR_VALUE_IS_INTEGER(index)) {
            Xvr_error("Failed to index a string");
            Xvr_freeValue(value);
            Xvr_freeValue(index);
            Xvr_freeValue(length);
            return;
        }

        if (!(XVR_VALUE_IS_NULL(length) || XVR_VALUE_IS_INTEGER(length))) {
            Xvr_error("Failed to index-length a string");
            Xvr_freeValue(value);
            Xvr_freeValue(index);
            Xvr_freeValue(length);
            return;
        }

        int i = XVR_VALUE_AS_INTEGER(index);
        int l = XVR_VALUE_IS_INTEGER(length) ? XVR_VALUE_AS_INTEGER(length) : 1;
        Xvr_String* str = XVR_VALUE_AS_STRING(value);

        if ((i < 0 || (unsigned int)i >= str->info.length) ||
            (i + l <= 0 || (unsigned int)(i + l) > str->info.length)) {
            Xvr_error("String index is out of bounds");
            Xvr_freeValue(value);
            Xvr_freeValue(index);
            Xvr_freeValue(length);
            return;
        }

        Xvr_String* result = NULL;

        if (str->info.type == XVR_STRING_LEAF) {
            const char* cstr = str->leaf.data;
            result = Xvr_createStringLength(&vm->stringBucket, cstr + i, l);
        } else if (str->info.type == XVR_STRING_NODE) {
            char* cstr = Xvr_getStringRawBuffer(str);
            result = Xvr_createStringLength(&vm->stringBucket, cstr + i, l);
            free(cstr);
        } else {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: Unknown string type found in processIndex, "
                    "exiting\n" XVR_CC_RESET);
            exit(-1);
        }

        Xvr_pushStack(&vm->stack, XVR_VALUE_FROM_STRING(result));
    }

    else if (XVR_VALUE_IS_ARRAY(value)) {
        if (!XVR_VALUE_IS_INTEGER(index)) {
            Xvr_error("Failed to index a string");
            Xvr_freeValue(value);
            Xvr_freeValue(index);
            Xvr_freeValue(length);
            return;
        }

        if (!(XVR_VALUE_IS_NULL(length) || XVR_VALUE_IS_INTEGER(length))) {
            Xvr_error("Failed to index-length a string");
            Xvr_freeValue(value);
            Xvr_freeValue(index);
            Xvr_freeValue(length);
            return;
        }

        int i = XVR_VALUE_AS_INTEGER(index);
        int l = XVR_VALUE_IS_INTEGER(length) ? XVR_VALUE_AS_INTEGER(length) : 1;
        Xvr_Array* array = XVR_VALUE_AS_ARRAY(value);

        if ((i < 0 || (unsigned int)i >= array->count) ||
            (i + l <= 0 || (unsigned int)(i + l) > array->count)) {
            Xvr_error("Array index is out of bounds");
            Xvr_freeValue(value);
            Xvr_freeValue(index);
            Xvr_freeValue(length);
            return;
        }

        if (XVR_VALUE_IS_REFERENCE(array->data[i]) ||
            XVR_VALUE_IS_ARRAY(array->data[i]) ||
            XVR_VALUE_IS_TABLE(array->data[i])) {
            Xvr_Value ref = XVR_REFERENCE_FROM_POINTER(&(array->data[i]));

            Xvr_pushStack(&vm->stack, ref);
        }

        else {
            Xvr_pushStack(&vm->stack, Xvr_copyValue(array->data[i]));
        }
    } else if (XVR_VALUE_IS_TABLE(value)) {
        if (XVR_VALUE_IS_NULL(length) != true) {
            Xvr_error("can't index-length a table");
            Xvr_freeValue(value);
            Xvr_freeValue(index);
            Xvr_freeValue(length);

            return;
        }

        Xvr_Table* table = XVR_VALUE_AS_TABLE(value);
        Xvr_TableEntry* entry = Xvr_private_lookupTableEntryPtr(&table, index);

        if (entry == NULL) {
            Xvr_error("table key not found");
            Xvr_freeValue(value);
            Xvr_freeValue(index);
            Xvr_freeValue(length);
            return;
        }

        if (XVR_VALUE_IS_REFERENCE(entry->value) ||
            XVR_VALUE_IS_ARRAY(entry->value) ||
            XVR_VALUE_IS_TABLE(entry->value)) {
            Xvr_Value ref = XVR_REFERENCE_FROM_POINTER(&(entry->value));
            Xvr_pushStack(&vm->stack, ref);
        }

        else {
            Xvr_pushStack(&vm->stack, Xvr_copyValue(entry->value));
        }
    }

    else {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Unknown value type '%s' found in processIndex, "
                "exiting\n" XVR_CC_RESET,
                Xvr_private_getValueTypeAsCString(value.type));
        exit(-1);
    }

    Xvr_freeValue(value);
    Xvr_freeValue(index);
    Xvr_freeValue(length);
}

static void process(Xvr_VM* vm) {
    while (true) {
        fixAlignment(vm);

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

        case XVR_OPCODE_ASSIGN_COMPOUND:
            processAssignCompound(vm);
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

        case XVR_OPCODE_JUMP:
            processJump(vm);
            break;

        case XVR_OPCODE_SCOPE_PUSH:
            vm->scope = Xvr_pushScope(&vm->scopeBucket, vm->scope);
            break;

        case XVR_OPCODE_SCOPE_POP:
            vm->scope = Xvr_popScope(vm->scope);
            break;

        case XVR_OPCODE_ASSERT:
            processAssert(vm);
            break;

        case XVR_OPCODE_PRINT:
            processPrint(vm);
            break;

        case XVR_OPCODE_CONCAT:
            processConcat(vm);
            break;

        case XVR_OPCODE_INDEX:
            processIndex(vm);
            break;

        case XVR_OPCODE_UNUSED:
        case XVR_OPCODE_PASS:
        case XVR_OPCODE_ERROR:
        case XVR_OPCODE_EOF:
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Invalid opcode %d found, exiting\n" XVR_CC_RESET,
                    opcode);
            exit(-1);
        }
    }
}

void Xvr_initVM(Xvr_VM* vm) {
    vm->stringBucket = NULL;
    vm->scopeBucket = NULL;
    vm->stack = NULL;
    vm->scope = NULL;

    Xvr_resetVM(vm);
}

void Xvr_bindVM(Xvr_VM* vm, struct Xvr_Bytecode* bc) {
    if (bc->ptr[0] != XVR_VERSION_MAJOR || bc->ptr[1] > XVR_VERSION_MINOR) {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Wrong bytecode version found: expected %d.%d.%d found "
                "%d.%d.%d, exiting\n" XVR_CC_RESET,
                XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
                bc->ptr[0], bc->ptr[1], bc->ptr[2]);
        exit(-1);
    }

    if (bc->ptr[2] != XVR_VERSION_PATCH) {
        fprintf(stderr,
                XVR_CC_WARN
                "WARNING: Wrong bytecode version found: expected %d.%d.%d "
                "found %d.%d.%d, continuing\n" XVR_CC_RESET,
                XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
                bc->ptr[0], bc->ptr[1], bc->ptr[2]);
    }

    if (strcmp((char*)(bc->ptr + 3), XVR_VERSION_BUILD) != 0) {
        fprintf(stderr,
                XVR_CC_WARN
                "WARNING: Wrong bytecode build info found: expected '%s' found "
                "'%s', continuing\n" XVR_CC_RESET,
                XVR_VERSION_BUILD, (char*)(bc->ptr + 3));
    }

    int offset = 3 + strlen(XVR_VERSION_BUILD) + 1;
    if (offset % 4 != 0) {
        offset += 4 - (offset % 4);
    }

    if (bc->moduleCount != 0) {
        Xvr_bindVMToModule(vm, bc->ptr + offset);
    }
}

void Xvr_bindVMToModule(Xvr_VM* vm, unsigned char* module) {
    vm->module = module;

    vm->moduleSize = READ_UNSIGNED_INT(vm);
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

    if (vm->stringBucket == NULL) {
        vm->stringBucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    }
    if (vm->scopeBucket == NULL) {
        vm->scopeBucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    }
    if (vm->stack == NULL) {
        vm->stack = Xvr_allocateStack();
    }
    if (vm->scope == NULL) {
        vm->scope = Xvr_pushScope(&vm->scopeBucket, NULL);
    }
}

void Xvr_runVM(Xvr_VM* vm) {
    if (vm->module == NULL) {
        return;
    }
    vm->programCounter = vm->codeAddr;

    process(vm);
}

void Xvr_freeVM(Xvr_VM* vm) {
    Xvr_freeStack(vm->stack);
    Xvr_popScope(vm->scope);
    Xvr_freeBucket(&vm->stringBucket);
    Xvr_freeBucket(&vm->scopeBucket);

    Xvr_resetVM(vm);
}

void Xvr_resetVM(Xvr_VM* vm) {
    vm->module = NULL;
    vm->moduleSize = 0;

    vm->paramSize = 0;
    vm->jumpsSize = 0;
    vm->dataSize = 0;
    vm->subsSize = 0;

    vm->paramAddr = 0;
    vm->codeAddr = 0;
    vm->jumpsAddr = 0;
    vm->dataAddr = 0;
    vm->subsAddr = 0;

    vm->programCounter = 0;
}
