#include "xvr_literal.h"

#include <stdio.h>
#include <string.h>

#include "xvr_console_colors.h"
#include "xvr_interpreter.h"
#include "xvr_literal_array.h"
#include "xvr_literal_dictionary.h"
#include "xvr_memory.h"
#include "xvr_refstring.h"
#include "xvr_scope.h"

static unsigned int hashString(const char* string, int length) {
    unsigned int hash = 2166136261u;

    for (int i = 0; i < length; i++) {
        hash *= string[i];
        hash ^= 16777619;
    }

    return hash;
}

static unsigned int hashUInt(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

// exposed functions
void Xvr_freeLiteral(Xvr_Literal literal) {
    // refstrings
    if (XVR_IS_STRING(literal)) {
        Xvr_deleteRefString(XVR_AS_STRING(literal));
        return;
    }

    if (XVR_IS_IDENTIFIER(literal)) {
        Xvr_deleteRefString(XVR_AS_IDENTIFIER(literal));
        return;
    }

    // compounds
    if (XVR_IS_ARRAY(literal) ||
        literal.type == XVR_LITERAL_ARRAY_INTERMEDIATE ||
        literal.type == XVR_LITERAL_DICTIONARY_INTERMEDIATE ||
        literal.type == XVR_LITERAL_TYPE_INTERMEDIATE) {
        Xvr_freeLiteralArray(XVR_AS_ARRAY(literal));
        XVR_FREE(Xvr_LiteralArray, XVR_AS_ARRAY(literal));
        return;
    }

    if (XVR_IS_DICTIONARY(literal)) {
        Xvr_freeLiteralDictionary(XVR_AS_DICTIONARY(literal));
        XVR_FREE(Xvr_LiteralDictionary, XVR_AS_DICTIONARY(literal));
        return;
    }

    // complex literals
    if (XVR_IS_FUNCTION(literal)) {
        Xvr_popScope(XVR_AS_FUNCTION(literal).scope);
        XVR_AS_FUNCTION(literal).scope = NULL;
        XVR_FREE_ARRAY(unsigned char, XVR_AS_FUNCTION(literal).inner.bytecode,
                       XVR_AS_FUNCTION_BYTECODE_LENGTH(literal));
    }

    if (XVR_IS_TYPE(literal)) {
        for (int i = 0; i < XVR_AS_TYPE(literal).count; i++) {
            Xvr_freeLiteral(((Xvr_Literal*)(XVR_AS_TYPE(literal).subtypes))[i]);
        }
        XVR_FREE_ARRAY(Xvr_Literal, XVR_AS_TYPE(literal).subtypes,
                       XVR_AS_TYPE(literal).capacity);
        return;
    }

    // opaque types are managed externally - no action needed
    if (XVR_IS_OPAQUE(literal)) {
        return;
    }
}

bool Xvr_private_isTruthy(Xvr_Literal x) {
    if (XVR_IS_NULL(x)) {
        fprintf(stderr, XVR_CC_ERROR
                "XVR_CC_ERROR: Null is neither true nor false\n" XVR_CC_RESET);
        return false;
    }

    if (XVR_IS_BOOLEAN(x)) {
        return XVR_AS_BOOLEAN(x);
    }

    return true;
}

Xvr_Literal Xvr_private_toStringLiteral(Xvr_RefString* ptr) {
    return ((Xvr_Literal){{.string.ptr = ptr}, XVR_LITERAL_STRING, 0});
}

Xvr_Literal Xvr_private_toIdentifierLiteral(Xvr_RefString* ptr) {
    return ((Xvr_Literal){{.identifier.ptr = ptr,
                           .identifier.hash = hashString(
                               Xvr_toCString(ptr), Xvr_lengthRefString(ptr))},
                          XVR_LITERAL_IDENTIFIER,
                          0});
}

Xvr_Literal* Xvr_private_typePushSubtype(Xvr_Literal* lit,
                                         Xvr_Literal subtype) {
    // grow the subtype array
    if (XVR_AS_TYPE(*lit).count + 1 > XVR_AS_TYPE(*lit).capacity) {
        int oldCapacity = XVR_AS_TYPE(*lit).capacity;

        XVR_AS_TYPE(*lit).capacity = XVR_GROW_CAPACITY(oldCapacity);
        XVR_AS_TYPE(*lit).subtypes =
            XVR_GROW_ARRAY(Xvr_Literal, XVR_AS_TYPE(*lit).subtypes, oldCapacity,
                           XVR_AS_TYPE(*lit).capacity);
    }

    // actually push
    ((Xvr_Literal*)(XVR_AS_TYPE(*lit).subtypes))[XVR_AS_TYPE(*lit).count++] =
        subtype;
    return &((
        Xvr_Literal*)(XVR_AS_TYPE(*lit).subtypes))[XVR_AS_TYPE(*lit).count - 1];
}

Xvr_Literal Xvr_copyLiteral(Xvr_Literal original) {
    switch (original.type) {
    case XVR_LITERAL_NULL:
    case XVR_LITERAL_BOOLEAN:
    case XVR_LITERAL_INTEGER:
    case XVR_LITERAL_FLOAT:
        // no copying needed
        return original;

    case XVR_LITERAL_STRING: {
        return XVR_TO_STRING_LITERAL(
            Xvr_copyRefString(XVR_AS_STRING(original)));
    }

    case XVR_LITERAL_ARRAY: {
        Xvr_LiteralArray* array = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(array);

        // copy each element
        for (int i = 0; i < XVR_AS_ARRAY(original)->count; i++) {
            Xvr_pushLiteralArray(array, XVR_AS_ARRAY(original)->literals[i]);
        }

        return XVR_TO_ARRAY_LITERAL(array);
    }

    case XVR_LITERAL_DICTIONARY: {
        Xvr_LiteralDictionary* dictionary =
            XVR_ALLOCATE(Xvr_LiteralDictionary, 1);
        Xvr_initLiteralDictionary(dictionary);

        // copy each entry
        for (int i = 0; i < XVR_AS_DICTIONARY(original)->capacity; i++) {
            if (!XVR_IS_NULL(XVR_AS_DICTIONARY(original)->entries[i].key)) {
                Xvr_setLiteralDictionary(
                    dictionary, XVR_AS_DICTIONARY(original)->entries[i].key,
                    XVR_AS_DICTIONARY(original)->entries[i].value);
            }
        }

        return XVR_TO_DICTIONARY_LITERAL(dictionary);
    }

    case XVR_LITERAL_FUNCTION: {
        unsigned char* buffer = XVR_ALLOCATE(
            unsigned char, XVR_AS_FUNCTION_BYTECODE_LENGTH(original));
        memcpy(buffer, XVR_AS_FUNCTION(original).inner.bytecode,
               XVR_AS_FUNCTION_BYTECODE_LENGTH(original));

        Xvr_Literal literal = XVR_TO_FUNCTION_LITERAL(
            buffer, XVR_AS_FUNCTION_BYTECODE_LENGTH(original));
        XVR_AS_FUNCTION(literal).scope =
            Xvr_copyScope(XVR_AS_FUNCTION(original).scope);
        return literal;
    }

    case XVR_LITERAL_IDENTIFIER: {
        return XVR_TO_IDENTIFIER_LITERAL(
            Xvr_copyRefString(XVR_AS_IDENTIFIER(original)));
    }

    case XVR_LITERAL_TYPE: {
        Xvr_Literal lit = XVR_TO_TYPE_LITERAL(XVR_AS_TYPE(original).typeOf,
                                              XVR_AS_TYPE(original).constant);

        for (int i = 0; i < XVR_AS_TYPE(original).count; i++) {
            XVR_TYPE_PUSH_SUBTYPE(
                &lit, Xvr_copyLiteral(
                          ((Xvr_Literal*)(XVR_AS_TYPE(original).subtypes))[i]));
        }

        return lit;
    }

    case XVR_LITERAL_OPAQUE: {
        // opaque types have external ownership - runtime does not manage
        // lifecycle shallow copy is intentional: ptr/tag are borrowed, not
        // owned by runtime
        return original;
    }

    case XVR_LITERAL_ARRAY_INTERMEDIATE: {
        Xvr_LiteralArray* array = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(array);

        // copy each element
        for (int i = 0; i < XVR_AS_ARRAY(original)->count; i++) {
            Xvr_Literal literal =
                Xvr_copyLiteral(XVR_AS_ARRAY(original)->literals[i]);
            Xvr_pushLiteralArray(array, literal);
            Xvr_freeLiteral(literal);
        }

        Xvr_Literal ret = XVR_TO_ARRAY_LITERAL(array);
        ret.type = XVR_LITERAL_ARRAY_INTERMEDIATE;
        return ret;
    }

    case XVR_LITERAL_DICTIONARY_INTERMEDIATE: {
        Xvr_LiteralArray* array = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(array);

        // copy each element
        for (int i = 0; i < XVR_AS_ARRAY(original)->count; i++) {
            Xvr_Literal literal =
                Xvr_copyLiteral(XVR_AS_ARRAY(original)->literals[i]);
            Xvr_pushLiteralArray(array, literal);
            Xvr_freeLiteral(literal);
        }

        Xvr_Literal ret = XVR_TO_ARRAY_LITERAL(array);
        ret.type = XVR_LITERAL_DICTIONARY_INTERMEDIATE;
        return ret;
    }

    case XVR_LITERAL_TYPE_INTERMEDIATE: {
        Xvr_LiteralArray* array = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(array);

        // copy each element
        for (int i = 0; i < XVR_AS_ARRAY(original)->count; i++) {
            Xvr_Literal literal =
                Xvr_copyLiteral(XVR_AS_ARRAY(original)->literals[i]);
            Xvr_pushLiteralArray(array, literal);
            Xvr_freeLiteral(literal);
        }

        Xvr_Literal ret = XVR_TO_ARRAY_LITERAL(array);
        ret.type = XVR_LITERAL_TYPE_INTERMEDIATE;
        return ret;
    }

    case XVR_LITERAL_FUNCTION_INTERMEDIATE:  // caries a compiler
    case XVR_LITERAL_FUNCTION_NATIVE:
    case XVR_LITERAL_FUNCTION_HOOK:
    case XVR_LITERAL_INDEX_BLANK:
        // no copying possible
        return original;

    default:
        fprintf(stderr,
                XVR_CC_ERROR
                "XVR_CC_ERROR: Can't copy that literal type: %d\n" XVR_CC_RESET,
                original.type);
        return XVR_TO_NULL_LITERAL;
    }
}

bool Xvr_literalsAreEqual(Xvr_Literal lhs, Xvr_Literal rhs) {
    // utility for other things
    if (lhs.type != rhs.type) {
        // ints and floats are compatible
        if ((XVR_IS_INTEGER(lhs) || XVR_IS_FLOAT(lhs)) &&
            (XVR_IS_INTEGER(rhs) || XVR_IS_FLOAT(rhs))) {
            float lhsVal = XVR_IS_INTEGER(lhs) ? (float)XVR_AS_INTEGER(lhs)
                                               : XVR_AS_FLOAT(lhs);
            float rhsVal = XVR_IS_INTEGER(rhs) ? (float)XVR_AS_INTEGER(rhs)
                                               : XVR_AS_FLOAT(rhs);
            return lhsVal == rhsVal;
        }

        return false;
    }

    switch (lhs.type) {
    case XVR_LITERAL_NULL:
        return true;  // can only be true because of the check above

    case XVR_LITERAL_BOOLEAN:
        return XVR_AS_BOOLEAN(lhs) == XVR_AS_BOOLEAN(rhs);

    case XVR_LITERAL_INTEGER:
        return XVR_AS_INTEGER(lhs) == XVR_AS_INTEGER(rhs);

    case XVR_LITERAL_FLOAT:
        return XVR_AS_FLOAT(lhs) == XVR_AS_FLOAT(rhs);

    case XVR_LITERAL_STRING:
        return Xvr_equalsRefString(XVR_AS_STRING(lhs), XVR_AS_STRING(rhs));

    case XVR_LITERAL_ARRAY:
    case XVR_LITERAL_ARRAY_INTERMEDIATE:
    case XVR_LITERAL_DICTIONARY_INTERMEDIATE:
    case XVR_LITERAL_TYPE_INTERMEDIATE:
        // mismatched sizes
        if (XVR_AS_ARRAY(lhs)->count != XVR_AS_ARRAY(rhs)->count) {
            return false;
        }

        // mismatched elements (in order)
        for (int i = 0; i < XVR_AS_ARRAY(lhs)->count; i++) {
            if (!Xvr_literalsAreEqual(XVR_AS_ARRAY(lhs)->literals[i],
                                      XVR_AS_ARRAY(rhs)->literals[i])) {
                return false;
            }
        }
        return true;

    case XVR_LITERAL_DICTIONARY:
        // relatively slow, especially when nested
        for (int i = 0; i < XVR_AS_DICTIONARY(lhs)->capacity; i++) {
            if (!XVR_IS_NULL(XVR_AS_DICTIONARY(lhs)
                                 ->entries[i]
                                 .key)) {  // only compare non-null keys
                // check it exists in rhs
                if (!Xvr_existsLiteralDictionary(
                        XVR_AS_DICTIONARY(rhs),
                        XVR_AS_DICTIONARY(lhs)->entries[i].key)) {
                    return false;
                }

                // compare the values
                Xvr_Literal val = Xvr_getLiteralDictionary(
                    XVR_AS_DICTIONARY(rhs),
                    XVR_AS_DICTIONARY(lhs)
                        ->entries[i]
                        .key);  // TODO: could be more efficient
                if (!Xvr_literalsAreEqual(
                        XVR_AS_DICTIONARY(lhs)->entries[i].value, val)) {
                    Xvr_freeLiteral(val);
                    return false;
                }
                Xvr_freeLiteral(val);
            }
        }

        return true;

    case XVR_LITERAL_FUNCTION:
    case XVR_LITERAL_FUNCTION_NATIVE:
    case XVR_LITERAL_FUNCTION_HOOK:
        return false;  // functions are never equal
        break;

    case XVR_LITERAL_IDENTIFIER:
        // check shortcuts
        if (XVR_HASH_I(lhs) != XVR_HASH_I(rhs)) {
            return false;
        }

        return Xvr_equalsRefString(XVR_AS_IDENTIFIER(lhs),
                                   XVR_AS_IDENTIFIER(rhs));

    case XVR_LITERAL_TYPE:
        // check types
        if (XVR_AS_TYPE(lhs).typeOf != XVR_AS_TYPE(rhs).typeOf) {
            return false;
        }

        // const don't match
        if (XVR_AS_TYPE(lhs).constant != XVR_AS_TYPE(rhs).constant) {
            return false;
        }

        // check subtypes
        if (XVR_AS_TYPE(lhs).count != XVR_AS_TYPE(rhs).count) {
            return false;
        }

        // check array|dictionary signatures are the same (in order)
        if (XVR_AS_TYPE(lhs).typeOf == XVR_LITERAL_ARRAY ||
            XVR_AS_TYPE(lhs).typeOf == XVR_LITERAL_DICTIONARY) {
            for (int i = 0; i < XVR_AS_TYPE(lhs).count; i++) {
                if (!Xvr_literalsAreEqual(
                        ((Xvr_Literal*)(XVR_AS_TYPE(lhs).subtypes))[i],
                        ((Xvr_Literal*)(XVR_AS_TYPE(rhs).subtypes))[i])) {
                    return false;
                }
            }
        }
        return true;

    case XVR_LITERAL_OPAQUE:
        return false;  // IDK what this is!

    case XVR_LITERAL_ANY:
        return true;

    case XVR_LITERAL_FUNCTION_INTERMEDIATE:
        fprintf(
            stderr, XVR_CC_ERROR
            "[internal] Can't compare intermediate functions\n" XVR_CC_RESET);
        return false;

    case XVR_LITERAL_INDEX_BLANK:
        return false;

    default:
        // should never be seen
        fprintf(stderr,
                XVR_CC_ERROR
                "[internal] Unrecognized literal type in equality: "
                "%d\n" XVR_CC_RESET,
                lhs.type);
        return false;
    }

    return false;
}

int Xvr_hashLiteral(Xvr_Literal lit) {
    switch (lit.type) {
    case XVR_LITERAL_NULL:
        return 0;

    case XVR_LITERAL_BOOLEAN:
        return XVR_AS_BOOLEAN(lit) ? 1 : 0;

    case XVR_LITERAL_INTEGER:
        return hashUInt((unsigned int)XVR_AS_INTEGER(lit));

    case XVR_LITERAL_FLOAT:
        return hashUInt(*(unsigned int*)(&XVR_AS_FLOAT(lit)));

    case XVR_LITERAL_STRING:
        return hashString(Xvr_toCString(XVR_AS_STRING(lit)),
                          Xvr_lengthRefString(XVR_AS_STRING(lit)));

    case XVR_LITERAL_ARRAY: {
        unsigned int res = 0;
        for (int i = 0; i < XVR_AS_ARRAY(lit)->count; i++) {
            res += Xvr_hashLiteral(XVR_AS_ARRAY(lit)->literals[i]);
        }
        return hashUInt(res);
    }

    case XVR_LITERAL_DICTIONARY: {
        unsigned int res = 0;
        for (int i = 0; i < XVR_AS_DICTIONARY(lit)->capacity; i++) {
            if (!XVR_IS_NULL(XVR_AS_DICTIONARY(lit)
                                 ->entries[i]
                                 .key)) {  // only hash non-null keys
                res += Xvr_hashLiteral(XVR_AS_DICTIONARY(lit)->entries[i].key);
                res +=
                    Xvr_hashLiteral(XVR_AS_DICTIONARY(lit)->entries[i].value);
            }
        }
        return hashUInt(res);
    }

    case XVR_LITERAL_FUNCTION:
    case XVR_LITERAL_FUNCTION_NATIVE:
    case XVR_LITERAL_FUNCTION_HOOK:
        return 0;  // can't hash these

    case XVR_LITERAL_IDENTIFIER:
        return XVR_HASH_I(lit);  // pre-computed

    case XVR_LITERAL_TYPE:
        return XVR_AS_TYPE(lit).typeOf;  // nothing else I can do

    case XVR_LITERAL_OPAQUE:
    case XVR_LITERAL_ANY:
        return -1;

    default:
        // should never bee seen
        fprintf(
            stderr,
            XVR_CC_ERROR
            "[internal] Unrecognized literal type in hash: %d\n" XVR_CC_RESET,
            lit.type);
        return 0;
    }
}

// utils
static void stdoutWrapper(const char* output) { printf("%s", output); }

// buffer the prints
static char* globalPrintBuffer = NULL;
static size_t globalPrintCapacity = 0;
static size_t globalPrintCount = 0;

static char quotes = 0;  // set to 0 to not show string quotes

static void printToBuffer(const char* str) {
    while (strlen(str) + globalPrintCount + 1 > globalPrintCapacity) {
        int oldCapacity = globalPrintCapacity;

        globalPrintCapacity = XVR_GROW_CAPACITY(globalPrintCapacity);
        globalPrintBuffer = XVR_GROW_ARRAY(char, globalPrintBuffer, oldCapacity,
                                           globalPrintCapacity);
    }

    snprintf(globalPrintBuffer + globalPrintCount, strlen(str) + 1, "%s", str);
    globalPrintCount += strlen(str);
}

// exposed functions
void Xvr_printLiteral(Xvr_Literal literal) {
    Xvr_printLiteralCustom(literal, stdoutWrapper);
}

void Xvr_printLiteralCustom(Xvr_Literal literal, void(printFn)(const char*)) {
    switch (literal.type) {
    case XVR_LITERAL_NULL:
        printFn("null");
        break;

    case XVR_LITERAL_BOOLEAN:
        printFn(XVR_AS_BOOLEAN(literal) ? "true" : "false");
        break;

    case XVR_LITERAL_INTEGER: {
        char buffer[256];
        snprintf(buffer, 256, "%d", XVR_AS_INTEGER(literal));
        printFn(buffer);
    } break;

    case XVR_LITERAL_FLOAT: {
        char buffer[256];

        if (XVR_AS_FLOAT(literal) - (int)XVR_AS_FLOAT(literal)) {
            snprintf(buffer, 256, "%g", XVR_AS_FLOAT(literal));
        } else {
            snprintf(buffer, 256, "%.1f", XVR_AS_FLOAT(literal));
        }

        printFn(buffer);
    } break;

    case XVR_LITERAL_STRING: {
        char buffer[XVR_MAX_STRING_LENGTH];
        if (!quotes) {
            snprintf(buffer, XVR_MAX_STRING_LENGTH, "%.*s",
                     (int)Xvr_lengthRefString(XVR_AS_STRING(literal)),
                     Xvr_toCString(XVR_AS_STRING(literal)));
        } else {
            snprintf(buffer, XVR_MAX_STRING_LENGTH, "%c%.*s%c", quotes,
                     (int)Xvr_lengthRefString(XVR_AS_STRING(literal)),
                     Xvr_toCString(XVR_AS_STRING(literal)), quotes);
        }
        printFn(buffer);
    } break;

    case XVR_LITERAL_ARRAY: {
        Xvr_LiteralArray* ptr = XVR_AS_ARRAY(literal);

        // hold potential parent-call buffers on the C stack
        char* cacheBuffer = globalPrintBuffer;
        globalPrintBuffer = NULL;
        int cacheCapacity = globalPrintCapacity;
        globalPrintCapacity = 0;
        int cacheCount = globalPrintCount;
        globalPrintCount = 0;

        // print the contents to the global buffer
        printToBuffer("[");
        for (int i = 0; i < ptr->count; i++) {
            quotes = '"';
            Xvr_printLiteralCustom(ptr->literals[i], printToBuffer);

            if (i + 1 < ptr->count) {
                printToBuffer(",");
            }
        }
        printToBuffer("]");

        // swap the parent-call buffer back into place
        char* printBuffer = globalPrintBuffer;
        int printCapacity = globalPrintCapacity;
        int printCount = globalPrintCount;

        globalPrintBuffer = cacheBuffer;
        globalPrintCapacity = cacheCapacity;
        globalPrintCount = cacheCount;

        // finally, output and cleanup
        printFn(printBuffer);
        XVR_FREE_ARRAY(char, printBuffer, printCapacity);
        quotes = 0;
    } break;

    case XVR_LITERAL_DICTIONARY: {
        Xvr_LiteralDictionary* ptr = XVR_AS_DICTIONARY(literal);

        // hold potential parent-call buffers on the C stack
        char* cacheBuffer = globalPrintBuffer;
        globalPrintBuffer = NULL;
        int cacheCapacity = globalPrintCapacity;
        globalPrintCapacity = 0;
        int cacheCount = globalPrintCount;
        globalPrintCount = 0;

        // print the contents to the global buffer
        int delimCount = 0;
        printToBuffer("[");
        for (int i = 0; i < ptr->capacity; i++) {
            if (XVR_IS_NULL(ptr->entries[i].key)) {
                continue;
            }

            if (delimCount++ > 0) {
                printToBuffer(",");
            }

            quotes = '"';
            Xvr_printLiteralCustom(ptr->entries[i].key, printToBuffer);
            printToBuffer(":");
            quotes = '"';
            Xvr_printLiteralCustom(ptr->entries[i].value, printToBuffer);
        }

        // empty dicts MUST have a ":" printed
        if (ptr->count == 0) {
            printToBuffer(":");
        }

        printToBuffer("]");

        // swap the parent-call buffer back into place
        char* printBuffer = globalPrintBuffer;
        int printCapacity = globalPrintCapacity;
        int printCount = globalPrintCount;

        globalPrintBuffer = cacheBuffer;
        globalPrintCapacity = cacheCapacity;
        globalPrintCount = cacheCount;

        // finally, output and cleanup
        printFn(printBuffer);
        XVR_FREE_ARRAY(char, printBuffer, printCapacity);
        quotes = 0;
    } break;

    case XVR_LITERAL_FUNCTION:
    case XVR_LITERAL_FUNCTION_NATIVE:
    case XVR_LITERAL_FUNCTION_HOOK:
        printFn("(function)");
        break;

    case XVR_LITERAL_IDENTIFIER: {
        char buffer[256];
        snprintf(buffer, 256, "%.*s",
                 (int)Xvr_lengthRefString(XVR_AS_IDENTIFIER(literal)),
                 Xvr_toCString(XVR_AS_IDENTIFIER(literal)));
        printFn(buffer);
    } break;

    case XVR_LITERAL_TYPE: {
        char* cacheBuffer = globalPrintBuffer;
        globalPrintBuffer = NULL;
        int cacheCapacity = globalPrintCapacity;
        globalPrintCapacity = 0;
        int cacheCount = globalPrintCount;
        globalPrintCount = 0;

        printToBuffer("<");

        switch (XVR_AS_TYPE(literal).typeOf) {
        case XVR_LITERAL_NULL:
            printToBuffer("null");
            break;

        case XVR_LITERAL_BOOLEAN:
            printToBuffer("bool");
            break;

        case XVR_LITERAL_INTEGER:
            printToBuffer("int");
            break;

        case XVR_LITERAL_FLOAT:
            printToBuffer("float");
            break;

        case XVR_LITERAL_STRING:
            printToBuffer("string");
            break;

        case XVR_LITERAL_ARRAY:
            // print all in the array
            printToBuffer("[");
            for (int i = 0; i < XVR_AS_TYPE(literal).count; i++) {
                Xvr_printLiteralCustom(
                    ((Xvr_Literal*)(XVR_AS_TYPE(literal).subtypes))[i],
                    printToBuffer);
            }
            printToBuffer("]");
            break;

        case XVR_LITERAL_DICTIONARY:
            printToBuffer("[");

            for (int i = 0; i < XVR_AS_TYPE(literal).count; i += 2) {
                Xvr_printLiteralCustom(
                    ((Xvr_Literal*)(XVR_AS_TYPE(literal).subtypes))[i],
                    printToBuffer);
                printToBuffer(":");
                Xvr_printLiteralCustom(
                    ((Xvr_Literal*)(XVR_AS_TYPE(literal).subtypes))[i + 1],
                    printToBuffer);
            }
            printToBuffer("]");
            break;

        case XVR_LITERAL_FUNCTION:
            printToBuffer("proc");
            break;

        case XVR_LITERAL_FUNCTION_NATIVE:
            printToBuffer("native");
            break;

        case XVR_LITERAL_IDENTIFIER:
            printToBuffer("identifier");
            break;

        case XVR_LITERAL_TYPE:
            printToBuffer("type");
            break;

        case XVR_LITERAL_OPAQUE:
            printToBuffer("opaque");
            break;

        case XVR_LITERAL_ANY:
            printToBuffer("any");
            break;

        default:
            // should never be seen
            fprintf(stderr,
                    XVR_CC_ERROR
                    "[internal] Unrecognized literal type in print type: "
                    "%d\n" XVR_CC_RESET,
                    XVR_AS_TYPE(literal).typeOf);
        }

        // const (printed last)
        if (XVR_AS_TYPE(literal).constant) {
            printToBuffer(" const");
        }

        printToBuffer(">");

        char* printBuffer = globalPrintBuffer;
        int printCapacity = globalPrintCapacity;
        int printCount = globalPrintCount;

        globalPrintBuffer = cacheBuffer;
        globalPrintCapacity = cacheCapacity;
        globalPrintCount = cacheCount;

        printFn(printBuffer);
        XVR_FREE_ARRAY(char, printBuffer, printCapacity);
        quotes = 0;
    } break;

    case XVR_LITERAL_TYPE_INTERMEDIATE:
    case XVR_LITERAL_FUNCTION_INTERMEDIATE:
        printFn("Unprintable literal found");
        break;

    case XVR_LITERAL_OPAQUE:
        printFn("(opaque)");
        break;

    case XVR_LITERAL_ANY:
        printFn("(any)");
        break;

    default:
        fprintf(
            stderr,
            XVR_CC_ERROR
            "[internal] Unrecognized literal type in print: %d\n" XVR_CC_RESET,
            literal.type);
    }
}

static void printToBufferSimple(const char* str) {
    int len = strlen(str);
    if (globalPrintCount + len > globalPrintCapacity) {
        int avail = globalPrintCapacity - globalPrintCount;
        if (avail > 0) {
            memcpy(globalPrintBuffer + globalPrintCount, str, avail);
        }
        globalPrintCount = globalPrintCapacity;
        return;
    }
    memcpy(globalPrintBuffer + globalPrintCount, str, len);
    globalPrintCount += len;
}

void Xvr_printLiteralToBuffer(Xvr_Literal literal, char* buffer, int* bufferPos,
                              int bufferSize) {
    char* saveBuffer = globalPrintBuffer;
    int saveCapacity = globalPrintCapacity;
    int saveCount = globalPrintCount;

    globalPrintBuffer = buffer;
    globalPrintCapacity = bufferSize - 1;
    globalPrintCount = 0;

    Xvr_printLiteralCustom(literal, printToBufferSimple);

    *bufferPos = globalPrintCount;
    buffer[globalPrintCount] = '\0';

    globalPrintBuffer = saveBuffer;
    globalPrintCapacity = saveCapacity;
    globalPrintCount = saveCount;
}
