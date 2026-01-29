#include "xvr_interpreter.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_builtin.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_keyword_types.h"
#include "xvr_literal.h"
#include "xvr_literal_array.h"
#include "xvr_literal_dictionary.h"
#include "xvr_memory.h"
#include "xvr_opcodes.h"
#include "xvr_refstring.h"
#include "xvr_scope.h"
#include "xvr_token_types.h"

static void printWrapper(const char* output) {
    if (Xvr_commandLine.enablePrintNewline) {
        printf("%s\n", output);
    } else {
        printf("%s", output);
    }
}

static void assertWrapper(const char* output) {
    fprintf(stderr, XVR_CC_ERROR "Assertion failure: ");
    fprintf(stderr, "%s", output);
    fprintf(stderr, "\n" XVR_CC_RESET);  // default new line
}

static void errorWrapper(const char* output) {
    fprintf(stderr, XVR_CC_ERROR "%s" XVR_CC_RESET, output);  // no newline
}

bool Xvr_injectNativeFn(Xvr_Interpreter* interpreter, const char* name,
                        Xvr_NativeFn func) {
    // reject reserved words
    if (Xvr_findTypeByKeyword(name) != XVR_TOKEN_EOF) {
        interpreter->errorOutput("Can't override an existing keyword\n");
        return false;
    }

    int identifierLength = strlen(name);
    Xvr_Literal identifier = XVR_TO_IDENTIFIER_LITERAL(
        Xvr_createRefStringLength(name, identifierLength));

    // make sure the name isn't taken
    if (Xvr_existsLiteralDictionary(&interpreter->scope->variables,
                                    identifier)) {
        interpreter->errorOutput("Can't override an existing variable\n");
        return false;
    }

    Xvr_Literal fn = XVR_TO_FUNCTION_NATIVE_LITERAL(func);
    Xvr_Literal type = XVR_TO_TYPE_LITERAL(fn.type, true);

    Xvr_setLiteralDictionary(&interpreter->scope->variables, identifier, fn);
    Xvr_setLiteralDictionary(&interpreter->scope->types, identifier, type);

    Xvr_freeLiteral(identifier);
    Xvr_freeLiteral(type);

    return true;
}

bool Xvr_injectNativeHook(Xvr_Interpreter* interpreter, const char* name,
                          Xvr_HookFn hook) {
    // reject reserved words
    if (Xvr_findTypeByKeyword(name) != XVR_TOKEN_EOF) {
        interpreter->errorOutput(
            "Can't inject a hook on an existing keyword\n");
        return false;
    }

    int identifierLength = strlen(name);
    Xvr_Literal identifier = XVR_TO_IDENTIFIER_LITERAL(
        Xvr_createRefStringLength(name, identifierLength));

    // make sure the name isn't taken
    if (Xvr_existsLiteralDictionary(interpreter->hooks, identifier)) {
        interpreter->errorOutput("Can't override an existing hook\n");
        return false;
    }

    Xvr_Literal fn = XVR_TO_FUNCTION_HOOK_LITERAL(hook);
    Xvr_setLiteralDictionary(interpreter->hooks, identifier, fn);

    Xvr_freeLiteral(identifier);

    return true;
}

void Xvr_parseCompoundToPureValues(Xvr_Interpreter* interpreter,
                                   Xvr_Literal* literalPtr) {
    if (XVR_IS_IDENTIFIER(*literalPtr)) {
        Xvr_parseIdentifierToValue(interpreter, literalPtr);
    }

    // parse out an array
    if (XVR_IS_ARRAY(*literalPtr)) {
        for (int i = 0; i < XVR_AS_ARRAY(*literalPtr)->count; i++) {
            Xvr_Literal index = XVR_TO_INTEGER_LITERAL(i);
            Xvr_Literal entry =
                Xvr_getLiteralArray(XVR_AS_ARRAY(*literalPtr), index);

            if (XVR_IS_IDENTIFIER(entry)) {
                Xvr_Literal idn = entry;
                Xvr_parseCompoundToPureValues(interpreter, &entry);

                Xvr_setLiteralArray(XVR_AS_ARRAY(*literalPtr), index, entry);

                Xvr_freeLiteral(idn);
            }

            Xvr_freeLiteral(index);
            Xvr_freeLiteral(entry);
        }
    }

    // parse out a dictionary
    if (XVR_IS_DICTIONARY(*literalPtr)) {
        Xvr_LiteralDictionary* ret = XVR_ALLOCATE(Xvr_LiteralDictionary, 1);
        Xvr_initLiteralDictionary(ret);

        for (int i = 0; i < XVR_AS_DICTIONARY(*literalPtr)->capacity; i++) {
            if (XVR_IS_NULL(XVR_AS_DICTIONARY(*literalPtr)->entries[i].key)) {
                continue;
            }

            Xvr_Literal key = XVR_TO_NULL_LITERAL;
            Xvr_Literal value = XVR_TO_NULL_LITERAL;

            key =
                Xvr_copyLiteral(XVR_AS_DICTIONARY(*literalPtr)->entries[i].key);
            value = Xvr_copyLiteral(
                XVR_AS_DICTIONARY(*literalPtr)->entries[i].value);

            //
            if (XVR_IS_IDENTIFIER(key) || XVR_IS_IDENTIFIER(value)) {
                Xvr_parseCompoundToPureValues(interpreter, &key);
                Xvr_parseCompoundToPureValues(interpreter, &value);
            }

            Xvr_setLiteralDictionary(ret, key, value);

            //
            Xvr_freeLiteral(key);
            Xvr_freeLiteral(value);
        }

        Xvr_freeLiteral(*literalPtr);
        *literalPtr = XVR_TO_DICTIONARY_LITERAL(ret);
    }
}

bool Xvr_parseIdentifierToValue(Xvr_Interpreter* interpreter,
                                Xvr_Literal* literalPtr) {
    // this converts identifiers to values
    if (XVR_IS_IDENTIFIER(*literalPtr)) {
        if (!Xvr_getScopeVariable(interpreter->scope, *literalPtr,
                                  literalPtr)) {
            interpreter->errorOutput("Undeclared variable ");
            Xvr_printLiteralCustom(*literalPtr, interpreter->errorOutput);
            interpreter->errorOutput("\n");
            return false;
        }
    }

    if (XVR_IS_ARRAY(*literalPtr) || XVR_IS_DICTIONARY(*literalPtr)) {
        Xvr_parseCompoundToPureValues(interpreter, literalPtr);
    }

    return true;
}

// utilities for the host program
void Xvr_setInterpreterPrint(Xvr_Interpreter* interpreter,
                             Xvr_PrintFn printOutput) {
    interpreter->printOutput = printOutput;
}

void Xvr_setInterpreterAssert(Xvr_Interpreter* interpreter,
                              Xvr_PrintFn assertOutput) {
    interpreter->assertOutput = assertOutput;
}

void Xvr_setInterpreterError(Xvr_Interpreter* interpreter,
                             Xvr_PrintFn errorOutput) {
    interpreter->errorOutput = errorOutput;
}

// utils
static unsigned char readByte(const unsigned char* tb, int* count) {
    unsigned char ret = *(unsigned char*)(tb + *count);
    *count += 1;
    return ret;
}

static unsigned short readShort(const unsigned char* tb, int* count) {
    unsigned short ret = 0;
    memcpy(&ret, tb + *count, 2);
    *count += 2;
    return ret;
}

static int readInt(const unsigned char* tb, int* count) {
    int ret = 0;
    memcpy(&ret, tb + *count, 4);
    *count += 4;
    return ret;
}

static float readFloat(const unsigned char* tb, int* count) {
    float ret = 0;
    memcpy(&ret, tb + *count, 4);
    *count += 4;
    return ret;
}

static const char* readString(const unsigned char* tb, int* count) {
    const unsigned char* ret = tb + *count;
    *count += strlen((char*)ret) + 1;  //+1 for null character
    return (const char*)ret;
}

static void consumeByte(Xvr_Interpreter* interpreter, const unsigned char byte,
                        const unsigned char* tb, int* count) {
    if (byte != tb[*count]) {
        char buffer[512];
        snprintf(buffer, 512,
                 "[internal] Failed to consume the correct byte (expected %u, "
                 "found %u)\n",
                 byte, tb[*count]);
        interpreter->errorOutput(buffer);
    }
    *count += 1;
}

static void consumeShort(Xvr_Interpreter* interpreter, unsigned short bytes,
                         const unsigned char* tb, int* count) {
    if (bytes != *(unsigned short*)(tb + *count)) {
        char buffer[512];
        snprintf(buffer, 512,
                 "[internal] Failed to consume the correct bytes (expected %u, "
                 "found %u)\n",
                 bytes, *(unsigned short*)(tb + *count));
        interpreter->errorOutput(buffer);
    }
    *count += 2;
}

// each available statement
static bool execAssert(Xvr_Interpreter* interpreter) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal lhs = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal lhsIdn = lhs;
    if (XVR_IS_IDENTIFIER(lhs) &&
        Xvr_parseIdentifierToValue(interpreter, &lhs)) {
        Xvr_freeLiteral(lhsIdn);
    }

    if (!XVR_IS_STRING(rhs)) {
        interpreter->errorOutput(
            "The assert keyword needs a string as the second argument, "
            "received: ");
        Xvr_printLiteralCustom(rhs, interpreter->errorOutput);
        interpreter->errorOutput("\n");

        Xvr_freeLiteral(rhs);
        Xvr_freeLiteral(lhs);
        return false;
    }

    if (XVR_IS_NULL(lhs) || !XVR_IS_TRUTHY(lhs)) {
        (*interpreter->assertOutput)(Xvr_toCString(XVR_AS_STRING(rhs)));
        interpreter->panic = true;

        Xvr_freeLiteral(rhs);
        Xvr_freeLiteral(lhs);
        return false;
    }

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return true;
}

static bool execPrint(Xvr_Interpreter* interpreter) {
    // print what is on top of the stack, then pop it
    Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal idn = lit;
    if (XVR_IS_IDENTIFIER(lit) &&
        Xvr_parseIdentifierToValue(interpreter, &lit)) {
        Xvr_freeLiteral(idn);
    }

    Xvr_printLiteralCustom(lit, interpreter->printOutput);

    Xvr_freeLiteral(lit);

    return true;
}

static bool execPushLiteral(Xvr_Interpreter* interpreter, bool lng) {
    // read the index in the cache
    int index = 0;

    if (lng) {
        index = (int)readShort(interpreter->bytecode, &interpreter->count);
    } else {
        index = (int)readByte(interpreter->bytecode, &interpreter->count);
    }

    // push from cache to stack (DO NOT account for identifiers - will do that
    // later)
    Xvr_pushLiteralArray(&interpreter->stack,
                         interpreter->literalCache.literals[index]);

    return true;
}

static bool rawLiteral(Xvr_Interpreter* interpreter) {
    Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal idn = lit;
    if (XVR_IS_IDENTIFIER(lit) &&
        Xvr_parseIdentifierToValue(interpreter, &lit)) {
        Xvr_freeLiteral(idn);
    }

    Xvr_pushLiteralArray(&interpreter->stack, lit);
    Xvr_freeLiteral(lit);

    return true;
}

static bool execNegate(Xvr_Interpreter* interpreter) {
    // negate the top literal on the stack (numbers only)
    Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal idn = lit;
    if (XVR_IS_IDENTIFIER(lit) &&
        Xvr_parseIdentifierToValue(interpreter, &lit)) {
        Xvr_freeLiteral(idn);
    }

    if (XVR_IS_INTEGER(lit)) {
        lit = XVR_TO_INTEGER_LITERAL(-XVR_AS_INTEGER(lit));
    } else if (XVR_IS_FLOAT(lit)) {
        lit = XVR_TO_FLOAT_LITERAL(-XVR_AS_FLOAT(lit));
    } else {
        interpreter->errorOutput("Can't negate that literal: ");
        Xvr_printLiteralCustom(lit, interpreter->errorOutput);
        interpreter->errorOutput("\n");

        Xvr_freeLiteral(lit);

        return false;
    }

    Xvr_pushLiteralArray(&interpreter->stack, lit);
    Xvr_freeLiteral(lit);

    return true;
}

static bool execInvert(Xvr_Interpreter* interpreter) {
    // negate the top literal on the stack (booleans only)
    Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal idn = lit;
    if (XVR_IS_IDENTIFIER(lit) &&
        Xvr_parseIdentifierToValue(interpreter, &lit)) {
        Xvr_freeLiteral(idn);
    }

    if (XVR_IS_BOOLEAN(lit)) {
        lit = XVR_TO_BOOLEAN_LITERAL(!XVR_AS_BOOLEAN(lit));
    } else {
        interpreter->errorOutput("Can't invert that literal: ");
        Xvr_printLiteralCustom(lit, interpreter->errorOutput);
        interpreter->errorOutput("\n");

        Xvr_freeLiteral(lit);

        return false;
    }

    Xvr_pushLiteralArray(&interpreter->stack, lit);
    Xvr_freeLiteral(lit);

    return true;
}

static bool execArithmetic(Xvr_Interpreter* interpreter, Xvr_Opcode opcode) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal lhs = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal rhsIdn = rhs;
    if (XVR_IS_IDENTIFIER(rhs) &&
        Xvr_parseIdentifierToValue(interpreter, &rhs)) {
        Xvr_freeLiteral(rhsIdn);
    }

    Xvr_Literal lhsIdn = lhs;
    if (XVR_IS_IDENTIFIER(lhs) &&
        Xvr_parseIdentifierToValue(interpreter, &lhs)) {
        Xvr_freeLiteral(lhsIdn);
    }

    // special case for string concatenation ONLY
    if (XVR_IS_STRING(lhs) && XVR_IS_STRING(rhs) &&
        (opcode == XVR_OP_ADDITION || opcode == XVR_OP_VAR_ADDITION_ASSIGN)) {
        // check for overflow
        int totalLength =
            XVR_AS_STRING(lhs)->length + XVR_AS_STRING(rhs)->length;
        if (totalLength > XVR_MAX_STRING_LENGTH) {
            interpreter->errorOutput(
                "Can't concatenate these strings, result is too long (error "
                "found in interpreter)\n");
            return false;
        }

        // concat the strings
        char buffer[XVR_MAX_STRING_LENGTH];
        snprintf(buffer, XVR_MAX_STRING_LENGTH, "%s%s",
                 Xvr_toCString(XVR_AS_STRING(lhs)),
                 Xvr_toCString(XVR_AS_STRING(rhs)));
        Xvr_Literal literal = XVR_TO_STRING_LITERAL(
            Xvr_createRefStringLength(buffer, totalLength));
        Xvr_pushLiteralArray(&interpreter->stack, literal);

        // cleanup
        Xvr_freeLiteral(literal);
        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);

        return true;
    }

    // type coersion
    if (XVR_IS_FLOAT(lhs) && XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    if (XVR_IS_INTEGER(lhs) && XVR_IS_FLOAT(rhs)) {
        lhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(lhs));
    }

    // maths based on types
    if (XVR_IS_INTEGER(lhs) && XVR_IS_INTEGER(rhs)) {
        switch (opcode) {
        case XVR_OP_ADDITION:
        case XVR_OP_VAR_ADDITION_ASSIGN:
            Xvr_pushLiteralArray(&interpreter->stack,
                                 XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) +
                                                        XVR_AS_INTEGER(rhs)));
            return true;

        case XVR_OP_SUBTRACTION:
        case XVR_OP_VAR_SUBTRACTION_ASSIGN:
            Xvr_pushLiteralArray(&interpreter->stack,
                                 XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) -
                                                        XVR_AS_INTEGER(rhs)));
            return true;

        case XVR_OP_MULTIPLICATION:
        case XVR_OP_VAR_MULTIPLICATION_ASSIGN:
            Xvr_pushLiteralArray(&interpreter->stack,
                                 XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) *
                                                        XVR_AS_INTEGER(rhs)));
            return true;

        case XVR_OP_DIVISION:
        case XVR_OP_VAR_DIVISION_ASSIGN:
            if (XVR_AS_INTEGER(rhs) == 0) {
                interpreter->errorOutput(
                    "Can't divide by zero (error found in interpreter)\n");
                return false;
            }
            Xvr_pushLiteralArray(&interpreter->stack,
                                 XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) /
                                                        XVR_AS_INTEGER(rhs)));
            return true;

        case XVR_OP_MODULO:
        case XVR_OP_VAR_MODULO_ASSIGN:
            if (XVR_AS_INTEGER(rhs) == 0) {
                interpreter->errorOutput(
                    "Can't modulo by zero (error found in interpreter)\n");
                return false;
            }
            Xvr_pushLiteralArray(&interpreter->stack,
                                 XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) %
                                                        XVR_AS_INTEGER(rhs)));
            return true;

        default:
            interpreter->errorOutput(
                "[internal] bad opcode argument passed to execArithmetic()\n");
            return false;
        }
    }

    // catch bad modulo
    if (opcode == XVR_OP_MODULO || opcode == XVR_OP_VAR_MODULO_ASSIGN) {
        interpreter->errorOutput(
            "Bad arithmetic argument (modulo on floats not allowed)\n");
        return false;
    }

    if (XVR_IS_FLOAT(lhs) && XVR_IS_FLOAT(rhs)) {
        switch (opcode) {
        case XVR_OP_ADDITION:
        case XVR_OP_VAR_ADDITION_ASSIGN:
            Xvr_pushLiteralArray(
                &interpreter->stack,
                XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) + XVR_AS_FLOAT(rhs)));
            return true;

        case XVR_OP_SUBTRACTION:
        case XVR_OP_VAR_SUBTRACTION_ASSIGN:
            Xvr_pushLiteralArray(
                &interpreter->stack,
                XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) - XVR_AS_FLOAT(rhs)));
            return true;

        case XVR_OP_MULTIPLICATION:
        case XVR_OP_VAR_MULTIPLICATION_ASSIGN:
            Xvr_pushLiteralArray(
                &interpreter->stack,
                XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) * XVR_AS_FLOAT(rhs)));
            return true;

        case XVR_OP_DIVISION:
        case XVR_OP_VAR_DIVISION_ASSIGN:
            if (XVR_AS_FLOAT(rhs) == 0) {
                interpreter->errorOutput(
                    "Can't divide by zero (error found in interpreter)\n");
                return false;
            }
            Xvr_pushLiteralArray(
                &interpreter->stack,
                XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) / XVR_AS_FLOAT(rhs)));
            return true;

        default:
            interpreter->errorOutput(
                "[internal] bad opcode argument passed to execArithmetic()\n");
            return false;
        }
    }

    // wrong types
    interpreter->errorOutput("Bad arithmetic argument ");
    Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
    interpreter->errorOutput(" and ");
    Xvr_printLiteralCustom(rhs, interpreter->errorOutput);
    interpreter->errorOutput("\n");

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return false;
}

static Xvr_Literal parseTypeToValue(Xvr_Interpreter* interpreter,
                                    Xvr_Literal type) {
    // if an identifier is embedded in the type, figure out what it iss
    Xvr_Literal typeIdn = type;
    if (XVR_IS_IDENTIFIER(type) &&
        Xvr_parseIdentifierToValue(interpreter, &type)) {
        Xvr_freeLiteral(typeIdn);
    }

    // if this is an array or dictionary, continue to the subtypes
    if (XVR_IS_TYPE(type) &&
        (XVR_AS_TYPE(type).typeOf == XVR_LITERAL_ARRAY ||
         XVR_AS_TYPE(type).typeOf == XVR_LITERAL_DICTIONARY)) {
        for (int i = 0; i < XVR_AS_TYPE(type).count; i++) {
            ((Xvr_Literal*)(XVR_AS_TYPE(type).subtypes))[i] = parseTypeToValue(
                interpreter, ((Xvr_Literal*)(XVR_AS_TYPE(type).subtypes))[i]);
        }
    }

    if (!XVR_IS_TYPE(type)) {
        interpreter->errorOutput("Bad type encountered: ");
        Xvr_printLiteralCustom(type, interpreter->errorOutput);
        interpreter->errorOutput("\n");
        // TODO: would be better to return an int here...
    }

    return type;
}

static bool execVarDecl(Xvr_Interpreter* interpreter, bool lng) {
    // read the index in the cache
    int identifierIndex = 0;
    int typeIndex = 0;

    if (lng) {
        identifierIndex =
            (int)readShort(interpreter->bytecode, &interpreter->count);
        typeIndex = (int)readShort(interpreter->bytecode, &interpreter->count);
    } else {
        identifierIndex =
            (int)readByte(interpreter->bytecode, &interpreter->count);
        typeIndex = (int)readByte(interpreter->bytecode, &interpreter->count);
    }

    Xvr_Literal identifier =
        interpreter->literalCache.literals[identifierIndex];
    Xvr_Literal type =
        Xvr_copyLiteral(interpreter->literalCache.literals[typeIndex]);

    Xvr_Literal typeIdn = type;
    if (XVR_IS_IDENTIFIER(type) &&
        Xvr_parseIdentifierToValue(interpreter, &type)) {
        Xvr_freeLiteral(typeIdn);
    }

    type = parseTypeToValue(interpreter, type);

    if (!Xvr_declareScopeVariable(interpreter->scope, identifier, type)) {
        interpreter->errorOutput("Can't redefine the variable \"");
        Xvr_printLiteralCustom(identifier, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");
        return false;
    }

    Xvr_Literal val = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal valIdn = val;
    if (XVR_IS_IDENTIFIER(val) &&
        Xvr_parseIdentifierToValue(interpreter, &val)) {
        Xvr_freeLiteral(valIdn);
    }

    if (XVR_IS_ARRAY(val) || XVR_IS_DICTIONARY(val)) {
        Xvr_parseCompoundToPureValues(interpreter, &val);
    }

    if (XVR_AS_TYPE(type).typeOf == XVR_LITERAL_FLOAT && XVR_IS_INTEGER(val)) {
        val = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(val));
    }

    if (!XVR_IS_NULL(val) &&
        !Xvr_setScopeVariable(interpreter->scope, identifier, val, false)) {
        interpreter->errorOutput("Incorrect type assigned to variable \"");
        Xvr_printLiteralCustom(identifier, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");

        Xvr_freeLiteral(type);
        Xvr_freeLiteral(val);

        return false;
    }

    Xvr_freeLiteral(val);
    Xvr_freeLiteral(type);

    return true;
}

static bool execFnDecl(Xvr_Interpreter* interpreter, bool lng) {
    // read the index in the cache
    int identifierIndex = 0;
    int functionIndex = 0;

    if (lng) {
        identifierIndex =
            (int)readShort(interpreter->bytecode, &interpreter->count);
        functionIndex =
            (int)readShort(interpreter->bytecode, &interpreter->count);
    } else {
        identifierIndex =
            (int)readByte(interpreter->bytecode, &interpreter->count);
        functionIndex =
            (int)readByte(interpreter->bytecode, &interpreter->count);
    }

    Xvr_Literal identifier =
        interpreter->literalCache.literals[identifierIndex];
    Xvr_Literal function = interpreter->literalCache.literals[functionIndex];

    XVR_AS_FUNCTION(function).scope = Xvr_pushScope(
        interpreter->scope);  // hacked in (needed for closure persistance)

    Xvr_Literal type = XVR_TO_TYPE_LITERAL(XVR_LITERAL_FUNCTION, true);

    if (!Xvr_declareScopeVariable(interpreter->scope, identifier, type)) {
        interpreter->errorOutput("Can't redefine the function \"");
        Xvr_printLiteralCustom(identifier, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");
        return false;
    }

    if (!Xvr_setScopeVariable(interpreter->scope, identifier, function,
                              false)) {  // scope gets copied here
        interpreter->errorOutput("Incorrect type assigned to variable \"");
        Xvr_printLiteralCustom(identifier, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");
        return false;
    }

    Xvr_popScope(XVR_AS_FUNCTION(function).scope);  // hacked out
    XVR_AS_FUNCTION(function).scope = NULL;

    Xvr_freeLiteral(type);

    return true;
}

static bool execVarAssign(Xvr_Interpreter* interpreter) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal lhs = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal rhsIdn = rhs;
    if (XVR_IS_IDENTIFIER(rhs) &&
        Xvr_parseIdentifierToValue(interpreter, &rhs)) {
        Xvr_freeLiteral(rhsIdn);
    }

    if (XVR_IS_ARRAY(rhs) || XVR_IS_DICTIONARY(rhs)) {
        Xvr_parseCompoundToPureValues(interpreter, &rhs);
    }

    if (!XVR_IS_IDENTIFIER(lhs)) {
        interpreter->errorOutput("Can't assign to a non-variable \"");
        Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");
        return false;
    }

    if (!Xvr_isDeclaredScopeVariable(interpreter->scope, lhs)) {
        interpreter->errorOutput("Undeclared variable \"");
        Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);
        return false;
    }

    Xvr_Literal type = Xvr_getScopeType(interpreter->scope, lhs);
    if (XVR_AS_TYPE(type).typeOf == XVR_LITERAL_FLOAT && XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    if (!Xvr_setScopeVariable(interpreter->scope, lhs, rhs, true)) {
        interpreter->errorOutput("Incorrect type assigned to variable \"");
        Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);
        Xvr_freeLiteral(type);
        return false;
    }

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);
    Xvr_freeLiteral(type);

    return true;
}

static bool execVarArithmeticAssign(Xvr_Interpreter* interpreter) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal lhs = Xvr_popLiteralArray(&interpreter->stack);

    // duplicate the name
    Xvr_pushLiteralArray(&interpreter->stack, lhs);
    Xvr_pushLiteralArray(&interpreter->stack, lhs);
    Xvr_pushLiteralArray(&interpreter->stack, rhs);

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return true;
}

static bool execValCast(Xvr_Interpreter* interpreter) {
    Xvr_Literal value = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal type = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal valueIdn = value;
    if (XVR_IS_IDENTIFIER(value) &&
        Xvr_parseIdentifierToValue(interpreter, &value)) {
        Xvr_freeLiteral(valueIdn);
    }

    Xvr_Literal result = XVR_TO_NULL_LITERAL;

    if (XVR_IS_NULL(value)) {
        interpreter->errorOutput("Can't cast a null value\n");

        Xvr_freeLiteral(value);
        Xvr_freeLiteral(type);

        return false;
    }

    // cast the rhs to the type represented by lhs
    switch (XVR_AS_TYPE(type).typeOf) {
    case XVR_LITERAL_BOOLEAN:
        result = XVR_TO_BOOLEAN_LITERAL(XVR_IS_TRUTHY(value));
        break;

    case XVR_LITERAL_INTEGER:
        if (XVR_IS_BOOLEAN(value)) {
            result = XVR_TO_INTEGER_LITERAL(XVR_AS_BOOLEAN(value) ? 1 : 0);
        }

        if (XVR_IS_INTEGER(value)) {
            result = Xvr_copyLiteral(value);
        }

        if (XVR_IS_FLOAT(value)) {
            result = XVR_TO_INTEGER_LITERAL(XVR_AS_FLOAT(value));
        }

        if (XVR_IS_STRING(value)) {
            int val = 0;
            sscanf(Xvr_toCString(XVR_AS_STRING(value)), "%d", &val);
            result = XVR_TO_INTEGER_LITERAL(val);
        }
        break;

    case XVR_LITERAL_FLOAT:
        if (XVR_IS_BOOLEAN(value)) {
            result = XVR_TO_FLOAT_LITERAL(XVR_AS_BOOLEAN(value) ? 1 : 0);
        }

        if (XVR_IS_INTEGER(value)) {
            result = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(value));
        }

        if (XVR_IS_FLOAT(value)) {
            result = Xvr_copyLiteral(value);
        }

        if (XVR_IS_STRING(value)) {
            float val = 0;
            sscanf(Xvr_toCString(XVR_AS_STRING(value)), "%f", &val);
            result = XVR_TO_FLOAT_LITERAL(val);
        }
        break;

    case XVR_LITERAL_STRING:
        if (XVR_IS_BOOLEAN(value)) {
            char* str = XVR_AS_BOOLEAN(value) ? "true" : "false";

            int length = strlen(str);
            result = XVR_TO_STRING_LITERAL(Xvr_createRefStringLength(
                str, length));  // TODO: static reference optimisation?
        }

        if (XVR_IS_INTEGER(value)) {
            char buffer[128];
            snprintf(buffer, 128, "%d", XVR_AS_INTEGER(value));
            int length = strlen(buffer);
            result = XVR_TO_STRING_LITERAL(
                Xvr_createRefStringLength(buffer, length));
        }

        if (XVR_IS_FLOAT(value)) {
            char buffer[128];
            snprintf(buffer, 128, "%g", XVR_AS_FLOAT(value));
            int length = strlen(buffer);
            result = XVR_TO_STRING_LITERAL(
                Xvr_createRefStringLength(buffer, length));
        }

        if (XVR_IS_STRING(value)) {
            result = Xvr_copyLiteral(value);
        }
        break;

    default:
        interpreter->errorOutput("Unknown cast type found: ");
        Xvr_printLiteralCustom(type, interpreter->errorOutput);
        interpreter->errorOutput("\n");
        return false;
    }

    // leave the new value on the stack
    Xvr_pushLiteralArray(&interpreter->stack, result);

    Xvr_freeLiteral(result);
    Xvr_freeLiteral(value);
    Xvr_freeLiteral(type);

    return true;
}

static bool execTypeOf(Xvr_Interpreter* interpreter) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal type = XVR_TO_NULL_LITERAL;

    if (XVR_IS_IDENTIFIER(rhs)) {
        type = Xvr_getScopeType(interpreter->scope, rhs);
    } else {
        type = XVR_TO_TYPE_LITERAL(rhs.type, false);  // see issue #53
    }

    Xvr_pushLiteralArray(&interpreter->stack, type);

    Xvr_freeLiteral(rhs);
    Xvr_freeLiteral(type);

    return true;
}

static bool execCompareEqual(Xvr_Interpreter* interpreter, bool invert) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal lhs = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal rhsIdn = rhs;
    if (XVR_IS_IDENTIFIER(rhs) &&
        Xvr_parseIdentifierToValue(interpreter, &rhs)) {
        Xvr_freeLiteral(rhsIdn);
    }

    Xvr_Literal lhsIdn = lhs;
    if (XVR_IS_IDENTIFIER(lhs) &&
        Xvr_parseIdentifierToValue(interpreter, &lhs)) {
        Xvr_freeLiteral(lhsIdn);
    }

    bool result = Xvr_literalsAreEqual(lhs, rhs);

    if (invert) {
        result = !result;
    }

    Xvr_pushLiteralArray(&interpreter->stack, XVR_TO_BOOLEAN_LITERAL(result));

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return true;
}

static bool execCompareLess(Xvr_Interpreter* interpreter, bool invert) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal lhs = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal rhsIdn = rhs;
    if (XVR_IS_IDENTIFIER(rhs) &&
        Xvr_parseIdentifierToValue(interpreter, &rhs)) {
        Xvr_freeLiteral(rhsIdn);
    }

    Xvr_Literal lhsIdn = lhs;
    if (XVR_IS_IDENTIFIER(lhs) &&
        Xvr_parseIdentifierToValue(interpreter, &lhs)) {
        Xvr_freeLiteral(lhsIdn);
    }

    // not a number, return falure
    if (!(XVR_IS_INTEGER(lhs) || XVR_IS_FLOAT(lhs))) {
        interpreter->errorOutput("Incorrect type in comparison, value \"");
        Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);
        return false;
    }

    if (!(XVR_IS_INTEGER(rhs) || XVR_IS_FLOAT(rhs))) {
        interpreter->errorOutput("Incorrect type in comparison, value \"");
        Xvr_printLiteralCustom(rhs, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");
        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);
        return false;
    }

    // convert to floats - easier
    if (XVR_IS_INTEGER(lhs)) {
        lhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(lhs));
    }

    if (XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    bool result;

    if (!invert) {
        result = (XVR_AS_FLOAT(lhs) < XVR_AS_FLOAT(rhs));
    } else {
        result = (XVR_AS_FLOAT(lhs) > XVR_AS_FLOAT(rhs));
    }

    Xvr_pushLiteralArray(&interpreter->stack, XVR_TO_BOOLEAN_LITERAL(result));

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return true;
}

static bool execCompareLessEqual(Xvr_Interpreter* interpreter, bool invert) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal lhs = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal rhsIdn = rhs;
    if (XVR_IS_IDENTIFIER(rhs) &&
        Xvr_parseIdentifierToValue(interpreter, &rhs)) {
        Xvr_freeLiteral(rhsIdn);
    }

    Xvr_Literal lhsIdn = lhs;
    if (XVR_IS_IDENTIFIER(lhs) &&
        Xvr_parseIdentifierToValue(interpreter, &lhs)) {
        Xvr_freeLiteral(lhsIdn);
    }

    // not a number, return falure
    if (!(XVR_IS_INTEGER(lhs) || XVR_IS_FLOAT(lhs))) {
        interpreter->errorOutput("Incorrect type in comparison, value \"");
        Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);
        return false;
    }

    if (!(XVR_IS_INTEGER(rhs) || XVR_IS_FLOAT(rhs))) {
        interpreter->errorOutput("Incorrect type in comparison, value \"");
        Xvr_printLiteralCustom(rhs, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);
        return false;
    }

    // convert to floats - easier
    if (XVR_IS_INTEGER(lhs)) {
        lhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(lhs));
    }

    if (XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    bool result;

    if (!invert) {
        result = (XVR_AS_FLOAT(lhs) < XVR_AS_FLOAT(rhs)) ||
                 Xvr_literalsAreEqual(lhs, rhs);
    } else {
        result = (XVR_AS_FLOAT(lhs) > XVR_AS_FLOAT(rhs)) ||
                 Xvr_literalsAreEqual(lhs, rhs);
    }

    Xvr_pushLiteralArray(&interpreter->stack, XVR_TO_BOOLEAN_LITERAL(result));

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return true;
}

static bool execAnd(Xvr_Interpreter* interpreter) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal lhs = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal rhsIdn = rhs;
    if (XVR_IS_IDENTIFIER(rhs) &&
        Xvr_parseIdentifierToValue(interpreter, &rhs)) {
        Xvr_freeLiteral(rhsIdn);
    }

    Xvr_Literal lhsIdn = lhs;
    if (XVR_IS_IDENTIFIER(lhs) &&
        Xvr_parseIdentifierToValue(interpreter, &lhs)) {
        Xvr_freeLiteral(lhsIdn);
    }

    if (XVR_IS_TRUTHY(lhs) && XVR_IS_TRUTHY(rhs)) {
        Xvr_pushLiteralArray(&interpreter->stack, XVR_TO_BOOLEAN_LITERAL(true));
    } else {
        Xvr_pushLiteralArray(&interpreter->stack,
                             XVR_TO_BOOLEAN_LITERAL(false));
    }

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return true;
}

static bool execOr(Xvr_Interpreter* interpreter) {
    Xvr_Literal rhs = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal lhs = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal rhsIdn = rhs;
    if (XVR_IS_IDENTIFIER(rhs) &&
        Xvr_parseIdentifierToValue(interpreter, &rhs)) {
        Xvr_freeLiteral(rhsIdn);
    }

    Xvr_Literal lhsIdn = lhs;
    if (XVR_IS_IDENTIFIER(lhs) &&
        Xvr_parseIdentifierToValue(interpreter, &lhs)) {
        Xvr_freeLiteral(lhsIdn);
    }

    if (XVR_IS_TRUTHY(lhs) || XVR_IS_TRUTHY(rhs)) {
        Xvr_pushLiteralArray(&interpreter->stack, XVR_TO_BOOLEAN_LITERAL(true));
    } else {
        Xvr_pushLiteralArray(&interpreter->stack,
                             XVR_TO_BOOLEAN_LITERAL(false));
    }

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return true;
}

static bool execJump(Xvr_Interpreter* interpreter) {
    int target = (int)readShort(interpreter->bytecode, &interpreter->count);

    if (target + interpreter->codeStart > interpreter->length) {
        interpreter->errorOutput("[internal] Jump out of range\n");
        return false;
    }

    // actually jump
    interpreter->count = target + interpreter->codeStart;

    return true;
}

static bool execFalseJump(Xvr_Interpreter* interpreter) {
    int target = (int)readShort(interpreter->bytecode, &interpreter->count);

    if (target + interpreter->codeStart > interpreter->length) {
        interpreter->errorOutput("[internal] Jump out of range (false jump)\n");
        return false;
    }

    // actually jump
    Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal litIdn = lit;
    if (XVR_IS_IDENTIFIER(lit) &&
        Xvr_parseIdentifierToValue(interpreter, &lit)) {
        Xvr_freeLiteral(litIdn);
    }

    if (XVR_IS_NULL(lit)) {
        interpreter->errorOutput("Null detected in comparison\n");
        Xvr_freeLiteral(lit);
        return false;
    }

    if (!XVR_IS_TRUTHY(lit)) {
        interpreter->count = target + interpreter->codeStart;
    }

    Xvr_freeLiteral(lit);

    return true;
}

// forward declare
static void execInterpreter(Xvr_Interpreter*);
static void readInterpreterSections(Xvr_Interpreter* interpreter);

static bool execFnCall(Xvr_Interpreter* interpreter, bool looseFirstArgument) {
    if (interpreter->depth >= 200) {
        interpreter->errorOutput("Infinite recursion detected - panicking\n");
        interpreter->panic = true;
        return false;
    }

    Xvr_LiteralArray arguments;
    Xvr_initLiteralArray(&arguments);

    Xvr_Literal stackSize = Xvr_popLiteralArray(&interpreter->stack);

    // unpack the stack of arguments
    for (int i = 0; i < XVR_AS_INTEGER(stackSize) - 1; i++) {
        Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);
        Xvr_pushLiteralArray(&arguments, lit);  // NOTE: also reverses the order
        Xvr_freeLiteral(lit);
    }

    // collect one more argument
    if (!looseFirstArgument && XVR_AS_INTEGER(stackSize) > 0) {
        Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);
        Xvr_pushLiteralArray(&arguments, lit);  // NOTE: also reverses the order
        Xvr_freeLiteral(lit);
    }

    Xvr_Literal identifier = Xvr_popLiteralArray(&interpreter->stack);

    // collect one more argument
    if (looseFirstArgument) {
        Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);
        Xvr_pushLiteralArray(&arguments, lit);  // NOTE: also reverses the order
        Xvr_freeLiteral(lit);
    }

    // let's screw with the fn name, too
    if (looseFirstArgument) {
        if (!XVR_IS_IDENTIFIER(identifier)) {
            interpreter->errorOutput(
                "bad literal passing as procedure identifier\n");
            Xvr_freeLiteral(identifier);
            Xvr_freeLiteral(stackSize);
            Xvr_freeLiteralArray(&arguments);
            return false;
        }

        int length = XVR_AS_IDENTIFIER(identifier)->length + 1;
        char buffer[XVR_MAX_STRING_LENGTH];
        snprintf(buffer, XVR_MAX_STRING_LENGTH, "_%s",
                 Xvr_toCString(
                     XVR_AS_IDENTIFIER(identifier)));  // prepend an underscore

        Xvr_freeLiteral(identifier);
        identifier = XVR_TO_IDENTIFIER_LITERAL(
            Xvr_createRefStringLength(buffer, length));
    }

    Xvr_Literal func = identifier;

    if (!Xvr_parseIdentifierToValue(interpreter, &func)) {
        Xvr_freeLiteralArray(&arguments);
        Xvr_freeLiteral(stackSize);
        Xvr_freeLiteral(identifier);
        return false;
    }

    // check for side-loaded native functions
    if (XVR_IS_FUNCTION_NATIVE(func)) {
        // reverse the order to the correct order
        Xvr_LiteralArray correct;
        Xvr_initLiteralArray(&correct);

        while (arguments.count) {
            Xvr_Literal lit = Xvr_popLiteralArray(&arguments);
            Xvr_pushLiteralArray(&correct, lit);
            Xvr_freeLiteral(lit);
        }

        Xvr_freeLiteralArray(&arguments);

        // call the native function
        XVR_AS_FUNCTION_NATIVE(func)(interpreter, &correct);

        Xvr_freeLiteralArray(&correct);
        Xvr_freeLiteral(stackSize);
        Xvr_freeLiteral(identifier);
        return true;
    }

    if (!XVR_IS_FUNCTION(func)) {
        interpreter->errorOutput("Function not found: ");
        Xvr_printLiteralCustom(identifier, interpreter->errorOutput);
        interpreter->errorOutput("\n");

        Xvr_freeLiteral(identifier);
        Xvr_freeLiteral(stackSize);
        Xvr_freeLiteralArray(&arguments);
        return false;
    }

    bool ret =
        Xvr_callLiteralFn(interpreter, func, &arguments, &interpreter->stack);

    if (!ret) {
        interpreter->errorOutput("Error encountered in function \"");
        Xvr_printLiteralCustom(identifier, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");
    }

    Xvr_freeLiteralArray(&arguments);
    Xvr_freeLiteral(func);
    Xvr_freeLiteral(stackSize);
    Xvr_freeLiteral(identifier);

    return ret;
}

bool Xvr_callLiteralFn(Xvr_Interpreter* interpreter, Xvr_Literal func,
                       Xvr_LiteralArray* arguments, Xvr_LiteralArray* returns) {
    // check for side-loaded native functions
    if (XVR_IS_FUNCTION_NATIVE(func)) {
        // reverse the order to the correct order
        Xvr_LiteralArray correct;
        Xvr_initLiteralArray(&correct);

        while (arguments->count) {
            Xvr_Literal lit = Xvr_popLiteralArray(arguments);
            Xvr_pushLiteralArray(&correct, lit);
            Xvr_freeLiteral(lit);
        }

        // call the native function
        int returnsCount = XVR_AS_FUNCTION_NATIVE(func)(interpreter, &correct);

        if (returnsCount < 0) {
            interpreter->errorOutput("Unknown error from native function\n");
            Xvr_freeLiteralArray(&correct);
            return false;
        }

        // get the results
        Xvr_LiteralArray returnsFromInner;
        Xvr_initLiteralArray(&returnsFromInner);

        for (int i = 0; i < (returnsCount || 1); i++) {
            Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);
            Xvr_pushLiteralArray(&returnsFromInner,
                                 lit);  // NOTE: also reverses the order
            Xvr_freeLiteral(lit);
        }

        // flip them around and pass to returns
        while (returnsFromInner.count > 0) {
            Xvr_Literal lit = Xvr_popLiteralArray(&returnsFromInner);
            Xvr_pushLiteralArray(returns, lit);
            Xvr_freeLiteral(lit);
        }

        Xvr_freeLiteralArray(&returnsFromInner);
        Xvr_freeLiteralArray(&correct);
        return true;
    }

    // normal Xvr function
    if (!XVR_IS_FUNCTION(func)) {
        interpreter->errorOutput("Function required in Xvr_callLiteralFn()\n");
        return false;
    }

    // set up a new interpreter
    Xvr_Interpreter inner;

    // init the inner interpreter manually
    Xvr_initLiteralArray(&inner.literalCache);
    inner.scope = Xvr_pushScope(func.as.function.scope);
    inner.bytecode = XVR_AS_FUNCTION(func).inner.bytecode;
    inner.length = XVR_AS_FUNCTION_BYTECODE_LENGTH(func);
    inner.count = 0;
    inner.codeStart = -1;
    inner.depth = interpreter->depth + 1;
    inner.panic = false;
    Xvr_initLiteralArray(&inner.stack);
    inner.hooks = interpreter->hooks;
    Xvr_setInterpreterPrint(&inner, interpreter->printOutput);
    Xvr_setInterpreterAssert(&inner, interpreter->assertOutput);
    Xvr_setInterpreterError(&inner, interpreter->errorOutput);

    // prep the sections
    readInterpreterSections(&inner);

    // prep the arguments
    Xvr_LiteralArray* paramArray = XVR_AS_ARRAY(
        inner.literalCache.literals[readShort(inner.bytecode, &inner.count)]);
    Xvr_LiteralArray* returnArray = XVR_AS_ARRAY(
        inner.literalCache.literals[readShort(inner.bytecode, &inner.count)]);

    // get the rest param, if it exists
    Xvr_Literal restParam = XVR_TO_NULL_LITERAL;
    if (paramArray->count >= 2 &&
        XVR_AS_TYPE(paramArray->literals[paramArray->count - 1]).typeOf ==
            XVR_LITERAL_FUNCTION_ARG_REST) {
        restParam = paramArray->literals[paramArray->count - 2];
    }

    // check the param total is correct
    if ((XVR_IS_NULL(restParam) && paramArray->count != arguments->count * 2) ||
        (!XVR_IS_NULL(restParam) &&
         paramArray->count - 2 > arguments->count * 2)) {
        interpreter->errorOutput(
            "Incorrect number of arguments passed to a function\n");

        // free, and skip out
        Xvr_popScope(inner.scope);

        Xvr_freeLiteralArray(&inner.stack);
        Xvr_freeLiteralArray(&inner.literalCache);

        return false;
    }

    // contents is the indexes of identifier & type
    for (int i = 0; i < paramArray->count - (XVR_IS_NULL(restParam) ? 0 : 2);
         i += 2) {  // don't count the rest parameter, if present
        // declare and define each entry in the scope
        if (!Xvr_declareScopeVariable(inner.scope, paramArray->literals[i],
                                      paramArray->literals[i + 1])) {
            interpreter->errorOutput(
                "[internal] Could not re-declare parameter\n");

            // free, and skip out
            Xvr_popScope(inner.scope);

            Xvr_freeLiteralArray(&inner.stack);
            Xvr_freeLiteralArray(&inner.literalCache);

            return false;
        }

        Xvr_Literal arg = Xvr_popLiteralArray(arguments);

        Xvr_Literal argIdn = arg;
        if (XVR_IS_IDENTIFIER(arg) &&
            Xvr_parseIdentifierToValue(interpreter, &arg)) {
            Xvr_freeLiteral(argIdn);
        }

        if (!Xvr_setScopeVariable(inner.scope, paramArray->literals[i], arg,
                                  false)) {
            interpreter->errorOutput(
                "[internal] Could not define parameter (bad type?)\n");

            // free, and skip out
            Xvr_freeLiteral(arg);
            Xvr_popScope(inner.scope);

            Xvr_freeLiteralArray(&inner.stack);
            Xvr_freeLiteralArray(&inner.literalCache);

            return false;
        }
        Xvr_freeLiteral(arg);
    }

    // if using rest, pack the optional extra arguments into the rest parameter
    // (array)
    if (!XVR_IS_NULL(restParam)) {
        Xvr_LiteralArray rest;
        Xvr_initLiteralArray(&rest);

        while (arguments->count > 0) {
            Xvr_Literal lit = Xvr_popLiteralArray(arguments);
            Xvr_pushLiteralArray(&rest, lit);
            Xvr_freeLiteral(lit);
        }

        Xvr_Literal restType = XVR_TO_TYPE_LITERAL(XVR_LITERAL_ARRAY, true);
        Xvr_Literal any = XVR_TO_TYPE_LITERAL(XVR_LITERAL_ANY, false);
        XVR_TYPE_PUSH_SUBTYPE(&restType, any);

        // declare & define the rest parameter
        if (!Xvr_declareScopeVariable(inner.scope, restParam, restType)) {
            interpreter->errorOutput(
                "[internal] Could not declare rest parameter\n");

            // free, and skip out
            Xvr_freeLiteral(restType);
            Xvr_freeLiteralArray(&rest);
            Xvr_popScope(inner.scope);

            Xvr_freeLiteralArray(&inner.stack);
            Xvr_freeLiteralArray(&inner.literalCache);

            return false;
        }

        Xvr_Literal lit = XVR_TO_ARRAY_LITERAL(&rest);
        if (!Xvr_setScopeVariable(inner.scope, restParam, lit, false)) {
            interpreter->errorOutput(
                "[internal] Could not define rest parameter\n");

            // free, and skip out
            Xvr_freeLiteral(restType);
            Xvr_freeLiteral(lit);
            Xvr_popScope(inner.scope);

            Xvr_freeLiteralArray(&inner.stack);
            Xvr_freeLiteralArray(&inner.literalCache);

            return false;
        }

        Xvr_freeLiteral(restType);
        Xvr_freeLiteralArray(&rest);
    }

    // execute the interpreter
    execInterpreter(&inner);

    // adopt the panic state
    interpreter->panic = inner.panic;

    // accept the stack as the results
    Xvr_LiteralArray returnsFromInner;
    Xvr_initLiteralArray(&returnsFromInner);

    // unpack the results
    for (int i = 0; i < (returnArray->count || 1); i++) {
        Xvr_Literal lit = Xvr_popLiteralArray(&inner.stack);
        Xvr_pushLiteralArray(&returnsFromInner,
                             lit);  // NOTE: also reverses the order
        Xvr_freeLiteral(lit);
    }

    bool returnValue = true;

    if (returnsFromInner.count > 1) {
        interpreter->errorOutput(
            "Too many values returned (multiple returns not yet supported)\n");

        returnValue = false;
    }

    for (int i = 0; i < returnsFromInner.count && returnValue; i++) {
        Xvr_Literal ret = Xvr_popLiteralArray(&returnsFromInner);

        // check the return types
        if (returnArray->count > 0 &&
            XVR_AS_TYPE(returnArray->literals[i]).typeOf != ret.type) {
            interpreter->errorOutput("Bad type found in return value\n");

            // free, and skip out
            returnValue = false;
            break;
        }

        Xvr_pushLiteralArray(returns, ret);  // NOTE: reverses again
        Xvr_freeLiteral(ret);
    }

    // manual free
    // NOTE: handle scopes of functions, which refer to the parent scope
    // (leaking memory)
    while (inner.scope != XVR_AS_FUNCTION(func).scope) {
        for (int i = 0; i < inner.scope->variables.capacity; i++) {
            // handle keys, just in case
            if (XVR_IS_FUNCTION(inner.scope->variables.entries[i].key)) {
                Xvr_popScope(
                    XVR_AS_FUNCTION(inner.scope->variables.entries[i].key)
                        .scope);
                XVR_AS_FUNCTION(inner.scope->variables.entries[i].key).scope =
                    NULL;
            }

            if (XVR_IS_FUNCTION(inner.scope->variables.entries[i].value)) {
                Xvr_popScope(
                    XVR_AS_FUNCTION(inner.scope->variables.entries[i].value)
                        .scope);
                XVR_AS_FUNCTION(inner.scope->variables.entries[i].value).scope =
                    NULL;
            }
        }

        inner.scope = Xvr_popScope(inner.scope);
    }
    Xvr_freeLiteralArray(&returnsFromInner);
    Xvr_freeLiteralArray(&inner.stack);
    Xvr_freeLiteralArray(&inner.literalCache);

    // actual bytecode persists until next call
    return true;
}

bool Xvr_callFn(Xvr_Interpreter* interpreter, const char* name,
                Xvr_LiteralArray* arguments, Xvr_LiteralArray* returns) {
    Xvr_Literal key = XVR_TO_IDENTIFIER_LITERAL(
        Xvr_createRefStringLength(name, strlen(name)));
    Xvr_Literal val = XVR_TO_NULL_LITERAL;

    if (!Xvr_isDeclaredScopeVariable(interpreter->scope, key)) {
        interpreter->errorOutput("No function with that name\n");
        return false;
    }

    Xvr_getScopeVariable(interpreter->scope, key, &val);

    bool ret = Xvr_callLiteralFn(interpreter, val, arguments, returns);

    Xvr_freeLiteral(key);
    Xvr_freeLiteral(val);

    return ret;
}

static bool execFnReturn(Xvr_Interpreter* interpreter) {
    Xvr_LiteralArray returns;
    Xvr_initLiteralArray(&returns);

    // get the values of everything on the stack
    while (interpreter->stack.count > 0) {
        Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);

        Xvr_Literal litIdn = lit;
        if (XVR_IS_IDENTIFIER(lit) &&
            Xvr_parseIdentifierToValue(interpreter, &lit)) {
            Xvr_freeLiteral(litIdn);
        }

        if (XVR_IS_ARRAY(lit) || XVR_IS_DICTIONARY(lit)) {
            Xvr_parseCompoundToPureValues(interpreter, &lit);
        }

        Xvr_pushLiteralArray(&returns, lit);  // reverses the order
        Xvr_freeLiteral(lit);
    }

    // and back again
    while (returns.count > 0) {
        Xvr_Literal lit = Xvr_popLiteralArray(&returns);
        Xvr_pushLiteralArray(&interpreter->stack, lit);
        Xvr_freeLiteral(lit);
    }

    Xvr_freeLiteralArray(&returns);

    // finally
    return false;
}

static bool execImport(Xvr_Interpreter* interpreter) {
    Xvr_Literal alias = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal identifier = Xvr_popLiteralArray(&interpreter->stack);

    // access the hooks
    if (!Xvr_existsLiteralDictionary(interpreter->hooks, identifier)) {
        interpreter->errorOutput("Unknown library name in import statement: ");
        Xvr_printLiteralCustom(identifier, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");
        interpreter->errorOutput("\n");

        Xvr_freeLiteral(alias);
        Xvr_freeLiteral(identifier);
        return false;
    }

    Xvr_Literal func = Xvr_getLiteralDictionary(interpreter->hooks, identifier);

    if (!XVR_IS_FUNCTION_HOOK(func)) {
        interpreter->errorOutput("Expected hook function, found: ");
        Xvr_printLiteralCustom(identifier, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");

        Xvr_freeLiteral(func);
        Xvr_freeLiteral(alias);
        Xvr_freeLiteral(identifier);
        return false;
    }

    XVR_AS_FUNCTION_HOOK(func)(interpreter, identifier, alias);

    Xvr_freeLiteral(func);
    Xvr_freeLiteral(alias);
    Xvr_freeLiteral(identifier);
    return true;
}

static bool execIndex(Xvr_Interpreter* interpreter, bool assignIntermediate) {
    // assume -> compound, first, second, third are all on the stack

    Xvr_Literal third = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal second = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal first = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal compound = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal compoundIdn = compound;
    bool freeIdn = false;
    if (XVR_IS_IDENTIFIER(compound) &&
        Xvr_parseIdentifierToValue(interpreter, &compound)) {
        freeIdn = true;
    }

    if (!XVR_IS_ARRAY(compound) && !XVR_IS_DICTIONARY(compound) &&
        !XVR_IS_STRING(compound)) {
        interpreter->errorOutput(
            "Unknown compound found in indexing notation: ");
        Xvr_printLiteralCustom(compound, interpreter->errorOutput);
        Xvr_freeLiteral(third);
        Xvr_freeLiteral(second);
        Xvr_freeLiteral(first);
        Xvr_freeLiteral(compound);

        if (freeIdn) {
            Xvr_freeLiteral(compoundIdn);
        }

        return false;
    }

    // build the argument list
    Xvr_LiteralArray arguments;
    Xvr_initLiteralArray(&arguments);

    Xvr_pushLiteralArray(&arguments, compound);
    Xvr_pushLiteralArray(&arguments, first);
    Xvr_pushLiteralArray(&arguments, second);
    Xvr_pushLiteralArray(&arguments, third);
    Xvr_pushLiteralArray(
        &arguments, XVR_TO_NULL_LITERAL);  // it expects an assignment command
    Xvr_pushLiteralArray(
        &arguments, XVR_TO_NULL_LITERAL);  // it expects an assignment "opcode"

    // leave the idn and compound on the stack
    if (assignIntermediate) {
        if (XVR_IS_IDENTIFIER(compoundIdn)) {
            Xvr_pushLiteralArray(&interpreter->stack, compoundIdn);
        }
        Xvr_pushLiteralArray(&interpreter->stack, compound);
        Xvr_pushLiteralArray(&interpreter->stack, first);
        Xvr_pushLiteralArray(&interpreter->stack, second);
        Xvr_pushLiteralArray(&interpreter->stack, third);
    }

    // call the _index function
    if (Xvr_private_index(interpreter, &arguments) < 0) {
        interpreter->errorOutput(
            "Something went wrong while indexing (simple index): ");
        Xvr_printLiteralCustom(compoundIdn, interpreter->errorOutput);
        interpreter->errorOutput("\n");

        // clean up
        Xvr_freeLiteral(third);
        Xvr_freeLiteral(second);
        Xvr_freeLiteral(first);
        Xvr_freeLiteral(compound);
        if (freeIdn) {
            Xvr_freeLiteral(compoundIdn);
        }
        Xvr_freeLiteralArray(&arguments);
        return false;
    }

    // clean up
    Xvr_freeLiteral(third);
    Xvr_freeLiteral(second);
    Xvr_freeLiteral(first);
    Xvr_freeLiteral(compound);
    if (freeIdn) {
        Xvr_freeLiteral(compoundIdn);
    }
    Xvr_freeLiteralArray(&arguments);

    return true;
}

static bool execIndexAssign(Xvr_Interpreter* interpreter) {
    // assume -> compound, first, second, third, assign are all on the stack

    Xvr_Literal assign = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal third = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal second = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal first = Xvr_popLiteralArray(&interpreter->stack);
    Xvr_Literal compound = Xvr_popLiteralArray(&interpreter->stack);

    Xvr_Literal assignIdn = assign;
    if (XVR_IS_IDENTIFIER(assign) &&
        Xvr_parseIdentifierToValue(interpreter, &assign)) {
        Xvr_freeLiteral(assignIdn);
    }

    Xvr_Literal compoundIdn = compound;
    bool freeIdn = false;
    if (XVR_IS_IDENTIFIER(compound) &&
        Xvr_parseIdentifierToValue(interpreter, &compound)) {
        freeIdn = true;
    }

    if (!XVR_IS_ARRAY(compound) && !XVR_IS_DICTIONARY(compound) &&
        !XVR_IS_STRING(compound)) {
        interpreter->errorOutput(
            "Unknown compound found in index assigning notation\n");
        Xvr_freeLiteral(assign);
        Xvr_freeLiteral(third);
        Xvr_freeLiteral(second);
        Xvr_freeLiteral(first);
        Xvr_freeLiteral(compound);
        if (freeIdn) {
            Xvr_freeLiteral(compoundIdn);
        }
        return false;
    }

    // build the opcode
    unsigned char opcode = readByte(interpreter->bytecode, &interpreter->count);
    char* opStr = "";
    switch (opcode) {
    case XVR_OP_VAR_ASSIGN:
        opStr = "=";
        break;
    case XVR_OP_VAR_ADDITION_ASSIGN:
        opStr = "+=";
        break;
    case XVR_OP_VAR_SUBTRACTION_ASSIGN:
        opStr = "-=";
        break;
    case XVR_OP_VAR_MULTIPLICATION_ASSIGN:
        opStr = "*=";
        break;
    case XVR_OP_VAR_DIVISION_ASSIGN:
        opStr = "/=";
        break;
    case XVR_OP_VAR_MODULO_ASSIGN:
        opStr = "%=";
        break;

    default:
        interpreter->errorOutput("bad opcode in index assigning notation\n");
        Xvr_freeLiteral(assign);
        Xvr_freeLiteral(third);
        Xvr_freeLiteral(second);
        Xvr_freeLiteral(first);
        Xvr_freeLiteral(compound);
        if (freeIdn) {
            Xvr_freeLiteral(compoundIdn);
        }
        return false;
    }

    int opLength = strlen(opStr);
    Xvr_Literal op = XVR_TO_STRING_LITERAL(Xvr_createRefStringLength(
        opStr, opLength));  // TODO: static reference optimisation?

    // build the argument list
    Xvr_LiteralArray arguments;
    Xvr_initLiteralArray(&arguments);

    Xvr_pushLiteralArray(&arguments, compound);
    Xvr_pushLiteralArray(&arguments, first);
    Xvr_pushLiteralArray(&arguments, second);
    Xvr_pushLiteralArray(&arguments, third);
    Xvr_pushLiteralArray(&arguments,
                         assign);          // it expects an assignment command
    Xvr_pushLiteralArray(&arguments, op);  // it expects an assignment "opcode"

    // call the _index function
    if (Xvr_private_index(interpreter, &arguments) < 0) {
        // clean up
        Xvr_freeLiteral(assign);
        Xvr_freeLiteral(third);
        Xvr_freeLiteral(second);
        Xvr_freeLiteral(first);
        Xvr_freeLiteral(compound);
        if (freeIdn) {
            Xvr_freeLiteral(compoundIdn);
        }
        Xvr_freeLiteral(op);
        Xvr_freeLiteralArray(&arguments);

        return false;
    }

    // save the result (assume top of the interpreter stack is the new compound
    // value)
    Xvr_Literal result = Xvr_popLiteralArray(&interpreter->stack);

    // deep
    if (!freeIdn) {
        while (interpreter->stack.count > 1) {
            // read the new values
            Xvr_freeLiteral(compound);
            Xvr_freeLiteral(third);
            Xvr_freeLiteral(second);
            Xvr_freeLiteral(first);
            Xvr_freeLiteralArray(&arguments);
            Xvr_initLiteralArray(&arguments);
            Xvr_freeLiteral(op);

            // reuse these like an idiot
            third = Xvr_popLiteralArray(&interpreter->stack);
            second = Xvr_popLiteralArray(&interpreter->stack);
            first = Xvr_popLiteralArray(&interpreter->stack);
            compound = Xvr_popLiteralArray(&interpreter->stack);

            char* opStr = "=";  // shadow, but force assignment
            int opLength = strlen(opStr);
            op = XVR_TO_STRING_LITERAL(Xvr_createRefStringLength(
                opStr, opLength));  // TODO: static reference optimisation?

            // assign to the idn / compound - with _index
            Xvr_pushLiteralArray(&arguments, compound);  //
            Xvr_pushLiteralArray(&arguments, first);
            Xvr_pushLiteralArray(&arguments, second);
            Xvr_pushLiteralArray(&arguments, third);
            Xvr_pushLiteralArray(&arguments, result);
            Xvr_pushLiteralArray(&arguments, op);

            if (Xvr_private_index(interpreter, &arguments) < 0) {
                interpreter->errorOutput(
                    "Something went wrong while indexing (index assign): ");
                Xvr_printLiteralCustom(compound, interpreter->errorOutput);
                interpreter->errorOutput("\n");

                // clean up
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                if (freeIdn) {
                    Xvr_freeLiteral(compoundIdn);
                }
                Xvr_freeLiteral(op);
                Xvr_freeLiteralArray(&arguments);
                return false;
            }

            Xvr_freeLiteral(result);
            result = Xvr_popLiteralArray(&interpreter->stack);
        }

        Xvr_freeLiteral(compound);
        compound = Xvr_popLiteralArray(&interpreter->stack);
        compoundIdn = compound;
        freeIdn = false;
    }

    if (XVR_IS_IDENTIFIER(compoundIdn) &&
        !Xvr_setScopeVariable(interpreter->scope, compoundIdn, result, true)) {
        interpreter->errorOutput("Incorrect type assigned to compound member ");
        Xvr_printLiteralCustom(compoundIdn, interpreter->errorOutput);
        interpreter->errorOutput("\n");

        // clean up
        Xvr_freeLiteral(assign);
        Xvr_freeLiteral(third);
        Xvr_freeLiteral(second);
        Xvr_freeLiteral(first);
        Xvr_freeLiteral(compound);
        if (freeIdn) {
            Xvr_freeLiteral(compoundIdn);
        }
        Xvr_freeLiteral(op);
        Xvr_freeLiteralArray(&arguments);
        Xvr_freeLiteral(result);
        return false;
    }

    // clean up
    Xvr_freeLiteral(assign);
    Xvr_freeLiteral(third);
    Xvr_freeLiteral(second);
    Xvr_freeLiteral(first);
    Xvr_freeLiteral(compound);
    if (freeIdn) {
        Xvr_freeLiteral(compoundIdn);
    }
    Xvr_freeLiteral(op);
    Xvr_freeLiteralArray(&arguments);
    Xvr_freeLiteral(result);

    return true;
}

// the heart of toy
static void execInterpreter(Xvr_Interpreter* interpreter) {
    // set the starting point for the interpreter
    if (interpreter->codeStart == -1) {
        interpreter->codeStart = interpreter->count;
    }

    unsigned char opcode = readByte(interpreter->bytecode, &interpreter->count);

    while (opcode != XVR_OP_EOF && opcode != XVR_OP_SECTION_END &&
           !interpreter->panic) {
        switch (opcode) {
        case XVR_OP_PASS:
            break;

        case XVR_OP_ASSERT:
            if (!execAssert(interpreter)) {
                return;
            }
            break;

        case XVR_OP_PRINT:
            if (!execPrint(interpreter)) {
                return;
            }
            break;

        case XVR_OP_LITERAL:
        case XVR_OP_LITERAL_LONG:
            if (!execPushLiteral(interpreter, opcode == XVR_OP_LITERAL_LONG)) {
                return;
            }
            break;

        case XVR_OP_LITERAL_RAW:
            if (!rawLiteral(interpreter)) {
                return;
            }
            break;

        case XVR_OP_NEGATE:
            if (!execNegate(interpreter)) {
                return;
            }
            break;

        case XVR_OP_ADDITION:
        case XVR_OP_SUBTRACTION:
        case XVR_OP_MULTIPLICATION:
        case XVR_OP_DIVISION:
        case XVR_OP_MODULO:
            if (!execArithmetic(interpreter, opcode)) {
                return;
            }
            break;

        case XVR_OP_VAR_ADDITION_ASSIGN:
        case XVR_OP_VAR_SUBTRACTION_ASSIGN:
        case XVR_OP_VAR_MULTIPLICATION_ASSIGN:
        case XVR_OP_VAR_DIVISION_ASSIGN:
        case XVR_OP_VAR_MODULO_ASSIGN:
            execVarArithmeticAssign(interpreter);
            if (!execArithmetic(interpreter, opcode)) {
                Xvr_freeLiteral(Xvr_popLiteralArray(&interpreter->stack));
                return;
            }
            if (!execVarAssign(interpreter)) {
                return;
            }
            break;

        case XVR_OP_GROUPING_BEGIN:
            execInterpreter(interpreter);
            break;

        case XVR_OP_GROUPING_END:
            return;

        // scope
        case XVR_OP_SCOPE_BEGIN:
            interpreter->scope = Xvr_pushScope(interpreter->scope);
            break;

        case XVR_OP_SCOPE_END:
            interpreter->scope = Xvr_popScope(interpreter->scope);
            break;

            // TODO: custom type declarations?

        case XVR_OP_VAR_DECL:
        case XVR_OP_VAR_DECL_LONG:
            if (!execVarDecl(interpreter, opcode == XVR_OP_VAR_DECL_LONG)) {
                return;
            }
            break;

        case XVR_OP_FN_DECL:
        case XVR_OP_FN_DECL_LONG:
            if (!execFnDecl(interpreter, opcode == XVR_OP_FN_DECL_LONG)) {
                return;
            }
            break;

        case XVR_OP_VAR_ASSIGN:
            if (!execVarAssign(interpreter)) {
                return;
            }
            break;

        case XVR_OP_TYPE_CAST:
            if (!execValCast(interpreter)) {
                return;
            }
            break;

        case XVR_OP_TYPE_OF:
            if (!execTypeOf(interpreter)) {
                return;
            }
            break;

        case XVR_OP_COMPARE_EQUAL:
            if (!execCompareEqual(interpreter, false)) {
                return;
            }
            break;

        case XVR_OP_COMPARE_NOT_EQUAL:
            if (!execCompareEqual(interpreter, true)) {
                return;
            }
            break;

        case XVR_OP_COMPARE_LESS:
            if (!execCompareLess(interpreter, false)) {
                return;
            }
            break;

        case XVR_OP_COMPARE_LESS_EQUAL:
            if (!execCompareLessEqual(interpreter, false)) {
                return;
            }
            break;

        case XVR_OP_COMPARE_GREATER:
            if (!execCompareLess(interpreter, true)) {
                return;
            }
            break;

        case XVR_OP_COMPARE_GREATER_EQUAL:
            if (!execCompareLessEqual(interpreter, true)) {
                return;
            }
            break;

        case XVR_OP_INVERT:
            if (!execInvert(interpreter)) {
                return;
            }
            break;

        case XVR_OP_AND:
            if (!execAnd(interpreter)) {
                return;
            }
            break;

        case XVR_OP_OR:
            if (!execOr(interpreter)) {
                return;
            }
            break;

        case XVR_OP_JUMP:
            if (!execJump(interpreter)) {
                return;
            }
            break;

        case XVR_OP_IF_FALSE_JUMP:
            if (!execFalseJump(interpreter)) {
                return;
            }
            break;

        case XVR_OP_FN_CALL:
            if (!execFnCall(interpreter, false)) {
                return;
            }
            break;

        case XVR_OP_DOT:
            if (!execFnCall(
                    interpreter,
                    true)) {  // compensate for the out-of-order arguments
                return;
            }
            break;

        case XVR_OP_FN_RETURN:
            if (!execFnReturn(interpreter)) {
                return;
            }
            break;

        case XVR_OP_IMPORT:
            if (!execImport(interpreter)) {
                return;
            }
            break;

        case XVR_OP_INDEX:
            if (!execIndex(interpreter, false)) {
                return;
            }
            break;

        case XVR_OP_INDEX_ASSIGN_INTERMEDIATE:
            if (!execIndex(interpreter, true)) {
                return;
            }
            break;

        case XVR_OP_INDEX_ASSIGN:
            if (!execIndexAssign(interpreter)) {
                return;
            }
            break;

        case XVR_OP_POP_STACK:
            while (interpreter->stack.count > 0) {
                Xvr_freeLiteral(Xvr_popLiteralArray(&interpreter->stack));
            }
            break;

        default:
            interpreter->errorOutput("Unknown opcode found, terminating\n");
            return;
        }

        opcode = readByte(interpreter->bytecode, &interpreter->count);
    }
}

static void readInterpreterSections(Xvr_Interpreter* interpreter) {
    // data section
    const unsigned short literalCount =
        readShort(interpreter->bytecode, &interpreter->count);

#ifndef XVR_EXPORT
    if (Xvr_commandLine.verbose) {
        printf(XVR_CC_NOTICE "Reading %d literals\n" XVR_CC_RESET,
               literalCount);
    }
#endif

    for (int i = 0; i < literalCount; i++) {
        const unsigned char literalType =
            readByte(interpreter->bytecode, &interpreter->count);

        switch (literalType) {
        case XVR_LITERAL_NULL:
            // read the null
            Xvr_pushLiteralArray(&interpreter->literalCache,
                                 XVR_TO_NULL_LITERAL);

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(null)\n");
            }
#endif
            break;

        case XVR_LITERAL_BOOLEAN: {
            // read the booleans
            const bool b = readByte(interpreter->bytecode, &interpreter->count);
            Xvr_Literal literal = XVR_TO_BOOLEAN_LITERAL(b);
            Xvr_pushLiteralArray(&interpreter->literalCache, literal);
            Xvr_freeLiteral(literal);

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(boolean %s)\n", b ? "true" : "false");
            }
#endif
        } break;

        case XVR_LITERAL_INTEGER: {
            const int d = readInt(interpreter->bytecode, &interpreter->count);
            Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(d);
            Xvr_pushLiteralArray(&interpreter->literalCache, literal);
            Xvr_freeLiteral(literal);

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(integer %d)\n", d);
            }
#endif
        } break;

        case XVR_LITERAL_FLOAT: {
            const float f =
                readFloat(interpreter->bytecode, &interpreter->count);
            Xvr_Literal literal = XVR_TO_FLOAT_LITERAL(f);
            Xvr_pushLiteralArray(&interpreter->literalCache, literal);
            Xvr_freeLiteral(literal);

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(float %f)\n", f);
            }
#endif
        } break;

        case XVR_LITERAL_STRING: {
            const char* s =
                readString(interpreter->bytecode, &interpreter->count);
            int length = strlen(s);
            Xvr_Literal literal =
                XVR_TO_STRING_LITERAL(Xvr_createRefStringLength(s, length));
            Xvr_pushLiteralArray(&interpreter->literalCache, literal);
            Xvr_freeLiteral(literal);

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(string \"%s\")\n", s);
            }
#endif
        } break;

        case XVR_LITERAL_ARRAY_INTERMEDIATE:
        case XVR_LITERAL_ARRAY: {
            Xvr_LiteralArray* array = XVR_ALLOCATE(Xvr_LiteralArray, 1);
            Xvr_initLiteralArray(array);

            unsigned short length =
                readShort(interpreter->bytecode, &interpreter->count);

            // read each index, then unpack the value from the existing literal
            // cache
            for (int i = 0; i < length; i++) {
                int index =
                    readShort(interpreter->bytecode, &interpreter->count);
                Xvr_pushLiteralArray(array,
                                     interpreter->literalCache.literals[index]);
            }

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(array ");
                Xvr_Literal literal = XVR_TO_ARRAY_LITERAL(array);
                Xvr_printLiteral(literal);
                printf(")\n");
            }
#endif

            // finally, push the array proper
            Xvr_Literal literal = XVR_TO_ARRAY_LITERAL(array);
            Xvr_pushLiteralArray(&interpreter->literalCache,
                                 literal);  // copied

            Xvr_freeLiteralArray(array);
            XVR_FREE(Xvr_LiteralArray, array);
        } break;

        case XVR_LITERAL_DICTIONARY_INTERMEDIATE:
        case XVR_LITERAL_DICTIONARY: {
            Xvr_LiteralDictionary* dictionary =
                XVR_ALLOCATE(Xvr_LiteralDictionary, 1);
            Xvr_initLiteralDictionary(dictionary);

            unsigned short length =
                readShort(interpreter->bytecode, &interpreter->count);

            // read each index, then unpack the value from the existing literal
            // cache
            for (int i = 0; i < length / 2; i++) {
                int key = readShort(interpreter->bytecode, &interpreter->count);
                int val = readShort(interpreter->bytecode, &interpreter->count);
                Xvr_setLiteralDictionary(
                    dictionary, interpreter->literalCache.literals[key],
                    interpreter->literalCache.literals[val]);
            }

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(dictionary ");
                Xvr_Literal literal = XVR_TO_DICTIONARY_LITERAL(dictionary);
                Xvr_printLiteral(literal);
                printf(")\n");
            }
#endif

            // finally, push the dictionary proper
            Xvr_Literal literal = XVR_TO_DICTIONARY_LITERAL(dictionary);
            Xvr_pushLiteralArray(&interpreter->literalCache,
                                 literal);  // copied

            Xvr_freeLiteralDictionary(dictionary);
            XVR_FREE(Xvr_LiteralDictionary, dictionary);
        } break;

        case XVR_LITERAL_FUNCTION: {
            // read the index
            unsigned short index =
                readShort(interpreter->bytecode, &interpreter->count);
            Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(index);

            // change the type, to read it PROPERLY below
            literal.type = XVR_LITERAL_FUNCTION_INTERMEDIATE;

            // push to the literal cache
            Xvr_pushLiteralArray(&interpreter->literalCache, literal);

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(function)\n");
            }
#endif
        } break;

        case XVR_LITERAL_IDENTIFIER: {
            const char* str =
                readString(interpreter->bytecode, &interpreter->count);

            int length = strlen(str);
            Xvr_Literal identifier = XVR_TO_IDENTIFIER_LITERAL(
                Xvr_createRefStringLength(str, length));

            Xvr_pushLiteralArray(&interpreter->literalCache, identifier);

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(identifier %s (hash: %x))\n",
                       Xvr_toCString(XVR_AS_IDENTIFIER(identifier)),
                       identifier.as.identifier.hash);
            }
#endif

            Xvr_freeLiteral(identifier);
        } break;

        case XVR_LITERAL_TYPE: {
            // what the literal is
            Xvr_LiteralType literalType = (Xvr_LiteralType)readByte(
                interpreter->bytecode, &interpreter->count);
            unsigned char constant =
                readByte(interpreter->bytecode, &interpreter->count);

            Xvr_Literal typeLiteral =
                XVR_TO_TYPE_LITERAL(literalType, constant);

            // save the type
            Xvr_pushLiteralArray(&interpreter->literalCache, typeLiteral);

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(type ");
                Xvr_printLiteral(typeLiteral);
                printf(")\n");
            }
#endif
        } break;

        case XVR_LITERAL_TYPE_INTERMEDIATE: {
            // what the literal represents
            Xvr_LiteralType literalType = (Xvr_LiteralType)readByte(
                interpreter->bytecode, &interpreter->count);
            unsigned char constant =
                readByte(interpreter->bytecode, &interpreter->count);

            Xvr_Literal typeLiteral =
                XVR_TO_TYPE_LITERAL(literalType, constant);

            // if it's an array type
            if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_ARRAY) {
                unsigned short vt =
                    readShort(interpreter->bytecode, &interpreter->count);

                XVR_TYPE_PUSH_SUBTYPE(
                    &typeLiteral,
                    Xvr_copyLiteral(interpreter->literalCache.literals[vt]));
            }

            if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_DICTIONARY) {
                unsigned short kt =
                    readShort(interpreter->bytecode, &interpreter->count);
                unsigned short vt =
                    readShort(interpreter->bytecode, &interpreter->count);

                XVR_TYPE_PUSH_SUBTYPE(
                    &typeLiteral,
                    Xvr_copyLiteral(interpreter->literalCache.literals[kt]));
                XVR_TYPE_PUSH_SUBTYPE(
                    &typeLiteral,
                    Xvr_copyLiteral(interpreter->literalCache.literals[vt]));
            }

            // save the type
            Xvr_pushLiteralArray(&interpreter->literalCache,
                                 typeLiteral);  // copied

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(type ");
                Xvr_printLiteral(typeLiteral);
                printf(")\n");
            }
#endif

            Xvr_freeLiteral(typeLiteral);
        } break;

        case XVR_LITERAL_INDEX_BLANK:
            // read the blank
            Xvr_pushLiteralArray(&interpreter->literalCache,
                                 XVR_TO_INDEX_BLANK_LITERAL);

#ifndef XVR_EXPORT
            if (Xvr_commandLine.verbose) {
                printf("(blank)\n");
            }
#endif
            break;
        }
    }

    consumeByte(interpreter, XVR_OP_SECTION_END, interpreter->bytecode,
                &interpreter->count);  // terminate the literal section

    // read the function metadata
    int functionCount = readShort(interpreter->bytecode, &interpreter->count);
    int functionSize = readShort(interpreter->bytecode,
                                 &interpreter->count);  // might not be needed

    // read in the functions
    for (int i = 0; i < interpreter->literalCache.count; i++) {
        if (interpreter->literalCache.literals[i].type ==
            XVR_LITERAL_FUNCTION_INTERMEDIATE) {
            // get the size of the function
            size_t size =
                (size_t)readShort(interpreter->bytecode, &interpreter->count);

            // read the function code (literal cache and all)
            unsigned char* bytes = XVR_ALLOCATE(unsigned char, size);
            memcpy(bytes, interpreter->bytecode + interpreter->count,
                   size);  // TODO: -1 for the ending mark
            interpreter->count += size;

            // assert that the last memory slot is function end
            if (bytes[size - 1] != XVR_OP_FN_END) {
                interpreter->errorOutput(
                    "[internal] Failed to find function end");
                XVR_FREE_ARRAY(unsigned char, bytes, size);
                return;
            }

            // change the type to normal
            interpreter->literalCache.literals[i] =
                XVR_TO_FUNCTION_LITERAL(bytes, size);
            XVR_AS_FUNCTION(interpreter->literalCache.literals[i]).scope = NULL;
        }
    }

    consumeByte(interpreter, XVR_OP_SECTION_END, interpreter->bytecode,
                &interpreter->count);  // terminate the function section
}

// exposed functions
void Xvr_initInterpreter(Xvr_Interpreter* interpreter) {
    interpreter->hooks = XVR_ALLOCATE(Xvr_LiteralDictionary, 1);
    Xvr_initLiteralDictionary(interpreter->hooks);

    // set up the output streams
    Xvr_setInterpreterPrint(interpreter, printWrapper);
    Xvr_setInterpreterAssert(interpreter, assertWrapper);
    Xvr_setInterpreterError(interpreter, errorWrapper);

    interpreter->scope = NULL;
    Xvr_resetInterpreter(interpreter);
}

void Xvr_runInterpreter(Xvr_Interpreter* interpreter,
                        const unsigned char* bytecode, int length) {
    // initialize here instead of initInterpreter()
    Xvr_initLiteralArray(&interpreter->literalCache);
    interpreter->bytecode = NULL;
    interpreter->length = 0;
    interpreter->count = 0;
    interpreter->codeStart = -1;

    Xvr_initLiteralArray(&interpreter->stack);

    interpreter->depth = 0;
    interpreter->panic = false;

    // prep the bytecode
    interpreter->bytecode = bytecode;
    interpreter->length = length;
    interpreter->count = 0;

    if (!interpreter->bytecode) {
        interpreter->errorOutput("No valid bytecode given\n");
        return;
    }

    // prep the literal cache
    if (interpreter->literalCache.count > 0) {
        Xvr_freeLiteralArray(&interpreter->literalCache);  // automatically
                                                           // inits
    }

    // header section
    const unsigned char major =
        readByte(interpreter->bytecode, &interpreter->count);
    const unsigned char minor =
        readByte(interpreter->bytecode, &interpreter->count);
    const unsigned char patch =
        readByte(interpreter->bytecode, &interpreter->count);

    if (major != XVR_VERSION_MAJOR || minor > XVR_VERSION_MINOR) {
        char buffer[XVR_MAX_STRING_LENGTH];
        snprintf(buffer, XVR_MAX_STRING_LENGTH,
                 "Interpreter/bytecode version mismatch (expected %d.%d.%d or "
                 "earlier, given %d.%d.%d)\n",
                 XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH, major,
                 minor, patch);
        interpreter->errorOutput(buffer);
        return;
    }

    const char* build = readString(interpreter->bytecode, &interpreter->count);

#ifndef XVR_EXPORT
    if (Xvr_commandLine.verbose) {
        if (strncmp(build, XVR_VERSION_BUILD, strlen(XVR_VERSION_BUILD))) {
            printf(
                XVR_CC_WARN
                "Warning: interpreter/bytecode build mismatch\n" XVR_CC_RESET);
        }
    }
#endif

    consumeByte(interpreter, XVR_OP_SECTION_END, interpreter->bytecode,
                &interpreter->count);

    // read the sections of the bytecode
    readInterpreterSections(interpreter);

    // code section
#ifndef XVR_EXPORT
    if (Xvr_commandLine.verbose) {
        printf(XVR_CC_NOTICE "executing bytecode\n" XVR_CC_RESET);
    }
#endif

    // execute the interpreter
    execInterpreter(interpreter);

    while (interpreter->stack.count > 0) {
        Xvr_Literal lit = Xvr_popLiteralArray(&interpreter->stack);
        Xvr_freeLiteral(lit);
    }

    XVR_FREE_ARRAY(unsigned char, interpreter->bytecode, interpreter->length);

    Xvr_freeLiteralArray(&interpreter->literalCache);
    Xvr_freeLiteralArray(&interpreter->stack);
}

void Xvr_resetInterpreter(Xvr_Interpreter* interpreter) {
    while (interpreter->scope != NULL) {
        interpreter->scope = Xvr_popScope(interpreter->scope);
    }

    interpreter->scope = Xvr_pushScope(NULL);

    Xvr_injectNativeFn(interpreter, "_set", Xvr_private_set);
    Xvr_injectNativeFn(interpreter, "_get", Xvr_private_get);
    Xvr_injectNativeFn(interpreter, "_push", Xvr_private_push);
    Xvr_injectNativeFn(interpreter, "_pop", Xvr_private_pop);
    Xvr_injectNativeFn(interpreter, "_length", Xvr_private_length);
    Xvr_injectNativeFn(interpreter, "_clear", Xvr_private_clear);
}

void Xvr_freeInterpreter(Xvr_Interpreter* interpreter) {
    // free the interpreter scope
    while (interpreter->scope != NULL) {
        interpreter->scope = Xvr_popScope(interpreter->scope);
    }

    if (interpreter->hooks) {
        Xvr_freeLiteralDictionary(interpreter->hooks);
        XVR_FREE(Xvr_LiteralDictionary, interpreter->hooks);
    }

    interpreter->hooks = NULL;
}
