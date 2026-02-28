/**
 * MIT License
 * Copyright (c) 2025 arfy slowy
 *
 * @file xvr_type.h
 * @brief Type system for XVR - handles type representation and operations
 */

#ifndef XVR_TYPE_H
#define XVR_TYPE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    XVR_KIND_VOID,
    XVR_KIND_BOOL,
    XVR_KIND_INTEGER,
    XVR_KIND_FLOAT,
    XVR_KIND_STRING,
    XVR_KIND_ARRAY,
    XVR_KIND_DICTIONARY,
    XVR_KIND_POINTER,
    XVR_KIND_FUNCTION,
    XVR_KIND_OPAQUE,
} Xvr_TypeKind;

typedef enum {
    XVR_SIGNEDNESS_SIGNED,
    XVR_SIGNEDNESS_UNSIGNED,
    XVR_SIGNEDNESS_UNKNOWN,
} Xvr_Signedness;

typedef enum {
    XVR_CAST_WIDENING,
    XVR_CAST_NARROWING,
    XVR_CAST_INVALID,
} Xvr_CastKind;

typedef struct Xvr_Type {
    Xvr_TypeKind kind;

    union {
        struct {
            int size_bits;
            Xvr_Signedness signedness;
        } integer;

        struct {
            int size_bits;
        } float_type;

        struct {
            struct Xvr_Type* element_type;
        } array;

        struct {
            struct Xvr_Type* key_type;
            struct Xvr_Type* value_type;
        } dictionary;

        struct {
            struct Xvr_Type* pointee_type;
        } pointer;

        struct {
            struct Xvr_Type** param_types;
            int param_count;
            struct Xvr_Type* return_type;
        } function;
    } data;

    const char* name;
} Xvr_Type;

typedef struct Xvr_TypeTable {
    Xvr_Type* types[32];
    int count;
} Xvr_TypeTable;

Xvr_Type* Xvr_TypeCreateInteger(int size_bits, Xvr_Signedness signedness);
Xvr_Type* Xvr_TypeCreateFloat(int size_bits);
Xvr_Type* Xvr_TypeCreatePointer(Xvr_Type* pointee);
Xvr_Type* Xvr_TypeCreateBool(void);
Xvr_Type* Xvr_TypeCreateVoid(void);
Xvr_Type* Xvr_TypeCreateString(void);
Xvr_Type* Xvr_TypeGetFromLiteral(int literal_type);

int Xvr_TypeGetSizeBits(const Xvr_Type* type);
Xvr_Signedness Xvr_TypeGetSignedness(const Xvr_Type* type);
Xvr_TypeKind Xvr_TypeGetKind(const Xvr_Type* type);
bool Xvr_TypeIsInteger(const Xvr_Type* type);
bool Xvr_TypeIsFloat(const Xvr_Type* type);
bool Xvr_TypeIsNumeric(const Xvr_Type* type);
bool Xvr_TypeIsPointer(const Xvr_Type* type);
bool Xvr_TypeIsBool(const Xvr_Type* type);

bool Xvr_TypeEquals(const Xvr_Type* a, const Xvr_Type* b);
const char* Xvr_TypeToString(const Xvr_Type* type);

void Xvr_TypeDestroy(Xvr_Type* type);

#endif
