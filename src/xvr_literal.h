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

/**
 * @brief unified value representation for the XVR runtime: dynamically typed,
 * GC-friendly literals
 *
 * design goals:
 *  - zero overhead abstaction: sizeof(Xvr_Literal) = 16 - 24 byte (cache-line
 * friendly)
 *  - explicit ownership semantics: manual refcounting via `Xvr_RefString`
 *
 * memory Layout (typical on LP64):
 *     +------------------+-----------------+------------------+
 *     | type (4 bytes)   | padding (4)     | union (16 bytes) |
 *     +------------------+-----------------+------------------+
 *     total: 24 bytes (fits in 3x64b registers; 2 cache lines max)
 *
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

/**
 * @typedef Xvr_NativeFn
 * @brief C function callable from XVR bytecode (native extension)
 *
 * @param[in, out] interpreter current VM instance (for stack, error, GC access)
 * @param[in] arguments array of arguments (length known from call site)
 * @return integer status code
 *  - 0 -> succes
 *  - < 0 -> runtime error
 *  - > 0 -> reserved
 */
typedef int (*Xvr_NativeFn)(struct Xvr_Interpreter* interpreter,
                            struct Xvr_LiteralArray* arguments);

/**
 * @typedef Xvr_HookFn
 * @brief callback for identifier resolution / interception
 *
 * invoked when:
 *  - global identifier is accessed
 *  - module import
 *  - debugger instropection runs
 *
 * @param[in, out] interpreter VM context
 * @param[in] identifier the symbol being lookup
 * @param[in] alias optional alias
 * @return same convetion as `Xvr_NativeFn`
 */
typedef int (*Xvr_HookFn)(struct Xvr_Interpreter* interpreter,
                          struct Xvr_Literal identifier,
                          struct Xvr_Literal alias);

/**
 * @enum Xvr_LiteralType
 * @brief discriminant for `Xvr_Literal` union
 *
 * Ordering:
 *  - primitive (NULL to string) -> fast switch / case
 *  - aggregate (ARRAY, DICTIONARY)
 *  - code objects (FUNCTION variants)
 *  - Metadata / compile-time types (IDENTIFIER, TYPE)
 *  - opaque / any for FFI
 */
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

/**
 * @strcut Xvr_Literal
 * @brief tagged union representing any runtime value
 *
 * invariants
 * - if type is XVR_LITERAL_STRING or XVR_STRING_IDENTIFIER, `as.string.ptr` /
 * `as.identifier.ptr` must be non-NULL and have `refCount >= 1`
 * - if type is XVR_LITERAL_ARRAY / DICTIONARY, pointers must be valid or NULL
 * - XVR_LITERAL_FUNCTION variants store distinct metadata in same union layout
 * - padding after `type` ensure `union as` is naturally aligned (critical for
 * `double` if added)
 *
 * union layout:
 * - `boolean`, `integer`, `number`: stored value (fast)
 * - `string`, `identifier`: store `Xvr_RefString` (shared ownership)
 * - `array`, `dictionary`: void* to avoiding circular header deps
 * - `function`: polymorphic container:
 *    -`.bytecode`: `void*` to compiled chunk
 *    - `.naative`: Xvr_NativeFn
 *    - `.hook`: `Xvr_HookFn`
 *    - `.scope`: closure environment
 *    - `.length`: arity (argument count), for rest args
 * - identifier: including precomputed hash
 * - type: supports parametric types via `subtypes` array
 * - opaque: `ptr` + `tag` enables safe downcastring
 */
typedef struct Xvr_Literal {
    union {
        bool boolean;  // XVR_LITERAL_BOOLEAN
        int integer;   // XVR_LITERAL_INTEGER
        float number;  // XVR_LITERAL_FLOAT

        // XVR_LITERAL_STRING or XVR_LITERAL_IDENTIFIER
        struct {
            Xvr_RefString* ptr;  // Ref-counted string data
        } string;

        // XVR_LITERAL_ARRAY
        void* array;
        // XVR_LITERAL_DICTIONARY
        void* dictionary;

        // XVR_LITERAL_FUNCTION, XVR_LITERAL_FUNCTION_NATIVE,
        // XVR_LITERAL_FUNCTION_HOOK
        struct {
            union {
                void* bytecode;
                Xvr_NativeFn native;
                Xvr_HookFn hook;
            } inner;
            void* scope;
        } function;

        // XVR_LITERAL_IDENTIFIER
        struct {
            Xvr_RefString* ptr;
            int hash;
        } identifier;

        // XVR_LITERAL_TYPE
        struct {
            void* subtypes;
            Xvr_LiteralType typeOf;
            unsigned char capacity;
            unsigned char count;
            bool constant;
        } type;

        // XVR_LITERAL_OPAQUE
        struct {
            void* ptr;
            int tag;
        } opaque;
    } as;

    Xvr_LiteralType type;
    int bytecodeLength;
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
#define XVR_AS_FUNCTION_NATIVE(value) ((value).as.function.inner.native)
#define XVR_AS_FUNCTION_HOOK(value) ((value).as.function.inner.hook)
#define XVR_AS_IDENTIFIER(value) ((value).as.identifier.ptr)
#define XVR_AS_TYPE(value) ((value).as.type)
#define XVR_AS_OPAQUE(value) ((value).as.opaque.ptr)

#define XVR_TO_NULL_LITERAL ((Xvr_Literal){{.integer = 0}, XVR_LITERAL_NULL, 0})
#define XVR_TO_BOOLEAN_LITERAL(value) \
    ((Xvr_Literal){{.boolean = value}, XVR_LITERAL_BOOLEAN, 0})
#define XVR_TO_INTEGER_LITERAL(value) \
    ((Xvr_Literal){{.integer = value}, XVR_LITERAL_INTEGER, 0})
#define XVR_TO_FLOAT_LITERAL(value) \
    ((Xvr_Literal){{.number = value}, XVR_LITERAL_FLOAT, 0})
#define XVR_TO_STRING_LITERAL(value) Xvr_private_toStringLiteral(value)
#define XVR_TO_ARRAY_LITERAL(value) \
    ((Xvr_Literal){{.array = value}, XVR_LITERAL_ARRAY, 0})
#define XVR_TO_DICTIONARY_LITERAL(value) \
    ((Xvr_Literal){{.dictionary = value}, XVR_LITERAL_DICTIONARY, 0})
#define XVR_TO_FUNCTION_LITERAL(value, l)                                      \
    ((Xvr_Literal){{.function.inner.bytecode = value, .function.scope = NULL}, \
                   XVR_LITERAL_FUNCTION,                                       \
                   l})
#define XVR_TO_FUNCTION_NATIVE_LITERAL(value)                                \
    ((Xvr_Literal){{.function.inner.native = value, .function.scope = NULL}, \
                   XVR_LITERAL_FUNCTION_NATIVE,                              \
                   0})
#define XVR_TO_FUNCTION_HOOK_LITERAL(value)                                \
    ((Xvr_Literal){{.function.inner.hook = value, .function.scope = NULL}, \
                   XVR_LITERAL_FUNCTION_HOOK,                              \
                   0})
#define XVR_TO_IDENTIFIER_LITERAL(value) Xvr_private_toIdentifierLiteral(value)
#define XVR_TO_TYPE_LITERAL(value, c)      \
    ((Xvr_Literal){{.type.typeOf = value,  \
                    .type.constant = c,    \
                    .type.subtypes = NULL, \
                    .type.capacity = 0,    \
                    .type.count = 0},      \
                   XVR_LITERAL_TYPE,       \
                   0})
#define XVR_TO_OPAQUE_LITERAL(value, t)                    \
    ((Xvr_Literal){{.opaque.ptr = value, .opaque.tag = t}, \
                   XVR_LITERAL_OPAQUE,                     \
                   0})

#define XVR_IS_INDEX_BLANK(value) ((value).type == XVR_LITERAL_INDEX_BLANK)
#define XVR_TO_INDEX_BLANK_LITERAL \
    ((Xvr_Literal){{.integer = 0}, XVR_LITERAL_INDEX_BLANK, 0})

/**
 * @brief release resource
 *
 * by type
 * - STRING, IDENTIFIER
 * - ARRAY, DICTIONARY
 * - PROCEDURE
 */
XVR_API void Xvr_freeLiteral(Xvr_Literal literal);

#define XVR_IS_TRUTHY(x) Xvr_private_isTruthy(x)

#define XVR_AS_FUNCTION_BYTECODE_LENGTH(lit) ((lit).bytecodeLength)

#define XVR_MAX_STRING_LENGTH 4096
#define XVR_HASH_I(lit) ((lit).as.identifier.hash)
#define XVR_TYPE_PUSH_SUBTYPE(lit, subtype) \
    Xvr_private_typePushSubtype(lit, subtype)
#define XVR_GET_OPAQUE_TAG(o) o.as.opaque.tag

/**
 * @private
 * @brief implement truthniess semantics
 */
XVR_API bool Xvr_private_isTruthy(Xvr_Literal x);

/**
 * @private
 * @brief convert `Xvr_String` to `XVR_LITERAL_STRING`, incrementing refcount
 * @pre `ptr != NULL`
 */
XVR_API Xvr_Literal Xvr_private_toStringLiteral(Xvr_RefString* ptr);
XVR_API Xvr_Literal Xvr_private_toIdentifierLiteral(Xvr_RefString* ptr);
XVR_API Xvr_Literal* Xvr_private_typePushSubtype(Xvr_Literal* lit,
                                                 Xvr_Literal subtype);

/**
 * @brief compare two literal for value equality
 *
 * @param[in] lhs, rhs literals to compare
 * @return `true` if equal
 */
XVR_API Xvr_Literal Xvr_copyLiteral(Xvr_Literal original);
XVR_API bool Xvr_literalsAreEqual(Xvr_Literal lhs, Xvr_Literal rhs);
XVR_API int Xvr_hashLiteral(Xvr_Literal lit);

/**
 * @brief print literal to `stdout` in human-readable
 *
 * for sample:
 * `proc` -> `<native procedure>` or `<bytecode @0xdeadbeef>`
 *
 * @param[in] lliteral literal to print it
 */
XVR_API void Xvr_printLiteral(Xvr_Literal literal);

/**
 * @brief print literal using custom output function (example: to buffer, file)
 *
 * @param[in] literal literal to print it
 * @param[in] printFn function to receive each output chunk (example: `fwrite`,
 * `sbuf_push`)
 */
XVR_API void Xvr_printLiteralCustom(Xvr_Literal literal,
                                    void(printFn)(const char*));

#endif  // !XVR_LITERAL_H
