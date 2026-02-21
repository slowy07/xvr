#include "xvr_unused.h"

#include <stdio.h>

#include "xvr_console_colors.h"
#include "xvr_literal.h"
#include "xvr_memory.h"
#include "xvr_refstring.h"

static void pushScope(Xvr_UnusedChecker* checker) {
    if (checker->scopeCount >= checker->scopeCapacity) {
        int oldCap = checker->scopeCapacity;
        checker->scopeCapacity = XVR_GROW_CAPACITY(oldCap);
        checker->scopes = XVR_GROW_ARRAY(Xvr_UnusedScope, checker->scopes,
                                         oldCap, checker->scopeCapacity);
    }
    Xvr_UnusedScope* scope = &checker->scopes[checker->scopeCount++];
    scope->declarations = NULL;
    scope->count = 0;
    scope->capacity = 0;
}

static void popScope(Xvr_UnusedChecker* checker) {
    if (checker->scopeCount <= 0) return;

    Xvr_UnusedScope* scope = &checker->scopes[checker->scopeCount - 1];

    for (int i = 0; i < scope->count; i++) {
        if (!scope->declarations[i].used) {
            checker->hasError = true;
            const char* name = Xvr_toCString(
                scope->declarations[i].identifier.as.identifier.ptr);
            const char* kind =
                scope->declarations[i].isFunction ? "procedure" : "variable";
            fprintf(stderr,
                    XVR_CC_ERROR
                    "error: unused %s '%s' declared at line %d\n" XVR_CC_RESET,
                    kind, name, scope->declarations[i].line);
        }
        Xvr_freeLiteral(scope->declarations[i].identifier);
    }

    XVR_FREE_ARRAY(Xvr_UnusedDecl, scope->declarations, scope->capacity);
    checker->scopeCount--;
}

static void addDeclaration(Xvr_UnusedChecker* checker, Xvr_Literal identifier,
                           int line, bool isFunction) {
    if (checker->scopeCount <= 0) return;

    Xvr_UnusedScope* scope = &checker->scopes[checker->scopeCount - 1];

    if (scope->count >= scope->capacity) {
        int oldCap = scope->capacity;
        scope->capacity = XVR_GROW_CAPACITY(oldCap);
        scope->declarations = XVR_GROW_ARRAY(
            Xvr_UnusedDecl, scope->declarations, oldCap, scope->capacity);
    }

    Xvr_UnusedDecl* decl = &scope->declarations[scope->count++];
    decl->identifier = identifier;
    Xvr_copyRefString(identifier.as.identifier.ptr);
    decl->line = line;
    decl->used = false;
    decl->isFunction = isFunction;
}

static void markUsed(Xvr_UnusedChecker* checker, Xvr_Literal identifier) {
    if (!XVR_IS_IDENTIFIER(identifier)) {
        return;
    }

    for (int s = checker->scopeCount - 1; s >= 0; s--) {
        Xvr_UnusedScope* scope = &checker->scopes[s];
        for (int i = 0; i < scope->count; i++) {
            if (Xvr_equalsRefString(
                    scope->declarations[i].identifier.as.identifier.ptr,
                    identifier.as.identifier.ptr)) {
                scope->declarations[i].used = true;
                return;
            }
        }
    }
}

static void checkNode(Xvr_UnusedChecker* checker, Xvr_ASTNode* node);

static void checkBlock(Xvr_UnusedChecker* checker, Xvr_ASTNode* node) {
    pushScope(checker);
    for (int i = 0; i < node->block.count; i++) {
        checkNode(checker, &node->block.nodes[i]);
    }
    popScope(checker);
}

static void checkNode(Xvr_UnusedChecker* checker, Xvr_ASTNode* node) {
    if (!node) return;

    switch (node->type) {
    case XVR_AST_NODE_ERROR:
        break;

    case XVR_AST_NODE_LITERAL:
        if (XVR_IS_IDENTIFIER(node->atomic.literal)) {
            markUsed(checker, node->atomic.literal);
        }
        break;

    case XVR_AST_NODE_UNARY:
        checkNode(checker, node->unary.child);
        break;

    case XVR_AST_NODE_BINARY:
        checkNode(checker, node->binary.left);
        checkNode(checker, node->binary.right);
        break;

    case XVR_AST_NODE_TERNARY:
        checkNode(checker, node->ternary.condition);
        checkNode(checker, node->ternary.thenPath);
        checkNode(checker, node->ternary.elsePath);
        break;

    case XVR_AST_NODE_GROUPING:
        checkNode(checker, node->grouping.child);
        break;

    case XVR_AST_NODE_BLOCK:
        checkBlock(checker, node);
        break;

    case XVR_AST_NODE_COMPOUND:
        for (int i = 0; i < node->compound.count; i++) {
            checkNode(checker, &node->compound.nodes[i]);
        }
        break;

    case XVR_AST_NODE_PAIR:
        checkNode(checker, node->pair.left);
        checkNode(checker, node->pair.right);
        break;

    case XVR_AST_NODE_INDEX:
        checkNode(checker, node->index.first);
        checkNode(checker, node->index.second);
        checkNode(checker, node->index.third);
        break;

    case XVR_AST_NODE_VAR_DECL: {
        checkNode(checker, node->varDecl.expression);
        addDeclaration(checker, node->varDecl.identifier, node->varDecl.line,
                       false);
    } break;

    case XVR_AST_NODE_FN_DECL: {
        addDeclaration(checker, node->fnDecl.identifier, node->fnDecl.line,
                       true);

        pushScope(checker);

        if (node->fnDecl.arguments) {
            for (int i = 0; i < node->fnDecl.arguments->fnCollection.count;
                 i++) {
                Xvr_ASTNode* arg =
                    &node->fnDecl.arguments->fnCollection.nodes[i];
                if (arg->type == XVR_AST_NODE_VAR_DECL) {
                    addDeclaration(checker, arg->varDecl.identifier,
                                   arg->varDecl.line, false);
                }
            }
        }

        if (node->fnDecl.returns) {
            checkNode(checker, node->fnDecl.returns);
        }

        checkNode(checker, node->fnDecl.block);

        popScope(checker);
    } break;

    case XVR_AST_NODE_FN_COLLECTION:
        for (int i = 0; i < node->fnCollection.count; i++) {
            checkNode(checker, &node->fnCollection.nodes[i]);
        }
        break;

    case XVR_AST_NODE_FN_CALL:
        if (node->fnCall.arguments) {
            for (int i = 0; i < node->fnCall.arguments->fnCollection.count;
                 i++) {
                checkNode(checker,
                          &node->fnCall.arguments->fnCollection.nodes[i]);
            }
        }
        break;

    case XVR_AST_NODE_FN_RETURN:
        if (node->returns.returns) {
            for (int i = 0; i < node->returns.returns->fnCollection.count;
                 i++) {
                checkNode(checker,
                          &node->returns.returns->fnCollection.nodes[i]);
            }
        }
        break;

    case XVR_AST_NODE_IF:
        checkNode(checker, node->pathIf.condition);
        checkNode(checker, node->pathIf.thenPath);
        checkNode(checker, node->pathIf.elsePath);
        break;

    case XVR_AST_NODE_WHILE:
        checkNode(checker, node->pathWhile.condition);
        checkNode(checker, node->pathWhile.thenPath);
        break;

    case XVR_AST_NODE_FOR: {
        pushScope(checker);
        checkNode(checker, node->pathFor.preClause);
        checkNode(checker, node->pathFor.condition);
        checkNode(checker, node->pathFor.postClause);
        checkNode(checker, node->pathFor.thenPath);
        popScope(checker);
    } break;

    case XVR_AST_NODE_BREAK:
    case XVR_AST_NODE_CONTINUE:
    case XVR_AST_NODE_PASS:
        break;

    case XVR_AST_NODE_PREFIX_INCREMENT:
    case XVR_AST_NODE_PREFIX_DECREMENT:
    case XVR_AST_NODE_POSTFIX_INCREMENT:
    case XVR_AST_NODE_POSTFIX_DECREMENT: {
        Xvr_Literal identifier;
        if (node->type == XVR_AST_NODE_PREFIX_INCREMENT)
            identifier = node->prefixIncrement.identifier;
        else if (node->type == XVR_AST_NODE_PREFIX_DECREMENT)
            identifier = node->prefixDecrement.identifier;
        else if (node->type == XVR_AST_NODE_POSTFIX_INCREMENT)
            identifier = node->postfixIncrement.identifier;
        else
            identifier = node->postfixDecrement.identifier;
        markUsed(checker, identifier);
    } break;

    case XVR_AST_NODE_IMPORT:
        break;
    }
}

void Xvr_initUnusedChecker(Xvr_UnusedChecker* checker) {
    checker->scopes = NULL;
    checker->scopeCount = 0;
    checker->scopeCapacity = 0;
    checker->hasError = false;
}

void Xvr_freeUnusedChecker(Xvr_UnusedChecker* checker) {
    while (checker->scopeCount > 0) {
        popScope(checker);
    }
    XVR_FREE_ARRAY(Xvr_UnusedScope, checker->scopes, checker->scopeCapacity);
    checker->scopes = NULL;
    checker->scopeCount = 0;
    checker->scopeCapacity = 0;
}

void Xvr_checkUnusedBegin(Xvr_UnusedChecker* checker) { pushScope(checker); }

void Xvr_checkUnusedNode(Xvr_UnusedChecker* checker, Xvr_ASTNode* node) {
    if (!node) return;
    checkNode(checker, node);
}

bool Xvr_checkUnusedEnd(Xvr_UnusedChecker* checker) {
    popScope(checker);
    return !checker->hasError;
}
