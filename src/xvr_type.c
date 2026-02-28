/**
 * MIT License
 * Copyright (c) 2025 arfy slowy
 *
 * @file xvr_type.c
 * @brief Type system implementation for XVR
 */

#include "xvr_type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_literal.h"

static Xvr_TypeTable g_type_table = {0};

static Xvr_Type* find_or_create_type(const char* name, Xvr_TypeKind kind) {
    for (int i = 0; i < g_type_table.count; i++) {
        if (g_type_table.types[i]->kind == kind &&
            strcmp(g_type_table.types[i]->name, name) == 0) {
            return g_type_table.types[i];
        }
    }
    return NULL;
}

Xvr_Type* Xvr_TypeCreateInteger(int size_bits, Xvr_Signedness signedness) {
    Xvr_Type* type = find_or_create_type("int", XVR_KIND_INTEGER);
    if (type) return type;

    type = calloc(1, sizeof(Xvr_Type));
    if (!type) return NULL;

    type->kind = XVR_KIND_INTEGER;
    type->data.integer.size_bits = size_bits;
    type->data.integer.signedness = signedness;
    type->name = "int";

    if (g_type_table.count < 32) {
        g_type_table.types[g_type_table.count++] = type;
    }

    return type;
}

Xvr_Type* Xvr_TypeCreateFloat(int size_bits) {
    char name[32];
    snprintf(name, sizeof(name), "float%d", size_bits);

    Xvr_Type* type = find_or_create_type(name, XVR_KIND_FLOAT);
    if (type) return type;

    type = calloc(1, sizeof(Xvr_Type));
    if (!type) return NULL;

    type->kind = XVR_KIND_FLOAT;
    type->data.float_type.size_bits = size_bits;
    type->name = strdup(name);

    if (g_type_table.count < 32) {
        g_type_table.types[g_type_table.count++] = type;
    }

    return type;
}

Xvr_Type* Xvr_TypeCreatePointer(Xvr_Type* pointee) {
    Xvr_Type* type = calloc(1, sizeof(Xvr_Type));
    if (!type) return NULL;

    type->kind = XVR_KIND_POINTER;
    type->data.pointer.pointee_type = pointee;
    type->name = "pointer";

    return type;
}

Xvr_Type* Xvr_TypeCreateBool(void) {
    static Xvr_Type* bool_type = NULL;
    if (bool_type) return bool_type;

    bool_type = calloc(1, sizeof(Xvr_Type));
    if (!bool_type) return NULL;

    bool_type->kind = XVR_KIND_BOOL;
    bool_type->name = "bool";

    return bool_type;
}

Xvr_Type* Xvr_TypeCreateVoid(void) {
    static Xvr_Type* void_type = NULL;
    if (void_type) return void_type;

    void_type = calloc(1, sizeof(Xvr_Type));
    if (!void_type) return NULL;

    void_type->kind = XVR_KIND_VOID;
    void_type->name = "void";

    return void_type;
}

Xvr_Type* Xvr_TypeCreateString(void) {
    static Xvr_Type* string_type = NULL;
    if (string_type) return string_type;

    string_type = calloc(1, sizeof(Xvr_Type));
    if (!string_type) return NULL;

    string_type->kind = XVR_KIND_STRING;
    string_type->name = "string";

    return string_type;
}

Xvr_Type* Xvr_TypeGetFromLiteral(int literal_type) {
    switch (literal_type) {
    case XVR_LITERAL_BOOLEAN:
        return Xvr_TypeCreateBool();
    case XVR_LITERAL_INTEGER:
        return Xvr_TypeCreateInteger(32, XVR_SIGNEDNESS_SIGNED);
    case XVR_LITERAL_INT8:
        return Xvr_TypeCreateInteger(8, XVR_SIGNEDNESS_SIGNED);
    case XVR_LITERAL_INT16:
        return Xvr_TypeCreateInteger(16, XVR_SIGNEDNESS_SIGNED);
    case XVR_LITERAL_INT32:
        return Xvr_TypeCreateInteger(32, XVR_SIGNEDNESS_SIGNED);
    case XVR_LITERAL_INT64:
        return Xvr_TypeCreateInteger(64, XVR_SIGNEDNESS_SIGNED);
    case XVR_LITERAL_UINT8:
        return Xvr_TypeCreateInteger(8, XVR_SIGNEDNESS_UNSIGNED);
    case XVR_LITERAL_UINT16:
        return Xvr_TypeCreateInteger(16, XVR_SIGNEDNESS_UNSIGNED);
    case XVR_LITERAL_UINT32:
        return Xvr_TypeCreateInteger(32, XVR_SIGNEDNESS_UNSIGNED);
    case XVR_LITERAL_UINT64:
        return Xvr_TypeCreateInteger(64, XVR_SIGNEDNESS_UNSIGNED);
    case XVR_LITERAL_FLOAT:
        return Xvr_TypeCreateFloat(32);
    case XVR_LITERAL_FLOAT16:
        return Xvr_TypeCreateFloat(16);
    case XVR_LITERAL_FLOAT32:
        return Xvr_TypeCreateFloat(32);
    case XVR_LITERAL_FLOAT64:
        return Xvr_TypeCreateFloat(64);
    case XVR_LITERAL_STRING:
        return Xvr_TypeCreateString();
    default:
        return NULL;
    }
}

int Xvr_TypeGetSizeBits(const Xvr_Type* type) {
    if (!type) return 0;

    switch (type->kind) {
    case XVR_KIND_INTEGER:
        return type->data.integer.size_bits;
    case XVR_KIND_FLOAT:
        return type->data.float_type.size_bits;
    case XVR_KIND_BOOL:
        return 8;
    case XVR_KIND_POINTER:
        return 64;
    default:
        return 0;
    }
}

Xvr_Signedness Xvr_TypeGetSignedness(const Xvr_Type* type) {
    if (!type) return XVR_SIGNEDNESS_UNKNOWN;

    if (type->kind == XVR_KIND_INTEGER) {
        return type->data.integer.signedness;
    }
    return XVR_SIGNEDNESS_UNKNOWN;
}

Xvr_TypeKind Xvr_TypeGetKind(const Xvr_Type* type) {
    return type ? type->kind : XVR_KIND_VOID;
}

bool Xvr_TypeIsInteger(const Xvr_Type* type) {
    return type && type->kind == XVR_KIND_INTEGER;
}

bool Xvr_TypeIsFloat(const Xvr_Type* type) {
    return type && type->kind == XVR_KIND_FLOAT;
}

bool Xvr_TypeIsNumeric(const Xvr_Type* type) {
    return Xvr_TypeIsInteger(type) || Xvr_TypeIsFloat(type);
}

bool Xvr_TypeIsPointer(const Xvr_Type* type) {
    return type && type->kind == XVR_KIND_POINTER;
}

bool Xvr_TypeIsBool(const Xvr_Type* type) {
    return type && type->kind == XVR_KIND_BOOL;
}

bool Xvr_TypeEquals(const Xvr_Type* a, const Xvr_Type* b) {
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;

    switch (a->kind) {
    case XVR_KIND_INTEGER:
        return a->data.integer.size_bits == b->data.integer.size_bits &&
               a->data.integer.signedness == b->data.integer.signedness;
    case XVR_KIND_FLOAT:
        return a->data.float_type.size_bits == b->data.float_type.size_bits;
    case XVR_KIND_BOOL:
    case XVR_KIND_STRING:
    case XVR_KIND_VOID:
        return true;
    case XVR_KIND_POINTER:
        return Xvr_TypeEquals(a->data.pointer.pointee_type,
                              b->data.pointer.pointee_type);
    default:
        return false;
    }
}

const char* Xvr_TypeToString(const Xvr_Type* type) {
    static char buffer[64];

    if (!type) return "unknown";

    switch (type->kind) {
    case XVR_KIND_INTEGER:
        snprintf(buffer, sizeof(buffer), "int%d%s",
                 type->data.integer.size_bits,
                 type->data.integer.signedness == XVR_SIGNEDNESS_UNSIGNED ? "u"
                                                                          : "");
        return buffer;
    case XVR_KIND_FLOAT:
        snprintf(buffer, sizeof(buffer), "float%d",
                 type->data.float_type.size_bits);
        return buffer;
    case XVR_KIND_BOOL:
        return "bool";
    case XVR_KIND_STRING:
        return "string";
    case XVR_KIND_POINTER:
        snprintf(buffer, sizeof(buffer), "pointer<%s>",
                 Xvr_TypeToString(type->data.pointer.pointee_type));
        return buffer;
    case XVR_KIND_VOID:
        return "void";
    default:
        return "unknown";
    }
}

void Xvr_TypeDestroy(Xvr_Type* type) {
    if (!type) return;
    free((void*)type->name);
    free(type);
}
