#include "lib_standard.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "xvr_memory.h"

static int nativeClock(Xvr_Interpreter* interpreter,
                       Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 0) {
        interpreter->errorOutput("Incorrect number of arguments to clock\n");
        return -1;
    }

    // get the time from C (what a pain)
    time_t rawtime = time(NULL);
    struct tm* timeinfo = localtime(&rawtime);
    char* timestr = asctime(timeinfo);

    // push to the stack
    int len = strlen(timestr) - 1;
    Xvr_Literal timeLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefStringLength(timestr, len));

    Xvr_pushLiteralArray(&interpreter->stack, timeLiteral);

    // cleanup
    Xvr_freeLiteral(timeLiteral);

    return 1;
}

static int nativeConcat(Xvr_Interpreter* interpreter,
                        Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("Incorrect number of arguments to _concat\n");
        return -1;
    }

    Xvr_Literal otherLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal otherLiteralIdn = otherLiteral;
    if (XVR_IS_IDENTIFIER(otherLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &otherLiteral)) {
        Xvr_freeLiteral(otherLiteralIdn);
    }

    if (XVR_IS_ARRAY(selfLiteral)) {
        if (!XVR_IS_ARRAY(otherLiteral)) {
            interpreter->errorOutput(
                "Incorrect argument type passed to _concat (unknown type for "
                "other)\n");
            Xvr_freeLiteral(selfLiteral);
            Xvr_freeLiteral(otherLiteral);
            return -1;
        }

        for (int i = 0; i < XVR_AS_ARRAY(otherLiteral)->count; i++) {
            Xvr_pushLiteralArray(XVR_AS_ARRAY(selfLiteral),
                                 XVR_AS_ARRAY(otherLiteral)->literals[i]);
        }

        Xvr_pushLiteralArray(&interpreter->stack, selfLiteral);

        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(otherLiteral);

        return 1;
    }

    if (XVR_IS_DICTIONARY(selfLiteral)) {
        if (!XVR_IS_DICTIONARY(otherLiteral)) {
            interpreter->errorOutput(
                "Incorrect argument type passed to _concat (unknown type for "
                "other)\n");
            Xvr_freeLiteral(selfLiteral);
            Xvr_freeLiteral(otherLiteral);
            return -1;
        }

        for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
            if (!XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key)) {
                Xvr_setLiteralDictionary(
                    XVR_AS_DICTIONARY(otherLiteral),
                    XVR_AS_DICTIONARY(selfLiteral)->entries[i].key,
                    XVR_AS_DICTIONARY(selfLiteral)->entries[i].value);
            }
        }

        Xvr_pushLiteralArray(&interpreter->stack, otherLiteral);

        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(otherLiteral);

        return 1;
    }

    if (XVR_IS_STRING(selfLiteral)) {
        if (!XVR_IS_STRING(otherLiteral)) {
            interpreter->errorOutput(
                "Incorrect argument type passed to _concat (unknown type for "
                "other)\n");
            Xvr_freeLiteral(selfLiteral);
            Xvr_freeLiteral(otherLiteral);
            return -1;
        }

        int length = XVR_AS_STRING(selfLiteral)->length +
                     XVR_AS_STRING(otherLiteral)->length + 1;

        if (length > XVR_MAX_STRING_LENGTH) {
            interpreter->errorOutput(
                "Can't concatenate these strings, result is too long (error "
                "found in _concat)\n");
            Xvr_freeLiteral(selfLiteral);
            Xvr_freeLiteral(otherLiteral);
            return -1;
        }

        char* buffer = XVR_ALLOCATE(char, length);
        snprintf(buffer, length, "%s%s",
                 Xvr_toCString(XVR_AS_STRING(selfLiteral)),
                 Xvr_toCString(XVR_AS_STRING(otherLiteral)));

        Xvr_Literal result = XVR_TO_STRING_LITERAL(Xvr_createRefString(buffer));

        Xvr_pushLiteralArray(&interpreter->stack, result);

        XVR_FREE_ARRAY(char, buffer, length);
        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(otherLiteral);
        Xvr_freeLiteral(result);

        return 1;
    }

    interpreter->errorOutput(
        "Incorrect argument type passed to _concat (unknown type for self)\n");
    Xvr_freeLiteral(selfLiteral);
    Xvr_freeLiteral(otherLiteral);
    return -1;
}

static int nativeContainsKey(Xvr_Interpreter* interpreter,
                             Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput(
            "incorrect number of arguments to _containsKey\n");
        return -1;
    }

    Xvr_Literal keyLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;

    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal keyLiteralIdn = keyLiteral;
    if (XVR_IS_IDENTIFIER(keyLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &keyLiteral)) {
        Xvr_freeLiteral(keyLiteralIdn);
    }

    if (!(XVR_IS_DICTIONARY(selfLiteral))) {
        interpreter->errorOutput(
            "incorrect argument type passed to _containsKey\n");
        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(keyLiteral);
        return -1;
    }

    Xvr_Literal resultLiteral = XVR_TO_BOOLEAN_LITERAL(false);
    if (XVR_IS_DICTIONARY(selfLiteral) &&
        Xvr_existsLiteralDictionary(XVR_AS_DICTIONARY(selfLiteral),
                                    keyLiteral)) {
        Xvr_freeLiteral(resultLiteral);
        resultLiteral = XVR_TO_BOOLEAN_LITERAL(true);
    }

    Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
    Xvr_freeLiteral(resultLiteral);

    Xvr_freeLiteral(selfLiteral);
    Xvr_freeLiteral(keyLiteral);

    return 1;
}

static int nativeContainsValue(Xvr_Interpreter* interpreter,
                               Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput(
            "incorrect number of arguments to _containsValue\n");
        return -1;
    }

    Xvr_Literal valueLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;

    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal valueLiteralIdn = valueLiteral;
    if (XVR_IS_IDENTIFIER(valueLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &valueLiteral)) {
        Xvr_freeLiteral(valueLiteralIdn);
    }

    if (!(XVR_IS_ARRAY(selfLiteral) || XVR_IS_DICTIONARY(selfLiteral))) {
        interpreter->errorOutput(
            "incorrect argument type passing to _containsValue\n");
        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(valueLiteral);
        return -1;
    }

    Xvr_Literal resultLiteral = XVR_TO_BOOLEAN_LITERAL(false);

    if (XVR_IS_DICTIONARY(selfLiteral)) {
        for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
            if (!XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key) &&
                Xvr_literalsAreEqual(
                    XVR_AS_DICTIONARY(selfLiteral)->entries[i].value,
                    valueLiteral)) {
                Xvr_freeLiteral(resultLiteral);
                resultLiteral = XVR_TO_BOOLEAN_LITERAL(true);
                break;
            }
        }
    } else if (XVR_IS_ARRAY(selfLiteral)) {
        for (int i = 0; i < XVR_AS_ARRAY(selfLiteral)->count; i++) {
            Xvr_Literal indexLiteral = XVR_TO_INTEGER_LITERAL(i);
            Xvr_Literal elementLiteral =
                Xvr_getLiteralArray(XVR_AS_ARRAY(selfLiteral), indexLiteral);

            if (Xvr_literalsAreEqual(elementLiteral, valueLiteral)) {
                Xvr_freeLiteral(indexLiteral);
                Xvr_freeLiteral(elementLiteral);

                Xvr_freeLiteral(resultLiteral);
                resultLiteral = XVR_TO_BOOLEAN_LITERAL(true);
                break;
            }
            Xvr_freeLiteral(indexLiteral);
            Xvr_freeLiteral(elementLiteral);
        }
    }

    Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
    Xvr_freeLiteral(resultLiteral);

    Xvr_freeLiteral(selfLiteral);
    Xvr_freeLiteral(valueLiteral);

    return 1;
}

static int nativeEvery(Xvr_Interpreter* interpreter,
                       Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("incorret number of arguments to _every\n");
        return -1;
    }

    Xvr_Literal procLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;

    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal procLiteralIdn = procLiteral;

    if (XVR_IS_IDENTIFIER(procLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &procLiteral)) {
        Xvr_freeLiteral(procLiteralIdn);
    }

    if (!(XVR_IS_ARRAY(selfLiteral) || XVR_IS_DICTIONARY(selfLiteral)) ||
        !(XVR_IS_FUNCTION(procLiteral) ||
          XVR_IS_FUNCTION_NATIVE(procLiteral))) {
        interpreter->errorOutput("incorrect argument type passed to _every\n");
        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(procLiteral);

        return -1;
    }

    if (XVR_IS_ARRAY(selfLiteral)) {
        bool result = true;

        for (int i = 0; i < XVR_AS_ARRAY(selfLiteral)->count; i++) {
            Xvr_Literal indexLiteral = XVR_TO_INTEGER_LITERAL(i);

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(&arguments,
                                 XVR_AS_ARRAY(selfLiteral)->literals[i]);
            Xvr_pushLiteralArray(&arguments, indexLiteral);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_Literal lit = Xvr_popLiteralArray(&returns);

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
            Xvr_freeLiteral(indexLiteral);

            if (!XVR_IS_TRUTHY(lit)) {
                Xvr_freeLiteral(lit);
                result = false;
                break;
            }

            Xvr_freeLiteral(lit);
        }

        Xvr_Literal resultLiteral = XVR_TO_BOOLEAN_LITERAL(result);
        Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
        Xvr_freeLiteral(resultLiteral);
    }

    if (XVR_IS_DICTIONARY(selfLiteral)) {
        bool result = true;

        for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
            if (XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key)) {
                continue;
            }

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].value);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].key);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_Literal lit = Xvr_popLiteralArray(&returns);

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);

            if (!XVR_IS_TRUTHY(lit)) {
                Xvr_freeLiteral(lit);
                result = false;
                break;
            }

            Xvr_freeLiteral(lit);
        }

        Xvr_Literal resultLiteral = XVR_TO_BOOLEAN_LITERAL(result);
        Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
        Xvr_freeLiteral(resultLiteral);
    }

    Xvr_freeLiteral(procLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static int nativeFilter(Xvr_Interpreter* interpreter,
                        Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("incorrect number of arguments to _filter\n");
        return -1;
    }

    Xvr_Literal procLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(procLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    if (!(XVR_IS_ARRAY(selfLiteral) || XVR_IS_DICTIONARY(selfLiteral)) ||
        !(XVR_IS_FUNCTION(procLiteral))) {
        interpreter->errorOutput(
            "incorrect argument type passing to _filter\n");
        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(procLiteral);
        return -1;
    }

    if (XVR_IS_ARRAY(selfLiteral)) {
        Xvr_LiteralArray* result = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(result);

        for (int i = 0; i < XVR_AS_ARRAY(selfLiteral)->count; i++) {
            Xvr_Literal indexLiteral = XVR_TO_INTEGER_LITERAL(i);

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(&arguments,
                                 XVR_AS_ARRAY(selfLiteral)->literals[i]);
            Xvr_pushLiteralArray(&arguments, indexLiteral);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_Literal lit = Xvr_popLiteralArray(&returns);

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
            Xvr_freeLiteral(indexLiteral);

            if (XVR_IS_TRUTHY(lit)) {
                Xvr_pushLiteralArray(result,
                                     XVR_AS_ARRAY(selfLiteral)->literals[i]);
            }

            Xvr_freeLiteral(lit);
        }

        Xvr_Literal resultLiteral = XVR_TO_ARRAY_LITERAL(result);
        Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
        Xvr_freeLiteral(resultLiteral);
    }

    if (XVR_IS_DICTIONARY(selfLiteral)) {
        Xvr_LiteralDictionary* result = XVR_ALLOCATE(Xvr_LiteralDictionary, 1);
        Xvr_initLiteralDictionary(result);

        for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
            if (XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key)) {
                continue;
            }

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].value);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].key);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_Literal lit = Xvr_popLiteralArray(&returns);

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);

            if (XVR_IS_TRUTHY(lit)) {
                Xvr_setLiteralDictionary(
                    result, XVR_AS_DICTIONARY(selfLiteral)->entries[i].key,
                    XVR_AS_DICTIONARY(selfLiteral)->entries[i].value);
            }

            Xvr_freeLiteral(lit);
        }

        Xvr_Literal resultLiteral = XVR_TO_DICTIONARY_LITERAL(result);
        Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
        Xvr_freeLiteral(resultLiteral);
    }

    return 1;
}

static int nativeForEach(Xvr_Interpreter* interpreter,
                         Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("Incorrect number of arguments to _forEach\n");
        return -1;
    }

    Xvr_Literal procLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal procLiteralIdn = procLiteral;
    if (XVR_IS_IDENTIFIER(procLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &procLiteralIdn)) {
        Xvr_freeLiteral(procLiteralIdn);
    }

    if (!(XVR_IS_ARRAY(selfLiteral) || XVR_IS_DICTIONARY(selfLiteral) ||
          XVR_IS_FUNCTION(procLiteral) ||
          XVR_IS_FUNCTION_NATIVE(procLiteral))) {
        interpreter->errorOutput(
            "Incorrect argument type passing to _forEach\n");
        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(procLiteral);
        return -1;
    }

    if (XVR_IS_ARRAY(selfLiteral)) {
        for (int i = 0; i < XVR_AS_ARRAY(selfLiteral)->count; i++) {
            Xvr_Literal indexLiteral = XVR_TO_INTEGER_LITERAL(i);
            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(&arguments,
                                 XVR_AS_ARRAY(selfLiteral)->literals[i]);
            Xvr_pushLiteralArray(&arguments, indexLiteral);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);
            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);
            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
            Xvr_freeLiteral(indexLiteral);
        }
    }

    if (XVR_IS_DICTIONARY(selfLiteral)) {
        for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
            if (XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key)) {
                continue;
            }

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].value);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].key);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);
            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
        }
    }

    Xvr_freeLiteral(procLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 0;
}

static int nativeGetKeys(Xvr_Interpreter* interpreter,
                         Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput("Incorrect number of arguments to _getKeys\n");
        return -1;
    }

    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    if (!XVR_IS_DICTIONARY(selfLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _getKeys\n");
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    Xvr_LiteralArray* resultPtr = XVR_ALLOCATE(Xvr_LiteralArray, 1);
    Xvr_initLiteralArray(resultPtr);

    for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
        if (!XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key)) {
            Xvr_pushLiteralArray(
                resultPtr, XVR_AS_DICTIONARY(selfLiteral)->entries[i].key);
        }
    }

    Xvr_Literal result = XVR_TO_ARRAY_LITERAL(resultPtr);
    Xvr_pushLiteralArray(&interpreter->stack, result);

    Xvr_freeLiteralArray(resultPtr);
    XVR_FREE(Xvr_LiteralArray, resultPtr);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static int nativeGetValues(Xvr_Interpreter* interpreter,
                           Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _getValues\n");
        return -1;
    }

    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    if (!XVR_IS_DICTIONARY(selfLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _getValues\n");
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    Xvr_LiteralArray* resultPtr = XVR_ALLOCATE(Xvr_LiteralArray, 1);
    Xvr_initLiteralArray(resultPtr);

    for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
        if (!XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key)) {
            Xvr_pushLiteralArray(
                resultPtr, XVR_AS_DICTIONARY(selfLiteral)->entries[i].value);
        }
    }

    Xvr_Literal result = XVR_TO_ARRAY_LITERAL(resultPtr);
    Xvr_pushLiteralArray(&interpreter->stack, result);

    Xvr_freeLiteralArray(resultPtr);
    XVR_FREE(Xvr_LiteralArray, resultPtr);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static int nativeIndexOf(Xvr_Interpreter* interpreter,
                         Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("incorrect number of arguments to _indexOf");
        return -1;
    }

    Xvr_Literal valueLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal valueLiteralIdn = valueLiteral;
    if (XVR_IS_IDENTIFIER(valueLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(valueLiteralIdn);
    }

    if (!XVR_IS_ARRAY(selfLiteral)) {
        interpreter->errorOutput("incorrect argument type passed to indexOf\n");
        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(valueLiteral);
        return -1;
    }

    Xvr_Literal resultLiteral = XVR_TO_NULL_LITERAL;
    for (int i = 0; i < XVR_AS_ARRAY(selfLiteral)->count; i++) {
        if (Xvr_literalsAreEqual(valueLiteral,
                                 XVR_AS_ARRAY(selfLiteral)->literals[i])) {
            resultLiteral = XVR_TO_INTEGER_LITERAL(i);
            break;
        }
    }

    Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
    Xvr_freeLiteral(resultLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static int nativeMap(Xvr_Interpreter* interpreter,
                     Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("Incorrect number of arguments to _map\n");
        return -1;
    }

    Xvr_Literal procLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal procLiteralIdn = procLiteral;
    if (XVR_IS_IDENTIFIER(procLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &procLiteral)) {
        Xvr_freeLiteral(procLiteralIdn);
    }

    if (!(XVR_IS_ARRAY(selfLiteral) || XVR_IS_DICTIONARY(selfLiteral)) ||
        !(XVR_IS_FUNCTION(procLiteral) ||
          XVR_IS_FUNCTION_NATIVE(procLiteral))) {
        interpreter->errorOutput("Incorrect argument type passed to _map\n");
        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(procLiteral);
        return -1;
    }

    if (XVR_IS_ARRAY(selfLiteral)) {
        Xvr_LiteralArray* returnsPtr = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(returnsPtr);
    }

    if (XVR_IS_ARRAY(selfLiteral)) {
        Xvr_LiteralArray* returnsPtr = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(returnsPtr);

        for (int i = 0; i < XVR_AS_ARRAY(selfLiteral)->count; i++) {
            Xvr_Literal indexLiteral = XVR_TO_INTEGER_LITERAL(i);

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(&arguments,
                                 XVR_AS_ARRAY(selfLiteral)->literals[i]);
            Xvr_pushLiteralArray(&arguments, indexLiteral);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_Literal lit = Xvr_popLiteralArray(&returns);
            Xvr_pushLiteralArray(returnsPtr, lit);
            Xvr_freeLiteral(lit);

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
            Xvr_freeLiteral(indexLiteral);
        }

        Xvr_Literal returnsLiteral = XVR_TO_ARRAY_LITERAL(returnsPtr);
        Xvr_pushLiteralArray(&interpreter->stack, returnsLiteral);
        Xvr_freeLiteral(returnsLiteral);
    }

    if (XVR_IS_DICTIONARY(selfLiteral)) {
        Xvr_LiteralArray* returnsPtr = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(returnsPtr);

        for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
            if (XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key)) {
                continue;
            }

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].value);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].key);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_Literal lit = Xvr_popLiteralArray(&returns);
            Xvr_pushLiteralArray(returnsPtr, lit);
            Xvr_freeLiteral(lit);
            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
        }

        Xvr_Literal returnsLiteral = XVR_TO_ARRAY_LITERAL(returnsPtr);
        Xvr_pushLiteralArray(&interpreter->stack, returnsLiteral);
        Xvr_freeLiteral(returnsLiteral);
    }

    Xvr_freeLiteral(procLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 0;
}

static int nativeReduce(Xvr_Interpreter* interpreter,
                        Xvr_LiteralArray* arguments) {
    if (arguments->count != 3) {
        interpreter->errorOutput("Incorrect number of arguments to _reduce\n");
        return -1;
    }

    Xvr_Literal procLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal defaultLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal procLiteralIdn = procLiteral;
    if (XVR_IS_IDENTIFIER(procLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &procLiteral)) {
        Xvr_freeLiteral(procLiteralIdn);
    }

    if (!(XVR_IS_ARRAY(selfLiteral) || XVR_IS_DICTIONARY(selfLiteral)) ||
        !(XVR_IS_FUNCTION(procLiteral) ||
          XVR_IS_FUNCTION_NATIVE(procLiteral))) {
        interpreter->errorOutput("Incorrect argument type passed to _reduce\n");
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    if (XVR_IS_ARRAY(selfLiteral)) {
        for (int i = 0; i < XVR_AS_ARRAY(selfLiteral)->count; i++) {
            Xvr_Literal indexLiteral = XVR_TO_INTEGER_LITERAL(i);

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(&arguments,
                                 XVR_AS_ARRAY(selfLiteral)->literals[i]);
            Xvr_pushLiteralArray(&arguments, indexLiteral);
            Xvr_pushLiteralArray(&arguments, defaultLiteral);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);
            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_freeLiteral(defaultLiteral);
            defaultLiteral = Xvr_popLiteralArray(&returns);

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
            Xvr_freeLiteral(indexLiteral);
        }
        Xvr_pushLiteralArray(&interpreter->stack, defaultLiteral);
    }

    if (XVR_IS_DICTIONARY(selfLiteral)) {
        for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
            if (XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key)) {
                continue;
            }
            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].value);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].key);
            Xvr_pushLiteralArray(&arguments, defaultLiteral);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_freeLiteral(defaultLiteral);
            defaultLiteral = Xvr_popLiteralArray(&returns);

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
        }

        Xvr_pushLiteralArray(&interpreter->stack, defaultLiteral);
    }

    Xvr_freeLiteral(procLiteral);
    Xvr_freeLiteral(defaultLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 0;
}

static int nativeSome(Xvr_Interpreter* interpreter,
                      Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("incorrect number of arguments to _some\n");
        return -1;
    }

    Xvr_Literal procLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal procLiteralIdn = procLiteral;
    if (XVR_IS_IDENTIFIER(procLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &procLiteral)) {
        Xvr_freeLiteral(procLiteralIdn);
    }

    if (!(XVR_IS_ARRAY(selfLiteral) || XVR_IS_DICTIONARY(selfLiteral)) ||
        !(XVR_IS_FUNCTION(procLiteral) ||
          XVR_IS_FUNCTION_NATIVE(procLiteral))) {
        interpreter->errorOutput("incorrect argument type passed to _some\n");
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    if (XVR_IS_ARRAY(selfLiteral)) {
        bool result = false;

        for (int i = 0; i < XVR_AS_ARRAY(selfLiteral)->count; i++) {
            Xvr_Literal indexLiteral = XVR_TO_INTEGER_LITERAL(i);

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(&arguments,
                                 XVR_AS_ARRAY(selfLiteral)->literals[i]);
            Xvr_pushLiteralArray(&arguments, indexLiteral);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_Literal lit = Xvr_popLiteralArray(&returns);

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
            Xvr_freeLiteral(indexLiteral);

            if (XVR_IS_TRUTHY(lit)) {
                Xvr_freeLiteral(lit);
                result = true;
                break;
            }

            Xvr_freeLiteral(lit);
        }

        Xvr_Literal resultLiteral = XVR_TO_BOOLEAN_LITERAL(result);
        Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
        Xvr_freeLiteral(resultLiteral);
    }

    if (XVR_IS_DICTIONARY(selfLiteral)) {
        bool result = true;

        for (int i = 0; i < XVR_AS_DICTIONARY(selfLiteral)->capacity; i++) {
            if (XVR_IS_NULL(XVR_AS_DICTIONARY(selfLiteral)->entries[i].key)) {
                continue;
            }

            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].value);
            Xvr_pushLiteralArray(
                &arguments, XVR_AS_DICTIONARY(selfLiteral)->entries[i].key);

            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callLiteralFn(interpreter, procLiteral, &arguments, &returns);

            Xvr_Literal lit = Xvr_popLiteralArray(&returns);

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);

            if (XVR_IS_TRUTHY(lit)) {
                Xvr_freeLiteral(lit);
                result = true;
                break;
            }

            Xvr_freeLiteral(lit);
        }

        Xvr_Literal resultLiteral = XVR_TO_BOOLEAN_LITERAL(result);
        Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
        Xvr_freeLiteral(resultLiteral);
    }

    Xvr_freeLiteral(procLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 0;
}

static void swapLiteralsUtil(Xvr_Literal* lhs, Xvr_Literal* rhs) {
    Xvr_Literal tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

static void recursiveLiteralQuickSortUtil(Xvr_Interpreter* interpreter,
                                          Xvr_Literal* ptr, int literalCount,
                                          Xvr_Literal fnCompareLiteral) {
    if (literalCount <= 1) {
        return;
    }

    int runner = 0;

    for (int checker = 0; checker < literalCount - 1; checker++) {
        Xvr_LiteralArray arguments;
        Xvr_LiteralArray returns;

        Xvr_initLiteralArray(&arguments);
        Xvr_initLiteralArray(&returns);

        Xvr_pushLiteralArray(&arguments, ptr[literalCount - 1]);
        Xvr_pushLiteralArray(&arguments, ptr[checker]);

        Xvr_callLiteralFn(interpreter, fnCompareLiteral, &arguments, &returns);

        Xvr_Literal lessThan = Xvr_popLiteralArray(&returns);

        Xvr_freeLiteralArray(&arguments);
        Xvr_freeLiteralArray(&returns);

        if (XVR_IS_TRUTHY(lessThan)) {
            swapLiteralsUtil(&ptr[runner++], &ptr[checker]);
        }

        Xvr_freeLiteral(lessThan);
    }

    swapLiteralsUtil(&ptr[runner], &ptr[literalCount - 1]);

    recursiveLiteralQuickSortUtil(interpreter, &ptr[0], runner,
                                  fnCompareLiteral);
    recursiveLiteralQuickSortUtil(interpreter, &ptr[runner + 1],
                                  literalCount - runner - 1, fnCompareLiteral);
}

static int nativeSort(Xvr_Interpreter* interpreter,
                      Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("incorrect number of arguments to _sort\n");
        return -1;
    }

    Xvr_Literal fnLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;

    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    Xvr_Literal fnLiteralIdn = fnLiteral;
    if (XVR_IS_IDENTIFIER(fnLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &fnLiteral)) {
        Xvr_freeLiteral(fnLiteralIdn);
    }

    if (!XVR_IS_ARRAY(selfLiteral) ||
        !(XVR_IS_FUNCTION(fnLiteral) || XVR_IS_FUNCTION_NATIVE(fnLiteral))) {
        interpreter->errorOutput("incorrect argument type to passing _sort\n");
        Xvr_freeLiteral(selfLiteral);
        Xvr_freeLiteral(fnLiteral);
        return -1;
    }

    if (XVR_IS_ARRAY(selfLiteral)) {
        recursiveLiteralQuickSortUtil(
            interpreter, XVR_AS_ARRAY(selfLiteral)->literals,
            XVR_AS_ARRAY(selfLiteral)->count, fnLiteral);
    }

    Xvr_pushLiteralArray(&interpreter->stack, selfLiteral);

    Xvr_freeLiteral(fnLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static int nativeToLower(Xvr_Interpreter* interpreter,
                         Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput("Incorrect number of arguments to _toLower\n");
        return -1;
    }

    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    if (!XVR_IS_STRING(selfLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _toLower\n");
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    Xvr_RefString* selfRefString = XVR_AS_STRING(selfLiteral);
    const char* self = Xvr_toCString(selfRefString);

    char* result = XVR_ALLOCATE(char, Xvr_lengthRefString(selfRefString) + 1);

    for (int i = 0; i < (int)Xvr_lengthRefString(selfRefString); i++) {
        result[i] = tolower(self[i]);
    }
    result[Xvr_lengthRefString(selfRefString)] = '\0';

    Xvr_RefString* resultRefString =
        Xvr_createRefStringLength(result, Xvr_lengthRefString(selfRefString));
    Xvr_Literal resultLiteral = XVR_TO_STRING_LITERAL(resultRefString);

    Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);

    XVR_FREE_ARRAY(char, result, Xvr_lengthRefString(resultRefString) + 1);
    Xvr_freeLiteral(resultLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static char* toStringUtilObject = NULL;
static void toStringUtil(const char* input) {
    size_t len = strlen(input) + 1;

    if (len > XVR_MAX_STRING_LENGTH) {
        len = XVR_MAX_STRING_LENGTH;
    }

    toStringUtilObject = XVR_ALLOCATE(char, len);

    snprintf(toStringUtilObject, len, "%s", input);
}

static int nativeToString(Xvr_Interpreter* interpreter,
                          Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _toString\n");
        return -1;
    }

    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    if (XVR_IS_IDENTIFIER(selfLiteral)) {
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    Xvr_printLiteralCustom(selfLiteral, toStringUtil);

    Xvr_Literal result =
        XVR_TO_STRING_LITERAL(Xvr_createRefString(toStringUtilObject));

    Xvr_pushLiteralArray(&interpreter->stack, result);

    XVR_FREE_ARRAY(char, toStringUtilObject,
                   Xvr_lengthRefString(XVR_AS_STRING(result)) + 1);
    toStringUtilObject = NULL;

    Xvr_freeLiteral(result);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static int nativeToUpper(Xvr_Interpreter* interpreter,
                         Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput("Incorrect number of arguments to _toUpper\n");
        return -1;
    }

    Xvr_Literal selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    if (!XVR_IS_STRING(selfLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _toUpper\n");
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    Xvr_RefString* selfRefString = XVR_AS_STRING(selfLiteral);
    const char* self = Xvr_toCString(selfRefString);

    char* result = XVR_ALLOCATE(char, Xvr_lengthRefString(selfRefString) + 1);

    for (int i = 0; i < (int)Xvr_lengthRefString(selfRefString); i++) {
        result[i] = toupper(self[i]);
    }
    result[Xvr_lengthRefString(selfRefString)] = '\0';

    Xvr_RefString* resultRefString =
        Xvr_createRefStringLength(result, Xvr_lengthRefString(selfRefString));
    Xvr_Literal resultLiteral = XVR_TO_STRING_LITERAL(resultRefString);

    Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);

    XVR_FREE_ARRAY(char, result, Xvr_lengthRefString(resultRefString) + 1);
    Xvr_freeLiteral(resultLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static int nativeTrim(Xvr_Interpreter* interpreter,
                      Xvr_LiteralArray* arguments) {
    if (arguments->count < 1 || arguments->count > 2) {
        interpreter->errorOutput("Incorrect number of arguments to _trim\n");
        return -1;
    }

    Xvr_Literal trimCharsLiteral;
    Xvr_Literal selfLiteral;

    if (arguments->count == 2) {
        trimCharsLiteral = Xvr_popLiteralArray(arguments);

        Xvr_Literal trimCharsLiteralIdn = trimCharsLiteral;
        if (XVR_IS_IDENTIFIER(trimCharsLiteral) &&
            Xvr_parseIdentifierToValue(interpreter, &trimCharsLiteral)) {
            Xvr_freeLiteral(trimCharsLiteralIdn);
        }
    } else {
        trimCharsLiteral =
            XVR_TO_STRING_LITERAL(Xvr_createRefString(" \t\n\r"));
    }
    selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    if (!XVR_IS_STRING(selfLiteral)) {
        interpreter->errorOutput("Incorrect argument type passed to _trim\n");
        Xvr_freeLiteral(trimCharsLiteral);
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    Xvr_RefString* trimCharsRefString = XVR_AS_STRING(trimCharsLiteral);
    Xvr_RefString* selfRefString = XVR_AS_STRING(selfLiteral);

    int bufferBegin = 0;
    int bufferEnd = Xvr_lengthRefString(selfRefString);

    for (int i = 0; i < (int)Xvr_lengthRefString(selfRefString); i++) {
        int trimIndex = 0;

        while (trimIndex < (int)Xvr_lengthRefString(trimCharsRefString)) {
            if (Xvr_toCString(selfRefString)[i] ==
                Xvr_toCString(trimCharsRefString)[trimIndex]) {
                break;
            }

            trimIndex++;
        }

        if (trimIndex < (int)Xvr_lengthRefString(trimCharsRefString)) {
            bufferBegin++;
            continue;
        } else {
            break;
        }
    }

    for (int i = Xvr_lengthRefString(selfRefString); i >= 0; i--) {
        int trimIndex = 0;

        while (trimIndex < (int)Xvr_lengthRefString(trimCharsRefString)) {
            if (Xvr_toCString(selfRefString)[i - 1] ==
                Xvr_toCString(trimCharsRefString)[trimIndex]) {
                break;
            }

            trimIndex++;
        }

        if (trimIndex < (int)Xvr_lengthRefString(trimCharsRefString)) {
            bufferEnd--;
            continue;
        } else {
            break;
        }
    }

    Xvr_Literal resultLiteral;
    if (bufferBegin >= bufferEnd) {
        resultLiteral = XVR_TO_STRING_LITERAL(Xvr_createRefString(""));
    } else {
        char buffer[XVR_MAX_STRING_LENGTH];
        snprintf(buffer, bufferEnd - bufferBegin + 1, "%s",
                 &Xvr_toCString(selfRefString)[bufferBegin]);
        resultLiteral = XVR_TO_STRING_LITERAL(Xvr_createRefString(buffer));
    }

    Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);

    Xvr_freeLiteral(resultLiteral);
    Xvr_freeLiteral(trimCharsLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static int nativeTrimBegin(Xvr_Interpreter* interpreter,
                           Xvr_LiteralArray* arguments) {
    if (arguments->count < 1 || arguments->count > 2) {
        interpreter->errorOutput(
            "incorrect number of arguments too _trimBegin\n");
        return -1;
    }

    Xvr_Literal trimCharsLiteral;
    Xvr_Literal selfLiteral;

    if (arguments->count == 2) {
        trimCharsLiteral = Xvr_popLiteralArray(arguments);
        Xvr_Literal trimCharsLiteralIdn = trimCharsLiteral;
        if (XVR_IS_IDENTIFIER(trimCharsLiteral) &&
            Xvr_parseIdentifierToValue(interpreter, &trimCharsLiteral)) {
            Xvr_freeLiteral(trimCharsLiteralIdn);
        }
    } else {
        trimCharsLiteral =
            XVR_TO_STRING_LITERAL(Xvr_createRefString(" \t\n\r"));
    }
    selfLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    if (!XVR_IS_STRING(selfLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _trimBegin\n");
        Xvr_freeLiteral(trimCharsLiteral);
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    Xvr_RefString* trimCharsRefString = XVR_AS_STRING(trimCharsLiteral);
    Xvr_RefString* selfRefString = XVR_AS_STRING(selfLiteral);

    int bufferBegin = 0;
    int bufferEnd = Xvr_lengthRefString(selfRefString);

    for (int i = 0; i < (int)Xvr_lengthRefString(selfRefString); i++) {
        int trimIndex = 0;

        while (trimIndex < (int)Xvr_lengthRefString(trimCharsRefString)) {
            if (Xvr_toCString(selfRefString)[i] ==
                Xvr_toCString(trimCharsRefString)[trimIndex]) {
                break;
            }
            trimIndex++;
        }

        if (trimIndex < (int)Xvr_lengthRefString(trimCharsRefString)) {
            bufferBegin++;
            continue;
        } else {
            break;
        }
    }

    Xvr_Literal resultLiteral;
    if (bufferBegin >= bufferEnd) {
        resultLiteral = XVR_TO_STRING_LITERAL(Xvr_createRefString(""));
    } else {
        char buffer[XVR_MAX_STRING_LENGTH];
        snprintf(buffer, bufferEnd - bufferBegin + 1, "%s",
                 &Xvr_toCString(selfRefString)[bufferBegin]);
        resultLiteral = XVR_TO_STRING_LITERAL(Xvr_createRefString(buffer));
    }

    Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);

    Xvr_freeLiteral(resultLiteral);
    Xvr_freeLiteral(trimCharsLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

static int nativeTrimEnd(Xvr_Interpreter* interpreter,
                         Xvr_LiteralArray* arguments) {
    if (arguments->count < 1 || arguments->count > 2) {
        interpreter->errorOutput("Incorrect number of arguments to _trimEnd\n");
        return -1;
    }

    Xvr_Literal trimCharsLiteral;
    Xvr_Literal selfLiteral;

    if (arguments->count == 2) {
        trimCharsLiteral = Xvr_popLiteralArray(arguments);

        Xvr_Literal trimCharsLiteralIdn = trimCharsLiteral;
        if (XVR_IS_IDENTIFIER(trimCharsLiteral) &&
            Xvr_parseIdentifierToValue(interpreter, &trimCharsLiteral)) {
            Xvr_freeLiteral(trimCharsLiteralIdn);
        }
    } else {
        trimCharsLiteral =
            XVR_TO_STRING_LITERAL(Xvr_createRefString(" \t\n\r"));
    }

    selfLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal selfLiteralIdn = selfLiteral;
    if (XVR_IS_IDENTIFIER(selfLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &selfLiteral)) {
        Xvr_freeLiteral(selfLiteralIdn);
    }

    if (!XVR_IS_STRING(selfLiteral)) {
        interpreter->errorOutput(
            "Incorrect arguments type passed to _trimEnd\n");
        Xvr_freeLiteral(trimCharsLiteral);
        Xvr_freeLiteral(selfLiteral);
        return -1;
    }

    Xvr_RefString* trimCharsRefString = XVR_AS_STRING(trimCharsLiteral);
    Xvr_RefString* selfRefString = XVR_AS_STRING(selfLiteral);

    int bufferBegin = 0;
    int bufferEnd = Xvr_lengthRefString(selfRefString);

    for (int i = (int)Xvr_lengthRefString(selfRefString); i >= 0; i--) {
        int trimIndex = 0;

        while (trimIndex < (int)Xvr_lengthRefString(trimCharsRefString)) {
            if (Xvr_toCString(selfRefString)[i - 1] ==
                Xvr_toCString(trimCharsRefString)[trimIndex]) {
                break;
            }
            trimIndex++;
        }

        if (trimIndex < (int)Xvr_lengthRefString(trimCharsRefString)) {
            bufferEnd--;
            continue;
        } else {
            break;
        }
    }

    Xvr_Literal resultLiteral;
    if (bufferBegin >= bufferEnd) {
        resultLiteral = XVR_TO_STRING_LITERAL(Xvr_createRefString(""));
    } else {
        char buffer[XVR_MAX_STRING_LENGTH];
        snprintf(buffer, bufferEnd - bufferBegin + 1, "%s",
                 &Xvr_toCString(selfRefString)[bufferBegin]);
        resultLiteral = XVR_TO_STRING_LITERAL(Xvr_createRefString(buffer));
    }

    Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);
    Xvr_freeLiteral(resultLiteral);
    Xvr_freeLiteral(trimCharsLiteral);
    Xvr_freeLiteral(selfLiteral);

    return 1;
}

typedef struct Natives {
    char* name;
    Xvr_NativeFn fn;
} Natives;

int Xvr_hookStandard(Xvr_Interpreter* interpreter, Xvr_Literal identifier,
                     Xvr_Literal alias) {
    Natives natives[] = {
        {"clock", nativeClock},
        {"_concat", nativeConcat},                // array, dictionary, string
        {"_containsKey", nativeContainsKey},      // dictionary
        {"_containsValue", nativeContainsValue},  // dictionary
        {"_every", nativeEvery},                  // array, dictionary
        {"_forEach", nativeForEach},              // array, dictionary
        {"_getKeys", nativeGetKeys},              // dictionary
        {"_getValues", nativeGetValues},          // dictionary
        {"_indexOf", nativeIndexOf},              // array
        {"_map", nativeMap},                      // array, dictionary
        {"_reduce", nativeReduce},                // array, dictionary
        {"_some", nativeSome},                    // array, dictionary
        {"_sort", nativeSort},                    // array
        {"_toLower", nativeToLower},               // string
        {"_toString", nativeToString},             // array, dictionary
        {"_toUpper", nativeToUpper},               // string
        {"_trim", nativeTrim},                    // string
        {"_trimBegin", nativeTrimBegin},          // string
        {"_trimEnd", nativeTrimEnd},              // string
        {NULL, NULL},
    };

    if (!XVR_IS_NULL(alias)) {
        if (Xvr_isDeclaredScopeVariable(interpreter->scope, alias)) {
            interpreter->errorOutput("Can't override an existing variable\n");
            Xvr_freeLiteral(alias);
            return -1;
        }

        // create the dictionary to load up with functions
        Xvr_LiteralDictionary* dictionary =
            XVR_ALLOCATE(Xvr_LiteralDictionary, 1);
        Xvr_initLiteralDictionary(dictionary);

        // load the dict with functions
        for (int i = 0; natives[i].name; i++) {
            Xvr_Literal name =
                XVR_TO_STRING_LITERAL(Xvr_createRefString(natives[i].name));
            Xvr_Literal func = XVR_TO_FUNCTION_NATIVE_LITERAL(natives[i].fn);

            Xvr_setLiteralDictionary(dictionary, name, func);

            Xvr_freeLiteral(name);
            Xvr_freeLiteral(func);
        }

        // build the type
        Xvr_Literal type = XVR_TO_TYPE_LITERAL(XVR_LITERAL_DICTIONARY, true);
        Xvr_Literal strType = XVR_TO_TYPE_LITERAL(XVR_LITERAL_STRING, true);
        Xvr_Literal fnType =
            XVR_TO_TYPE_LITERAL(XVR_LITERAL_FUNCTION_NATIVE, true);
        XVR_TYPE_PUSH_SUBTYPE(&type, strType);
        XVR_TYPE_PUSH_SUBTYPE(&type, fnType);

        Xvr_Literal dict = XVR_TO_DICTIONARY_LITERAL(dictionary);
        Xvr_declareScopeVariable(interpreter->scope, alias, type);
        Xvr_setScopeVariable(interpreter->scope, alias, dict, false);

        Xvr_freeLiteral(dict);
        Xvr_freeLiteral(type);
        return 0;
    }

    for (int i = 0; natives[i].name; i++) {
        Xvr_injectNativeFn(interpreter, natives[i].name, natives[i].fn);
    }

    return 0;
}
