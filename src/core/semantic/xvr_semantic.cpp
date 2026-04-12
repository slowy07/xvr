/**
 * MIT License
 * Copyright (c) 2025 arfy slowy
 *
 * @file xvr_semantic.c
 * @brief Semantic analysis implementation for XVR
 */

#include "xvr_semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ast/xvr_ast_node.h"
#include "../types/xvr_type.h"

struct Xvr_SemanticAnalyzer {
    Xvr_SemanticError errors[16];
    int error_count;
    int current_line;
    int current_col;
};

Xvr_SemanticAnalyzer* Xvr_SemanticAnalyzerCreate(void) {
    Xvr_SemanticAnalyzer* analyzer = (Xvr_SemanticAnalyzer*)calloc(1, sizeof(Xvr_SemanticAnalyzer));
    return analyzer;
}

void Xvr_SemanticAnalyzerDestroy(Xvr_SemanticAnalyzer* analyzer) {
    free(analyzer);
}

bool Xvr_IsWidening(Xvr_Type* from, Xvr_Type* to) {
    if (!from || !to) return false;

    if (from->kind == XVR_KIND_INTEGER && to->kind == XVR_KIND_INTEGER) {
        return to->data.integer.size_bits >= from->data.integer.size_bits;
    }

    if (from->kind == XVR_KIND_FLOAT && to->kind == XVR_KIND_FLOAT) {
        return to->data.float_type.size_bits >= from->data.float_type.size_bits;
    }

    if (from->kind == XVR_KIND_INTEGER && to->kind == XVR_KIND_FLOAT) {
        return to->data.float_type.size_bits >= from->data.integer.size_bits;
    }

    return false;
}

bool Xvr_IsNarrowing(Xvr_Type* from, Xvr_Type* to) {
    return !Xvr_IsWidening(from, to) && Xvr_TypeIsNumeric(from) &&
           Xvr_TypeIsNumeric(to);
}

Xvr_CastKind Xvr_GetCastKind(Xvr_Type* from, Xvr_Type* to) {
    if (!from || !to) return XVR_CAST_INVALID;
    if (Xvr_TypeEquals(from, to)) return XVR_CAST_WIDENING;
    if (Xvr_IsWidening(from, to)) return XVR_CAST_WIDENING;
    if (Xvr_IsNarrowing(from, to)) return XVR_CAST_NARROWING;
    return XVR_CAST_INVALID;
}

Xvr_CastResult Xvr_CanCast(Xvr_Type* from, Xvr_Type* to, bool isExplicit) {
    if (!from || !to) return XVR_CAST_RESULT_INVALID;

    if (Xvr_TypeEquals(from, to)) {
        return XVR_CAST_RESULT_OK;
    }

    if (from->kind == XVR_KIND_INTEGER && to->kind == XVR_KIND_INTEGER) {
        if (from->data.integer.size_bits == to->data.integer.size_bits &&
            from->data.integer.signedness != to->data.integer.signedness) {
            return XVR_CAST_RESULT_SIGN_CONVERSION;
        }
        return XVR_CAST_RESULT_OK;
    }

    if (from->kind == XVR_KIND_FLOAT && to->kind == XVR_KIND_FLOAT) {
        return XVR_CAST_RESULT_OK;
    }

    if (from->kind == XVR_KIND_INTEGER && to->kind == XVR_KIND_FLOAT) {
        return XVR_CAST_RESULT_OK;
    }

    if (from->kind == XVR_KIND_FLOAT && to->kind == XVR_KIND_INTEGER) {
        if (!isExplicit) {
            return XVR_CAST_RESULT_IMPLICIT_NARROWING;
        }
        return XVR_CAST_RESULT_OK;
    }

    if (from->kind == XVR_KIND_BOOL) {
        if (to->kind == XVR_KIND_INTEGER || to->kind == XVR_KIND_FLOAT) {
            return XVR_CAST_RESULT_OK;
        }
    }

    if (to->kind == XVR_KIND_BOOL) {
        if (from->kind == XVR_KIND_INTEGER || from->kind == XVR_KIND_FLOAT) {
            return XVR_CAST_RESULT_OK;
        }
    }

    if (from->kind == XVR_KIND_POINTER && to->kind == XVR_KIND_POINTER) {
        return XVR_CAST_RESULT_OK;
    }

    if (from->kind == XVR_KIND_INTEGER && to->kind == XVR_KIND_POINTER) {
        if (!isExplicit) {
            return XVR_CAST_RESULT_IMPLICIT_NARROWING;
        }
        return XVR_CAST_RESULT_OK;
    }

    return XVR_CAST_RESULT_INVALID;
}

bool Xvr_CanImplicitCast(Xvr_Type* from, Xvr_Type* to) {
    Xvr_CastResult result = Xvr_CanCast(from, to, false);
    return result == XVR_CAST_RESULT_OK;
}

const char* Xvr_CastResultToString(Xvr_CastResult result) {
    switch (result) {
    case XVR_CAST_RESULT_OK:
        return "OK";
    case XVR_CAST_RESULT_INVALID:
        return "Invalid cast";
    case XVR_CAST_RESULT_IMPLICIT_NARROWING:
        return "Implicit narrowing not allowed";
    case XVR_CAST_RESULT_SIGN_CONVERSION:
        return "Sign conversion requires explicit cast";
    default:
        return "Unknown";
    }
}

void Xvr_SemanticErrorCreate(Xvr_SemanticAnalyzer* analyzer, const char* msg,
                             int line, int col) {
    if (!analyzer || analyzer->error_count >= 16) return;

    Xvr_SemanticError* err = &analyzer->errors[analyzer->error_count++];
    err->message = strdup(msg);
    err->line = line;
    err->column = col;
}

bool Xvr_SemanticAnalyzerHasErrors(const Xvr_SemanticAnalyzer* analyzer) {
    return analyzer && analyzer->error_count > 0;
}

const Xvr_SemanticError* Xvr_SemanticAnalyzerGetErrors(
    const Xvr_SemanticAnalyzer* analyzer, int* count) {
    if (!analyzer) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = analyzer->error_count;
    return analyzer->errors;
}

bool Xvr_SemanticAnalyze(Xvr_SemanticAnalyzer* analyzer, Xvr_ASTNode* node) {
    if (!node) return true;

    return true;
}

Xvr_ASTNode* Xvr_InsertImplicitCast(Xvr_ASTNode* expr, Xvr_Type* target_type) {
    if (!expr || !target_type) return expr;

    return expr;
}
