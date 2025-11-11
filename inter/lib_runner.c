#include "lib_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "inter_tools.h"
#include "xvr_common.h"
#include "xvr_interpreter.h"
#include "xvr_literal.h"
#include "xvr_literal_array.h"
#include "xvr_literal_dictionary.h"
#include "xvr_memory.h"
#include "xvr_refstring.h"
#include "xvr_scope.h"

typedef struct Xvr_Runner {
    Xvr_Interpreter interpreter;
    const unsigned char* bytecode;
    size_t size;

    bool dirty;
} Xvr_Runner;

// Xvr native functions
static int nativeLoadScript(Xvr_Interpreter* interpreter,
                            Xvr_LiteralArray* arguments) {
    // arguments
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to loadScript\n");
        return -1;
    }

    // get the file path literal with a handle
    Xvr_Literal drivePathLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal drivePathLiteralIdn = drivePathLiteral;
    if (XVR_IS_IDENTIFIER(drivePathLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &drivePathLiteral)) {
        Xvr_freeLiteral(drivePathLiteralIdn);
    }

    Xvr_Literal filePathLiteral =
        Xvr_getFilePathLiteral(interpreter, &drivePathLiteral);

    if (XVR_IS_NULL(filePathLiteral)) {
        Xvr_freeLiteral(filePathLiteral);
        Xvr_freeLiteral(drivePathLiteral);
        return -1;
    }

    Xvr_freeLiteral(drivePathLiteral);

    // use raw types - easier
    const char* filePath = Xvr_toCString(XVR_AS_STRING(filePathLiteral));

    // load and compile the bytecode
    size_t fileSize = 0;
    const char* source = Xvr_readFile(filePath, &fileSize);

    if (!source) {
        interpreter->errorOutput("Failed to load source file\n");
        Xvr_freeLiteral(filePathLiteral);
        return -1;
    }

    const unsigned char* bytecode = Xvr_compileString(source, &fileSize);
    free((void*)source);

    if (!bytecode) {
        interpreter->errorOutput("Failed to compile source file\n");
        Xvr_freeLiteral(filePathLiteral);
        return -1;
    }

    // build the runner object
    Xvr_Runner* runner = XVR_ALLOCATE(Xvr_Runner, 1);
    Xvr_setInterpreterPrint(&runner->interpreter, interpreter->printOutput);
    Xvr_setInterpreterAssert(&runner->interpreter, interpreter->assertOutput);
    Xvr_setInterpreterError(&runner->interpreter, interpreter->errorOutput);
    runner->interpreter.hooks = interpreter->hooks;
    runner->interpreter.scope = NULL;
    Xvr_resetInterpreter(&runner->interpreter);
    runner->bytecode = bytecode;
    runner->size = fileSize;
    runner->dirty = false;

    // build the opaque object, and push it to the stack
    Xvr_Literal runnerLiteral =
        XVR_TO_OPAQUE_LITERAL(runner, XVR_OPAQUE_TAG_RUNNER);
    Xvr_pushLiteralArray(&interpreter->stack, runnerLiteral);

    // free the drive path
    Xvr_freeLiteral(filePathLiteral);

    return 1;
}

static int nativeLoadScriptBytecode(Xvr_Interpreter* interpreter,
                                    Xvr_LiteralArray* arguments) {
    // arguments
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to loadScriptBytecode\n");
        return -1;
    }

    // get the argument
    Xvr_Literal drivePathLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal drivePathLiteralIdn = drivePathLiteral;
    if (XVR_IS_IDENTIFIER(drivePathLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &drivePathLiteral)) {
        Xvr_freeLiteral(drivePathLiteralIdn);
    }

    Xvr_RefString* drivePath =
        Xvr_copyRefString(XVR_AS_STRING(drivePathLiteral));

    // get the drive and path as a string (can't trust that pesky strtok -
    // custom split) TODO: move this to refstring library
    size_t driveLength = 0;
    while (Xvr_toCString(drivePath)[driveLength] != ':') {
        if (driveLength >= Xvr_lengthRefString(drivePath)) {
            interpreter->errorOutput(
                "Incorrect drive path format given to loadScriptBytecode\n");
            Xvr_deleteRefString(drivePath);
            Xvr_freeLiteral(drivePathLiteral);
            return -1;
        }

        driveLength++;
    }

    Xvr_RefString* drive =
        Xvr_createRefStringLength(Xvr_toCString(drivePath), driveLength);
    Xvr_RefString* path =
        Xvr_createRefStringLength(&Xvr_toCString(drivePath)[driveLength + 1],
                                  Xvr_lengthRefString(drivePath) - driveLength);

    Xvr_Literal driveLiteral = XVR_TO_STRING_LITERAL(
        drive);  // NOTE: driveLiteral takes ownership of the refString
    Xvr_Literal realDriveLiteral =
        Xvr_getLiteralDictionary(Xvr_getDriveDictionary(), driveLiteral);

    if (!XVR_IS_STRING(realDriveLiteral)) {
        interpreter->errorOutput("Incorrect literal type found for drive: ");
        Xvr_printLiteralCustom(realDriveLiteral, interpreter->errorOutput);
        interpreter->errorOutput("\n");
        Xvr_freeLiteral(realDriveLiteral);
        Xvr_freeLiteral(driveLiteral);
        Xvr_deleteRefString(path);
        Xvr_deleteRefString(drivePath);
        Xvr_freeLiteral(drivePathLiteral);
        return -1;
    }

    Xvr_RefString* realDrive =
        Xvr_copyRefString(XVR_AS_STRING(realDriveLiteral));
    int realLength = Xvr_lengthRefString(realDrive) + Xvr_lengthRefString(path);

    char* filePath = XVR_ALLOCATE(char, realLength + 1);
    snprintf(filePath, realLength, "%s%s", Xvr_toCString(realDrive),
             Xvr_toCString(path));

    Xvr_deleteRefString(realDrive);
    Xvr_freeLiteral(realDriveLiteral);
    Xvr_freeLiteral(driveLiteral);
    Xvr_deleteRefString(path);
    Xvr_deleteRefString(drivePath);
    Xvr_freeLiteral(drivePathLiteral);

    if (!(filePath[realLength - 4] == '.' && filePath[realLength - 3] == 'x' &&
          filePath[realLength - 2] == 'b')) {
        interpreter->errorOutput("Bad binary file extension (expected .tb)\n");
        XVR_FREE_ARRAY(char, filePath, realLength);
        return -1;
    }

    // check for break-out attempts
    for (int i = 0; i < realLength - 1; i++) {
        if (filePath[i] == '.' && filePath[i + 1] == '.') {
            interpreter->errorOutput("Parent directory access not allowed\n");
            XVR_FREE_ARRAY(char, filePath, realLength);
            return -1;
        }
    }

    // load the bytecode
    size_t fileSize = 0;
    unsigned char* bytecode = (unsigned char*)Xvr_readFile(filePath, &fileSize);

    if (!bytecode) {
        interpreter->errorOutput("Failed to load bytecode file\n");
        return -1;
    }

    Xvr_Runner* runner = XVR_ALLOCATE(Xvr_Runner, 1);
    Xvr_setInterpreterPrint(&runner->interpreter, interpreter->printOutput);
    Xvr_setInterpreterAssert(&runner->interpreter, interpreter->assertOutput);
    Xvr_setInterpreterError(&runner->interpreter, interpreter->errorOutput);
    runner->interpreter.hooks = interpreter->hooks;
    runner->interpreter.scope = NULL;
    Xvr_resetInterpreter(&runner->interpreter);
    runner->bytecode = bytecode;
    runner->size = fileSize;
    runner->dirty = false;

    Xvr_Literal runnerLiteral =
        XVR_TO_OPAQUE_LITERAL(runner, XVR_OPAQUE_TAG_RUNNER);
    Xvr_pushLiteralArray(&interpreter->stack, runnerLiteral);

    XVR_FREE_ARRAY(char, filePath, realLength);

    return 1;
}

static int nativeRunScript(Xvr_Interpreter* interpreter,
                           Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _runScript\n");
        return -1;
    }

    Xvr_Literal runnerLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal runnerIdn = runnerLiteral;
    if (XVR_IS_IDENTIFIER(runnerLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &runnerLiteral)) {
        Xvr_freeLiteral(runnerIdn);
    }

    if (XVR_GET_OPAQUE_TAG(runnerLiteral) != XVR_OPAQUE_TAG_RUNNER) {
        interpreter->errorOutput("Unrecognized opaque literal in _runScript\n");
        return -1;
    }

    Xvr_Runner* runner = XVR_AS_OPAQUE(runnerLiteral);

    // run
    if (runner->dirty) {
        interpreter->errorOutput(
            "Can't re-run a dirty script (try resetting it first)\n");
        Xvr_freeLiteral(runnerLiteral);
        return -1;
    }

    unsigned char* bytecodeCopy = XVR_ALLOCATE(unsigned char, runner->size);
    memcpy(bytecodeCopy, runner->bytecode,
           runner->size);  // need a COPY of the bytecode, because the
                           // interpreter eats it

    Xvr_runInterpreter(&runner->interpreter, bytecodeCopy, runner->size);
    runner->dirty = true;

    // cleanup
    Xvr_freeLiteral(runnerLiteral);

    return 0;
}

static int nativeGetScriptVar(Xvr_Interpreter* interpreter,
                              Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 2) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _getScriptVar\n");
        return -1;
    }

    // get the runner object
    Xvr_Literal varName = Xvr_popLiteralArray(arguments);
    Xvr_Literal runnerLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal varNameIdn = varName;
    if (XVR_IS_IDENTIFIER(varName) &&
        Xvr_parseIdentifierToValue(interpreter, &varName)) {
        Xvr_freeLiteral(varNameIdn);
    }

    Xvr_Literal runnerIdn = runnerLiteral;
    if (XVR_IS_IDENTIFIER(runnerLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &runnerLiteral)) {
        Xvr_freeLiteral(runnerIdn);
    }

    if (XVR_GET_OPAQUE_TAG(runnerLiteral) != XVR_OPAQUE_TAG_RUNNER) {
        interpreter->errorOutput("Unrecognized opaque literal in _runScript\n");
        return -1;
    }

    Xvr_Runner* runner = XVR_AS_OPAQUE(runnerLiteral);

    // dirty check
    if (!runner->dirty) {
        interpreter->errorOutput(
            "Can't access variable from a non-dirty script (try running it "
            "first)\n");
        Xvr_freeLiteral(runnerLiteral);
        return -1;
    }

    // get the desired variable
    Xvr_Literal varIdn =
        XVR_TO_IDENTIFIER_LITERAL(Xvr_copyRefString(XVR_AS_STRING(varName)));
    Xvr_Literal result = XVR_TO_NULL_LITERAL;
    Xvr_getScopeVariable(runner->interpreter.scope, varIdn, &result);

    Xvr_pushLiteralArray(&interpreter->stack, result);

    // cleanup
    Xvr_freeLiteral(result);
    Xvr_freeLiteral(varIdn);
    Xvr_freeLiteral(varName);
    Xvr_freeLiteral(runnerLiteral);

    return 1;
}

static int nativeCallScriptFn(Xvr_Interpreter* interpreter,
                              Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count < 2) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _callScriptFn\n");
        return -1;
    }

    // get the rest args
    Xvr_LiteralArray tmp;
    Xvr_initLiteralArray(&tmp);

    while (arguments->count > 2) {
        Xvr_Literal lit = Xvr_popLiteralArray(arguments);
        Xvr_pushLiteralArray(&tmp, lit);
        Xvr_freeLiteral(lit);
    }

    Xvr_LiteralArray rest;
    Xvr_initLiteralArray(&rest);

    while (tmp.count) {  // correct the order of the rest args
        Xvr_Literal lit = Xvr_popLiteralArray(&tmp);
        Xvr_pushLiteralArray(&rest, lit);
        Xvr_freeLiteral(lit);
    }

    Xvr_freeLiteralArray(&tmp);

    // get the runner object
    Xvr_Literal varName = Xvr_popLiteralArray(arguments);
    Xvr_Literal runnerLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal varNameIdn = varName;
    if (XVR_IS_IDENTIFIER(varName) &&
        Xvr_parseIdentifierToValue(interpreter, &varName)) {
        Xvr_freeLiteral(varNameIdn);
    }

    Xvr_Literal runnerIdn = runnerLiteral;
    if (XVR_IS_IDENTIFIER(runnerLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &runnerLiteral)) {
        Xvr_freeLiteral(runnerIdn);
    }

    if (XVR_GET_OPAQUE_TAG(runnerLiteral) != XVR_OPAQUE_TAG_RUNNER) {
        interpreter->errorOutput("Unrecognized opaque literal in _runScript\n");
        return -1;
    }

    Xvr_Runner* runner = XVR_AS_OPAQUE(runnerLiteral);

    // dirty check
    if (!runner->dirty) {
        interpreter->errorOutput(
            "Can't access fn from a non-dirty script (try running it first)\n");
        Xvr_freeLiteral(runnerLiteral);
        Xvr_freeLiteralArray(&rest);
        return -1;
    }

    // get the desired variable
    Xvr_Literal varIdn =
        XVR_TO_IDENTIFIER_LITERAL(Xvr_copyRefString(XVR_AS_STRING(varName)));
    Xvr_Literal fn = XVR_TO_NULL_LITERAL;
    Xvr_getScopeVariable(runner->interpreter.scope, varIdn, &fn);

    if (!XVR_IS_FUNCTION(fn)) {
        interpreter->errorOutput("Can't run a non-function literal\n");
        Xvr_freeLiteral(fn);
        Xvr_freeLiteral(varIdn);
        Xvr_freeLiteral(varName);
        Xvr_freeLiteral(runnerLiteral);
        Xvr_freeLiteralArray(&rest);
    }

    // call
    Xvr_LiteralArray resultArray;
    Xvr_initLiteralArray(&resultArray);

    Xvr_callLiteralFn(interpreter, fn, &rest, &resultArray);

    Xvr_Literal result = XVR_TO_NULL_LITERAL;
    if (resultArray.count > 0) {
        result = Xvr_popLiteralArray(&resultArray);
    }

    Xvr_pushLiteralArray(&interpreter->stack, result);

    // cleanup
    Xvr_freeLiteralArray(&resultArray);
    Xvr_freeLiteral(result);
    Xvr_freeLiteral(fn);
    Xvr_freeLiteral(varIdn);
    Xvr_freeLiteral(varName);
    Xvr_freeLiteral(runnerLiteral);
    Xvr_freeLiteralArray(&rest);

    return 1;
}

static int nativeResetScript(Xvr_Interpreter* interpreter,
                             Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _resetScript\n");
        return -1;
    }

    // get the runner object
    Xvr_Literal runnerLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal runnerIdn = runnerLiteral;
    if (XVR_IS_IDENTIFIER(runnerLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &runnerLiteral)) {
        Xvr_freeLiteral(runnerIdn);
    }

    if (XVR_GET_OPAQUE_TAG(runnerLiteral) != XVR_OPAQUE_TAG_RUNNER) {
        interpreter->errorOutput("Unrecognized opaque literal in _runScript\n");
        return -1;
    }

    Xvr_Runner* runner = XVR_AS_OPAQUE(runnerLiteral);

    // reset
    if (!runner->dirty) {
        interpreter->errorOutput(
            "Can't reset a non-dirty script (try running it first)\n");
        Xvr_freeLiteral(runnerLiteral);
        return -1;
    }

    Xvr_resetInterpreter(&runner->interpreter);
    runner->dirty = false;
    Xvr_freeLiteral(runnerLiteral);

    return 0;
}

static int nativeFreeScript(Xvr_Interpreter* interpreter,
                            Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _freeScript\n");
        return -1;
    }

    // get the runner object
    Xvr_Literal runnerLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal runnerIdn = runnerLiteral;
    if (XVR_IS_IDENTIFIER(runnerLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &runnerLiteral)) {
        Xvr_freeLiteral(runnerIdn);
    }

    if (XVR_GET_OPAQUE_TAG(runnerLiteral) != XVR_OPAQUE_TAG_RUNNER) {
        interpreter->errorOutput(
            "Unrecognized opaque literal in _freeScript\n");
        return -1;
    }

    Xvr_Runner* runner = XVR_AS_OPAQUE(runnerLiteral);

    // clear out the runner object
    runner->interpreter.hooks = NULL;
    Xvr_freeInterpreter(&runner->interpreter);
    XVR_FREE_ARRAY(unsigned char, runner->bytecode, runner->size);

    XVR_FREE(Xvr_Runner, runner);

    Xvr_freeLiteral(runnerLiteral);

    return 0;
}

static int nativeCheckScriptDirty(Xvr_Interpreter* interpreter,
                                  Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 1) {
        interpreter->errorOutput(
            "Incorrect number of arguments to _runScript\n");
        return -1;
    }

    // get the runner object
    Xvr_Literal runnerLiteral = Xvr_popLiteralArray(arguments);

    Xvr_Literal runnerIdn = runnerLiteral;
    if (XVR_IS_IDENTIFIER(runnerLiteral) &&
        Xvr_parseIdentifierToValue(interpreter, &runnerLiteral)) {
        Xvr_freeLiteral(runnerIdn);
    }

    if (XVR_GET_OPAQUE_TAG(runnerLiteral) != XVR_OPAQUE_TAG_RUNNER) {
        interpreter->errorOutput("Unrecognized opaque literal in _runScript\n");
        return -1;
    }

    Xvr_Runner* runner = XVR_AS_OPAQUE(runnerLiteral);

    // run
    Xvr_Literal result = XVR_TO_BOOLEAN_LITERAL(runner->dirty);

    Xvr_pushLiteralArray(&interpreter->stack, result);

    // cleanup
    Xvr_freeLiteral(result);
    Xvr_freeLiteral(runnerLiteral);

    return 0;
}

typedef struct Natives {
    const char* name;
    Xvr_NativeFn fn;
} Natives;

int Xvr_hookRunner(Xvr_Interpreter* interpreter, Xvr_Literal identifier,
                   Xvr_Literal alias) {
    Natives natives[] = {{"loadScript", nativeLoadScript},
                         {"loadScriptBytecode", nativeLoadScriptBytecode},
                         {"_runScript", nativeRunScript},
                         {"_getScriptVar", nativeGetScriptVar},
                         {"_callScriptFn", nativeCallScriptFn},
                         {"_resetScript", nativeResetScript},
                         {"_freeScript", nativeFreeScript},
                         {"_checkScriptDirty", nativeCheckScriptDirty},
                         {NULL, NULL}};

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

static Xvr_LiteralDictionary Xvr_driveDictionary;

void Xvr_initDriveDictionary(void) {
    Xvr_initLiteralDictionary(&Xvr_driveDictionary);
}

void Xvr_freeDriveDictionary(void) {
    Xvr_freeLiteralDictionary(&Xvr_driveDictionary);
}

Xvr_LiteralDictionary* Xvr_getDriveDictionary(void) {
    return &Xvr_driveDictionary;
}

Xvr_Literal Xvr_getFilePathLiteral(Xvr_Interpreter* interpreter,
                                   Xvr_Literal* drivePathLiteral) {
    // check argument types
    if (!XVR_IS_STRING(*drivePathLiteral)) {
        interpreter->errorOutput(
            "Incorrect argument type passed to Xvr_getFilePathLiteral\n");
        return XVR_TO_NULL_LITERAL;
    }

    Xvr_RefString* drivePath =
        Xvr_copyRefString(XVR_AS_STRING(*drivePathLiteral));

    size_t driveLength = 0;
    while (Xvr_toCString(drivePath)[driveLength] != ':') {
        if (driveLength >= Xvr_lengthRefString(drivePath)) {
            interpreter->errorOutput(
                "Incorrect drive path format given to "
                "Xvr_getFilePathLiteral\n");

            return XVR_TO_NULL_LITERAL;
        }

        driveLength++;
    }

    Xvr_RefString* drive =
        Xvr_createRefStringLength(Xvr_toCString(drivePath), driveLength);
    Xvr_RefString* path =
        Xvr_createRefStringLength(&Xvr_toCString(drivePath)[driveLength + 1],
                                  Xvr_lengthRefString(drivePath) - driveLength);

    // get the real drive file path
    Xvr_Literal driveLiteral = XVR_TO_STRING_LITERAL(
        drive);  // NOTE: driveLiteral takes ownership of the refString
    Xvr_Literal realDriveLiteral =
        Xvr_getLiteralDictionary(Xvr_getDriveDictionary(), driveLiteral);

    if (!XVR_IS_STRING(realDriveLiteral)) {
        interpreter->errorOutput("Incorrect literal type found for drive: ");
        Xvr_printLiteralCustom(realDriveLiteral, interpreter->errorOutput);
        interpreter->errorOutput("\n");
        Xvr_freeLiteral(realDriveLiteral);
        Xvr_freeLiteral(driveLiteral);
        Xvr_deleteRefString(path);
        Xvr_deleteRefString(drivePath);

        return XVR_TO_NULL_LITERAL;
    }

    Xvr_RefString* realDrive =
        Xvr_copyRefString(XVR_AS_STRING(realDriveLiteral));
    int realLength = Xvr_lengthRefString(realDrive) + Xvr_lengthRefString(path);

    char* filePath = XVR_ALLOCATE(char, realLength + 1);
    snprintf(filePath, realLength, "%s%s", Xvr_toCString(realDrive),
             Xvr_toCString(path));

    Xvr_deleteRefString(realDrive);
    Xvr_freeLiteral(realDriveLiteral);
    Xvr_freeLiteral(driveLiteral);
    Xvr_deleteRefString(path);
    Xvr_deleteRefString(drivePath);

    for (int i = 0; i < realLength - 1; i++) {
        if (filePath[i] == '.' && filePath[i + 1] == '.') {
            interpreter->errorOutput("Parent directory access not allowed\n");
            XVR_FREE_ARRAY(char, filePath, realLength + 1);
            return XVR_TO_NULL_LITERAL;
        }
    }

    Xvr_Literal result =
        XVR_TO_STRING_LITERAL(Xvr_createRefStringLength(filePath, realLength));

    XVR_FREE_ARRAY(char, filePath, realLength + 1);

    return result;
}
