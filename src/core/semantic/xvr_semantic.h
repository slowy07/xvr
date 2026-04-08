/**
 * MIT License
 * Copyright (c) 2025 arfy slowy
 *
 * @file xvr_semantic.h
 * @brief Semantic analysis for XVR - type checking and cast validation
 */

#ifndef XVR_SEMANTIC_H
#define XVR_SEMANTIC_H

#include <stdbool.h>

#include "../ast/xvr_ast_node.h"
#include "../types/xvr_type.h"

typedef enum {
    XVR_CAST_RESULT_OK,
    XVR_CAST_RESULT_INVALID,
    XVR_CAST_RESULT_IMPLICIT_NARROWING,
    XVR_CAST_RESULT_SIGN_CONVERSION,
} Xvr_CastResult;

typedef struct Xvr_SemanticAnalyzer Xvr_SemanticAnalyzer;

Xvr_SemanticAnalyzer* Xvr_SemanticAnalyzerCreate(void);
void Xvr_SemanticAnalyzerDestroy(Xvr_SemanticAnalyzer* analyzer);

Xvr_CastResult Xvr_CanCast(Xvr_Type* from, Xvr_Type* to, bool isExplicit);
bool Xvr_CanImplicitCast(Xvr_Type* from, Xvr_Type* to);

Xvr_CastKind Xvr_GetCastKind(Xvr_Type* from, Xvr_Type* to);

bool Xvr_IsWidening(Xvr_Type* from, Xvr_Type* to);
bool Xvr_IsNarrowing(Xvr_Type* from, Xvr_Type* to);

bool Xvr_SemanticAnalyze(Xvr_SemanticAnalyzer* analyzer, Xvr_ASTNode* node);
Xvr_ASTNode* Xvr_InsertImplicitCast(Xvr_ASTNode* expr, Xvr_Type* target_type);

const char* Xvr_CastResultToString(Xvr_CastResult result);

typedef struct {
    const char* message;
    int line;
    int column;
} Xvr_SemanticError;

void Xvr_SemanticErrorCreate(Xvr_SemanticAnalyzer* analyzer, const char* msg,
                             int line, int col);
bool Xvr_SemanticAnalyzerHasErrors(const Xvr_SemanticAnalyzer* analyzer);
const Xvr_SemanticError* Xvr_SemanticAnalyzerGetErrors(
    const Xvr_SemanticAnalyzer* analyzer, int* count);

#endif
