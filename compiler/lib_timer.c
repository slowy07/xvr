#include "lib_timer.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "xvr_memory.h"

static int timeval_subtract(struct timeval* result, struct timeval* x,
                            struct timeval* y) {
    if (x->tv_usec > 999999) {
        x->tv_sec += x->tv_usec / 1000000;
        x->tv_usec %= 1000000;
    }

    if (y->tv_usec > 999999) {
        y->tv_sec += y->tv_usec / 1000000;
        y->tv_usec %= 1000000;
    }

    // calc
    result->tv_sec = x->tv_sec - y->tv_sec;

    if ((result->tv_usec = x->tv_usec - y->tv_usec) < 0) {
        if (result->tv_sec != 0) {
            result->tv_usec += 1000000;
            result->tv_sec--;
        }
    }

    return result->tv_sec < 0 || (result->tv_sec == 0 && result->tv_usec < 0);
}

static struct timeval* diff(struct timeval* lhs, struct timeval* rhs) {
    struct timeval* d = XVR_ALLOCATE(struct timeval, 1);

    timeval_subtract(d, rhs, lhs);

    return d;
}

// callbacks
static int nativeStartTimer(Xvr_Interpreter* interpreter,
                            Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 0) {
        interpreter->errorOutput(
            "Incorrect number of arguments to startTimer\n");
        return -1;
    }

    // get the timeinfo from C
    struct timeval* timeinfo = XVR_ALLOCATE(struct timeval, 1);
    gettimeofday(timeinfo, NULL);

    // wrap in an opaque literal for Xvr
    Xvr_Literal timeLiteral = XVR_TO_OPAQUE_LITERAL(timeinfo, -1);
    Xvr_pushLiteralArray(&interpreter->stack, timeLiteral);

    Xvr_freeLiteral(timeLiteral);

    return 1;
}

static int nativeStopTimer(Xvr_Interpreter* interpreter,
                           Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _stopTimer\n");
        return -1;
    }

    struct timeval timerStop;
    gettimeofday(&timerStop, NULL);

    Xvr_Literal timeLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal timeLiteralIdn = timeLiteral;
    if (XVR_IS_IDENTIFIER(timeLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &timeLiteral)) {
        Xvr_freeLiteral(timeLiteralIdn);
    }

    if (!XVR_IS_OPAQUE(timeLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _stopTimer\n");
        Xvr_freeLiteral(timeLiteral);
        return -1;
    }

    struct timeval* timerStart = XVR_AS_OPAQUE(timeLiteral);

    // determine the difference, and wrap it
    struct timeval* d = diff(timerStart, &timerStop);
    Xvr_Literal diffLiteral = XVR_TO_OPAQUE_LITERAL(d, -1);
    Xvr_pushLiteralArray(&interpreter->stack, diffLiteral);

    // cleanup
    Xvr_freeLiteral(timeLiteral);
    Xvr_freeLiteral(diffLiteral);

    return 1;
}

static int nativeCreateTimer(Xvr_Interpreter* interpreter,
                             Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 2) {
        interpreter->errorOutput(
            "Incorrect number of arguments to createTimer\n");
        return -1;
    }

    // get the args
    Xvr_Literal microsecondLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal secondLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal secondLiteralIdn = secondLiteral;
    if (XVR_IS_IDENTIFIER(secondLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &secondLiteral)) {
        Xvr_freeLiteral(secondLiteralIdn);
    }

    Xvr_Literal microsecondLiteralIdn = microsecondLiteral;
    if (XVR_IS_IDENTIFIER(microsecondLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &microsecondLiteral)) {
        Xvr_freeLiteral(microsecondLiteralIdn);
    }

    if (!XVR_IS_INTEGER(secondLiteral) || !XVR_IS_INTEGER(microsecondLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to createTimer\n");
        Xvr_freeLiteral(secondLiteral);
        Xvr_freeLiteral(microsecondLiteral);
        return -1;
    }

    if (XVR_AS_INTEGER(microsecondLiteral) <= -1000 * 1000 ||
        XVR_AS_INTEGER(microsecondLiteral) >= 1000 * 1000 ||
        (XVR_AS_INTEGER(secondLiteral) != 0 &&
         XVR_AS_INTEGER(microsecondLiteral) < 0)) {
        interpreter->errorOutput("Microseconds out of range in createTimer\n");
        Xvr_freeLiteral(secondLiteral);
        Xvr_freeLiteral(microsecondLiteral);
        return -1;
    }

    // get the timeinfo from toy
    struct timeval* timeinfo = XVR_ALLOCATE(struct timeval, 1);
    timeinfo->tv_sec = XVR_AS_INTEGER(secondLiteral);
    timeinfo->tv_usec = XVR_AS_INTEGER(microsecondLiteral);

    // wrap in an opaque literal for Xvr
    Xvr_Literal timeLiteral = XVR_TO_OPAQUE_LITERAL(timeinfo, -1);
    Xvr_pushLiteralArray(&interpreter->stack, timeLiteral);

    Xvr_freeLiteral(timeLiteral);
    Xvr_freeLiteral(secondLiteral);
    Xvr_freeLiteral(microsecondLiteral);

    return 1;
}

static int nativeGetTimerSeconds(Xvr_Interpreter* interpreter,
                                 Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _getTimerSeconds\n");
        return -1;
    }

    // unwrap the opaque literal
    Xvr_Literal timeLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal timeLiteralIdn = timeLiteral;
    if (XVR_IS_IDENTIFIER(timeLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &timeLiteral)) {
        Xvr_freeLiteral(timeLiteralIdn);
    }

    if (!XVR_IS_OPAQUE(timeLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _getTimerSeconds\n");
        Xvr_freeLiteral(timeLiteral);
        return -1;
    }

    struct timeval* timer = XVR_AS_OPAQUE(timeLiteral);

    // create the result literal
    Xvr_Literal result = XVR_TO_INTEGER_LITERAL(timer->tv_sec);
    Xvr_pushLiteralArray(&interpreter->stack, result);

    // cleanup
    Xvr_freeLiteral(timeLiteral);
    Xvr_freeLiteral(result);

    return 1;
}

static int nativeGetTimerMicroseconds(Xvr_Interpreter* interpreter,
                                      Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _getTimerMicroseconds\n");
        return -1;
    }

    // unwrap the opaque literal
    Xvr_Literal timeLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal timeLiteralIdn = timeLiteral;
    if (XVR_IS_IDENTIFIER(timeLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &timeLiteral)) {
        Xvr_freeLiteral(timeLiteralIdn);
    }

    if (!XVR_IS_OPAQUE(timeLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _getTimerMicroseconds\n");
        Xvr_freeLiteral(timeLiteral);
        return -1;
    }

    struct timeval* timer = XVR_AS_OPAQUE(timeLiteral);

    // create the result literal
    Xvr_Literal result = XVR_TO_INTEGER_LITERAL(timer->tv_usec);
    Xvr_pushLiteralArray(&interpreter->stack, result);

    // cleanup
    Xvr_freeLiteral(timeLiteral);
    Xvr_freeLiteral(result);

    return 1;
}

static int nativeCompareTimer(Xvr_Interpreter* interpreter,
                              Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 2) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _compareTimer\n");
        return -1;
    }

    // unwrap the opaque literals
    Xvr_Literal rhsLiteral = Xvr_popLiteralArray(arguments);
    Xvr_Literal lhsLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal lhsLiteralIdn = lhsLiteral;
    if (XVR_IS_IDENTIFIER(lhsLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &lhsLiteral)) {
        Xvr_freeLiteral(lhsLiteralIdn);
    }

    Xvr_Literal rhsLiteralIdn = rhsLiteral;
    if (XVR_IS_IDENTIFIER(rhsLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &rhsLiteral)) {
        Xvr_freeLiteral(rhsLiteralIdn);
    }

    if (!XVR_IS_OPAQUE(lhsLiteral) || !XVR_IS_OPAQUE(rhsLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _compareTimer\n");
        Xvr_freeLiteral(lhsLiteral);
        Xvr_freeLiteral(rhsLiteral);
        return -1;
    }

    struct timeval* lhsTimer = XVR_AS_OPAQUE(lhsLiteral);
    struct timeval* rhsTimer = XVR_AS_OPAQUE(rhsLiteral);

    // determine the difference, and wrap it
    struct timeval* d = diff(lhsTimer, rhsTimer);
    Xvr_Literal diffLiteral = XVR_TO_OPAQUE_LITERAL(d, -1);
    Xvr_pushLiteralArray(&interpreter->stack, diffLiteral);

    // cleanup
    Xvr_freeLiteral(lhsLiteral);
    Xvr_freeLiteral(rhsLiteral);
    Xvr_freeLiteral(diffLiteral);

    return 1;
}

static int nativeTimerToString(Xvr_Interpreter* interpreter,
                               Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _timerToString\n");
        return -1;
    }

    // unwrap in an opaque literal
    Xvr_Literal timeLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal timeLiteralIdn = timeLiteral;
    if (XVR_IS_IDENTIFIER(timeLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &timeLiteral)) {
        Xvr_freeLiteral(timeLiteralIdn);
    }

    if (!XVR_IS_OPAQUE(timeLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _timerToString\n");
        Xvr_freeLiteral(timeLiteral);
        return -1;
    }

    struct timeval* timer = XVR_AS_OPAQUE(timeLiteral);

    // create the string literal
    Xvr_Literal resultLiteral = XVR_TO_NULL_LITERAL;
    if (timer->tv_sec == 0 &&
        timer->tv_usec < 0) {  // special case, for when the negative sign is
                               // encoded in the usec
        char buffer[128];
        snprintf(buffer, 128, "-%ld.%06ld", timer->tv_sec, -timer->tv_usec);
        resultLiteral = XVR_TO_STRING_LITERAL(
            Xvr_createRefStringLength(buffer, strlen(buffer)));
    } else {  // normal case
        char buffer[128];
        snprintf(buffer, 128, "%ld.%06ld", timer->tv_sec, timer->tv_usec);
        resultLiteral = XVR_TO_STRING_LITERAL(
            Xvr_createRefStringLength(buffer, strlen(buffer)));
    }

    Xvr_pushLiteralArray(&interpreter->stack, resultLiteral);

    // cleanup
    Xvr_freeLiteral(timeLiteral);
    Xvr_freeLiteral(resultLiteral);

    return 1;
}

static int nativeDestroyTimer(Xvr_Interpreter* interpreter,
                              Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _destroyTimer\n");
        return -1;
    }

    // unwrap in an opaque literal
    Xvr_Literal timeLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal timeLiteralIdn = timeLiteral;
    if (XVR_IS_IDENTIFIER(timeLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &timeLiteral)) {
        Xvr_freeLiteral(timeLiteralIdn);
    }

    if (!XVR_IS_OPAQUE(timeLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to _destroyTimer\n");
        Xvr_freeLiteral(timeLiteral);
        return -1;
    }

    struct timeval* timer = XVR_AS_OPAQUE(timeLiteral);

    XVR_FREE(struct timeval, timer);

    Xvr_freeLiteral(timeLiteral);

    return 0;
}

// call the hook
typedef struct Natives {
    char* name;
    Xvr_NativeFn fn;
} Natives;

int Xvr_hookTimer(Xvr_Interpreter* interpreter, Xvr_Literal identifier,
                  Xvr_Literal alias) {
    // build the natives list
    Natives natives[] = {{"startTimer", nativeStartTimer},
                         {"_stopTimer", nativeStopTimer},
                         {"createTimer", nativeCreateTimer},
                         {"_getTimerSeconds", nativeGetTimerSeconds},
                         {"_getTimerMicroseconds", nativeGetTimerMicroseconds},
                         {"_compareTimer", nativeCompareTimer},
                         {"_timerToString", nativeTimerToString},
                         {"_destroyTimer", nativeDestroyTimer},
                         {NULL, NULL}};

    // store the library in an aliased dictionary
    if (!XVR_IS_NULL(alias)) {
        // make sure the name isn't taken
        if (Xvr_isDeclaredScopeVariable(interpreter->scope, alias)) {
            interpreter->errorOutput("Can't override an existing variable\n");
            Xvr_freeLiteral(alias);
            return -1;
        }

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

        // set scope
        Xvr_Literal dict = XVR_TO_DICTIONARY_LITERAL(dictionary);
        Xvr_declareScopeVariable(interpreter->scope, alias, type);
        Xvr_setScopeVariable(interpreter->scope, alias, dict, false);

        // cleanup
        Xvr_freeLiteral(dict);
        Xvr_freeLiteral(type);
        return 0;
    }

    // default
    for (int i = 0; natives[i].name; i++) {
        Xvr_injectNativeFn(interpreter, natives[i].name, natives[i].fn);
    }

    return 0;
}
