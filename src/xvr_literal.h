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

#ifndef XVR_LITERAL_H
#define XVR_LITERAL_H

#include <inttypes.h>
#include <string.h>

#include "xvr_common.h"
#include "xvr_refstring.h"

struct Xvr_Literal;
struct Xvr_Interpreter;
struct Xvr_LiteralArray;
typedef int (*Xvr_NativeFn)(struct Xvr_Interpreter* interpreter,
                            struct Xvr_LiteralArray* arguments);
typedef int (*Xvr_HookFn)(struct Xvr_Interpreter* interpreter,
                          struct Xvr_Literal identifier,
                          struct Xvr_Literal alias);

typedef enum {
    XVR_LITERAL_NULL,
    XVR_LITERAL_BOOLEAN,
    XVR_LITERAL_INTEGER,
    XVR_LITERAL_FLOAT,
    XVR_LITERAL_STRING,
    XVR_LITERAL_ARRAY,
    XVR_LITERAL_DICTIONARY,
    XVR_LITERAL_FUNCTION,
    XVR_LITERAL_IDENTIFIER,
    XVR_LITERAL_TYPE,
    XVR_LITERAL_OPAQUE,
    XVR_LITERAL_ANY,

    XVR_LITERAL_TYPE_INTERMEDIATE,
    XVR_LITERAL_ARRAY_INTERMEDIATE,
    XVR_LITERAL_DICTIONARY_INTERMEDIATE,
    XVR_LITERAL_FUNCTION_INTERMEDIATE,
    XVR_LITERAL_FUNCTION_ARG_REST,
    XVR_LITERAL_FUNCTION_NATIVE,
    XVR_LITERAL_FUNCTION_HOOK,
    XVR_LITERAL_INDEX_BLANK,
} Xvr_LiteralType;

typedef struct Xvr_Literal {
    Xvr_LiteralType type;
    union {
        bool boolean;
        int integer;
        float number;
        struct {
            Xvr_RefString* ptr;
        } string;

        void* array;
        void* dictionary;

        struct {
            void* bytecode;
            Xvr_NativeFn native;
            Xvr_HookFn hook;
            void* scope;
            int length;
        } function;

        struct {
            Xvr_RefString* ptr;
            int hash;
        } identifier;

        struct {
            Xvr_LiteralType typeOf;
            bool constant;
            void* subtypes;
            int capacity;
            int count;
        } type;

        struct {
            void* ptr;
            int tag;
        } opaque;
    } as;
} Xvr_Literal;

#define XVR_IS_NULL(value) ((value).type == XVR_LITERAL_NULL)
#define XVR_IS_BOOLEAN(value) ((value).type == XVR_LITERAL_BOOLEAN)
#define XVR_IS_INTEGER(value) ((value).type == XVR_LITERAL_INTEGER)
#define XVR_IS_FLOAT(value) ((value).type == XVR_LITERAL_FLOAT)
#define XVR_IS_STRING(value) ((value).type == XVR_LITERAL_STRING)
#define XVR_IS_ARRAY(value) ((value).type == XVR_LITERAL_ARRAY)
#define XVR_IS_DICTIONARY(value) ((value).type == XVR_LITERAL_DICTIONARY)
#define XVR_IS_FUNCTION(value) ((value).type == XVR_LITERAL_FUNCTION)
#define XVR_IS_FUNCTION_NATIVE(value) \
    ((value).type == XVR_LITERAL_FUNCTION_NATIVE)
#define XVR_IS_FUNCTION_HOOK(value) ((value).type == XVR_LITERAL_FUNCTION_HOOK)
#define XVR_IS_IDENTIFIER(value) ((value).type == XVR_LITERAL_IDENTIFIER)
#define XVR_IS_TYPE(value) ((value).type == XVR_LITERAL_TYPE)
#define XVR_IS_OPAQUE(value) ((value).type == XVR_LITERAL_OPAQUE)

#define XVR_AS_BOOLEAN(value) ((value).as.boolean)
#define XVR_AS_INTEGER(value) ((value).as.integer)
#define XVR_AS_FLOAT(value) ((value).as.number)
#define XVR_AS_STRING(value) ((value).as.string.ptr)
#define XVR_AS_ARRAY(value) ((Xvr_LiteralArray*)((value).as.array))
#define XVR_AS_DICTIONARY(value) \
    ((Xvr_LiteralDictionary*)((value).as.dictionary))
#define XVR_AS_FUNCTION(value) ((value).as.function)
#define XVR_AS_FUNCTION_NATIVE(value) ((value).as.function.native)
#define XVR_AS_FUNCTION_HOOK(value) ((value).as.function.hook)
#define XVR_AS_IDENTIFIER(value) ((value).as.identifier.ptr)
#define XVR_AS_TYPE(value) ((value).as.type)
#define XVR_AS_OPAQUE(value) ((value).as.opaque.ptr)

#define XVR_TO_NULL_LITERAL ((Xvr_Literal){XVR_LITERAL_NULL, {.integer = 0}})
#define XVR_TO_BOOLEAN_LITERAL(value) \
    ((Xvr_Literal){XVR_LITERAL_BOOLEAN, {.boolean = value}})
#define XVR_TO_INTEGER_LITERAL(value) \
    ((Xvr_Literal){XVR_LITERAL_INTEGER, {.integer = value}})
#define XVR_TO_FLOAT_LITERAL(value) \
    ((Xvr_Literal){XVR_LITERAL_FLOAT, {.number = value}})
#define XVR_TO_STRING_LITERAL(value) Xvr_private_toStringLiteral(value)
#define XVR_TO_ARRAY_LITERAL(value) \
    ((Xvr_Literal){XVR_LITERAL_ARRAY, {.array = value}})
#define XVR_TO_DICTIONARY_LITERAL(value) \
    ((Xvr_Literal){XVR_LITERAL_DICTIONARY, {.dictionary = value}})
#define XVR_TO_FUNCTION_LITERAL(value, l)       \
    ((Xvr_Literal){XVR_LITERAL_FUNCTION,        \
                   {.function.bytecode = value, \
                    .function.scope = NULL,     \
                    .function.length = l}})
#define XVR_TO_FUNCTION_NATIVE_LITERAL(value)   \
    ((Xvr_Literal){XVR_LITERAL_FUNCTION_NATIVE, \
                   {.function.native = value,   \
                    .function.scope = NULL,     \
                    .function.length = 0}})
#define XVR_TO_FUNCTION_HOOK_LITERAL(value)   \
    ((Xvr_Literal){XVR_LITERAL_FUNCTION_HOOK, \
                   {.function.hook = value,   \
                    .function.scope = NULL,   \
                    .function.length = 0}})
#define XVR_TO_IDENTIFIER_LITERAL(value) Xvr_private_toIdentifierLiteral(value)
#define XVR_TO_TYPE_LITERAL(value, c)      \
    ((Xvr_Literal){XVR_LITERAL_TYPE,       \
                   {.type.typeOf = value,  \
                    .type.constant = c,    \
                    .type.subtypes = NULL, \
                    .type.capacity = 0,    \
                    .type.count = 0}})
#define XVR_TO_OPAQUE_LITERAL(value, t) \
    ((Xvr_Literal){XVR_LITERAL_OPAQUE, {.opaque.ptr = value, .opaque.tag = t}})

#define XVR_IS_INDEX_BLANK(value) ((value).type == XVR_LITERAL_INDEX_BLANK)
#define XVR_TO_INDEX_BLANK_LITERAL \
    ((Xvr_Literal){XVR_LITERAL_INDEX_BLANK, {.integer = 0}})

XVR_API void Xvr_freeLiteral(Xvr_Literal literal);

#define XVR_IS_TRUTHY(x) Xvr_private_isTruthy(x)

#define XVR_MAX_STRING_LENGTH 4096
#define XVR_HASH_I(lit) ((lit).as.identifier.hash)
#define XVR_TYPE_PUSH_SUBTYPE(lit, subtype) \
    Xvr_private_typePushSubtype(lit, subtype)
#define XVR_GET_OPAQUE_TAG(o) o.as.opaque.tag

XVR_API bool Xvr_private_isTruthy(Xvr_Literal x);
XVR_API Xvr_Literal Xvr_private_toStringLiteral(Xvr_RefString* ptr);
XVR_API Xvr_Literal Xvr_private_toIdentifierLiteral(Xvr_RefString* ptr);
XVR_API Xvr_Literal* Xvr_private_typePushSubtype(Xvr_Literal* lit,
                                                 Xvr_Literal subtype);

XVR_API Xvr_Literal Xvr_copyLiteral(Xvr_Literal original);
XVR_API bool Xvr_literalsAreEqual(Xvr_Literal lhs, Xvr_Literal rhs);
XVR_API int Xvr_hashLiteral(Xvr_Literal lit);

XVR_API void Xvr_printLiteral(Xvr_Literal literal);
XVR_API void Xvr_printLiteralCustom(Xvr_Literal literal,
                                    void(printFn)(const char*));

#endif  // !XVR_LITERAL_H
