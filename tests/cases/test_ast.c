#include <stdio.h>
#include <string.h>

#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_string.h"
#include "xvr_value.h"

#if XVR_BITNESS == 32
#    define TEST_SIZEOF(type, bit32, bit64)                              \
        if (sizeof(type) != bit32) {                                     \
            fprintf(stderr,                                              \
                    XVR_CC_ERROR                                         \
                    "ERROR: sizeof(" #type                               \
                    " ) is %d, expected %d (bitness %d)\n" XVR_CC_RESET, \
                    (int)sizeof(type), bit32, XVR_BITNESS);              \
        }
#elif XVR_BITNESS == 64
#    define TEST_SIZEOF(type, bit32, bit64)                             \
        if (sizeof(type) != bit64) {                                    \
            fprintf(stderr,                                             \
                    XVR_CC_ERROR                                        \
                    "ERROR: sizeof(" #type                              \
                    ") is %d, expected %d (bitness %d)\n" XVR_CC_RESET, \
                    (int)sizeof(type), bit32, XVR_BITNESS);             \
        }
#else
#    pragma message( \
        "unable to test sizeo fo Xvr_Ast members, as XVR_BITNESS is not recognized")
#    define TEST_SIZEOF(type, bit32, bit64)
#endif /* if XVR_BITNESS == 32 */

int test_sizeof_ast(void) {
    int err = 0;

    TEST_SIZEOF(Xvr_AstType, 4, 4);
    TEST_SIZEOF(Xvr_AstBlock, 20, 32);
    TEST_SIZEOF(Xvr_AstValue, 12, 24);
    TEST_SIZEOF(Xvr_AstUnary, 12, 16);
    TEST_SIZEOF(Xvr_AstBinary, 16, 24);
    TEST_SIZEOF(Xvr_AstBinaryShortCircuit, 16, 24);
    TEST_SIZEOF(Xvr_AstCompare, 16, 24);
    TEST_SIZEOF(Xvr_AstGroup, 8, 16);
    TEST_SIZEOF(Xvr_AstCompound, 12, 16);
    TEST_SIZEOF(Xvr_AstAggregate, 16, 24);
    TEST_SIZEOF(Xvr_AstAssert, 12, 24);
    TEST_SIZEOF(Xvr_AstIfThenElse, 16, 32);
    TEST_SIZEOF(Xvr_AstWhileThen, 12, 24);
    TEST_SIZEOF(Xvr_AstBreak, 4, 4);
    TEST_SIZEOF(Xvr_AstContinue, 4, 4);
    TEST_SIZEOF(Xvr_AstPrint, 8, 16);
    TEST_SIZEOF(Xvr_AstVarDeclare, 12, 24);
    TEST_SIZEOF(Xvr_AstVarAssign, 16, 24);
    TEST_SIZEOF(Xvr_AstVarAccess, 8, 16);
    TEST_SIZEOF(Xvr_AstFnDeclare, 16, 32);
    TEST_SIZEOF(Xvr_AstPass, 4, 4);
    TEST_SIZEOF(Xvr_AstError, 4, 4);
    TEST_SIZEOF(Xvr_AstEnd, 4, 4);
    TEST_SIZEOF(Xvr_Ast, 20, 32)
    return -err;
}

int test_type_emission(Xvr_Bucket** bucketHandle) {
    {
        Xvr_Ast* ast = NULL;
        Xvr_private_emitAstValue(bucketHandle, &ast,
                                 XVR_VALUE_FROM_INTEGER(42));

        if (ast == NULL || ast->type != XVR_AST_VALUE ||
            XVR_VALUE_AS_INTEGER(ast->value.value) != 42) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed to emit a value as `Xvr_Ast` "
                    "state unknown\n" XVR_CC_RESET);
            return -1;
        }
    }

    {
        Xvr_Ast* ast = NULL;
        Xvr_private_emitAstValue(bucketHandle, &ast,
                                 XVR_VALUE_FROM_INTEGER(42));
        Xvr_private_emitAstUnary(bucketHandle, &ast, XVR_AST_FLAG_NEGATE);

        if (ast == NULL || ast->type != XVR_AST_UNARY ||
            ast->unary.flag != XVR_AST_FLAG_NEGATE ||
            ast->unary.child->type != XVR_AST_VALUE ||
            XVR_VALUE_AS_INTEGER(ast->unary.child->value.value) != 42) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: failed to emit a unary as "
                    "`Xvr_Ast` state unknown\n" XVR_CC_RESET);
            return -1;
        }
    }

    {
        Xvr_Ast* block = NULL;
        Xvr_private_initAstBlock(bucketHandle, &block);

        for (int i = 0; i < 5; i++) {
            Xvr_Ast* ast = NULL;
            Xvr_Ast* right = NULL;
            Xvr_private_emitAstValue(bucketHandle, &ast,
                                     XVR_VALUE_FROM_INTEGER(42));
            Xvr_private_emitAstValue(bucketHandle, &right,
                                     XVR_VALUE_FROM_INTEGER(69));
            Xvr_private_emitAstBinary(bucketHandle, &ast, XVR_AST_FLAG_ADD,
                                      right);
            Xvr_private_emitAstGroup(bucketHandle, &ast);

            Xvr_private_appendAstBlock(bucketHandle, block, ast);
        }

        Xvr_Ast* iter = block;

        while (iter != NULL) {
            if (iter->type != XVR_AST_BLOCK || iter->block.child == NULL ||
                iter->block.child->type != XVR_AST_GROUP ||
                iter->block.child->group.child == NULL ||
                iter->block.child->group.child->type != XVR_AST_BINARY ||
                iter->block.child->group.child->binary.flag !=
                    XVR_AST_FLAG_ADD ||
                iter->block.child->group.child->binary.left->type !=
                    XVR_AST_VALUE ||
                XVR_VALUE_AS_INTEGER(
                    iter->block.child->group.child->binary.left->value.value) !=
                    42 ||
                iter->block.child->group.child->binary.right->type !=
                    XVR_AST_VALUE ||
                XVR_VALUE_AS_INTEGER(iter->block.child->group.child->binary
                                         .right->value.value) != 69) {
                fprintf(stderr, XVR_CC_ERROR
                        "ERROR: failed to emit a block as "
                        "`Xvr_Ast` state unknown\n" XVR_CC_RESET);
                return -1;
            }

            iter = iter->block.next;
        }
    }

    {
        Xvr_Ast* ast = NULL;
        Xvr_String* name = Xvr_createNameStringLength(bucketHandle, "foobar", 6,
                                                      XVR_VALUE_ANY, false);

        Xvr_private_emitAstVariableDeclaration(bucketHandle, &ast, name, NULL);

        if (ast == NULL || ast->type != XVR_AST_VAR_DECLARE ||

            ast->varDeclare.name == NULL ||
            ast->varDeclare.name->info.type != XVR_STRING_NAME ||
            strcmp(ast->varDeclare.name->name.data, "foobar") != 0 ||

            ast->varDeclare.expr != NULL) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: failed to emit a var declare as "
                    "`Xvr_Ast` state unknown\n" XVR_CC_RESET);
            Xvr_freeString(name);
            return -1;
        }

        Xvr_freeString(name);
    }

    {
        Xvr_Ast* ast = NULL;
        Xvr_Ast* right = NULL;
        Xvr_private_emitAstValue(bucketHandle, &ast,
                                 XVR_VALUE_FROM_INTEGER(42));
        Xvr_private_emitAstValue(bucketHandle, &right,
                                 XVR_VALUE_FROM_INTEGER(69));
        Xvr_private_emitAstCompare(bucketHandle, &ast, XVR_AST_FLAG_ADD, right);

        if (ast == NULL || ast->type != XVR_AST_COMPARE ||
            ast->compare.flag != XVR_AST_FLAG_ADD ||
            ast->compare.left->type != XVR_AST_VALUE ||
            XVR_VALUE_AS_INTEGER(ast->compare.left->value.value) != 42 ||
            ast->compare.right->type != XVR_AST_VALUE ||
            XVR_VALUE_AS_INTEGER(ast->compare.right->value.value) != 69) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed to emiitting compare as "
                    "`Xvr_Ast`, state unknown\n" XVR_CC_RESET);
            return -1;
        }
    }

    {
        Xvr_Ast* ast = NULL;
        Xvr_String* name = Xvr_createNameStringLength(
            bucketHandle, "woilah cik", 10, XVR_VALUE_ANY, false);

        Xvr_private_emitAstVariableDeclaration(bucketHandle, &ast, name, NULL);

        if (ast == NULL || ast->type != XVR_AST_VAR_DECLARE ||
            ast->varDeclare.name == NULL ||
            ast->varDeclare.name->info.type != XVR_STRING_NAME ||
            strcmp(ast->varDeclare.name->name.data, "woilah cik") != 0 ||
            ast->varDeclare.expr != NULL) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed to emit var declare as 'Xvr_Ast', state "
                    "unknown cik\n" XVR_CC_RESET);
            Xvr_freeString(name);
            return -1;
        }
        Xvr_freeString(name);
    }

    return 0;
}

int main(void) {
    printf(XVR_CC_WARN "TESTING: XVR AST\n" XVR_CC_RESET);
    int total = 0, res = 0;

    {
        res = test_sizeof_ast();
        if (res == 0) {
            printf(
                XVR_CC_NOTICE
                "SIZEOF AST: woilah cik aman loh test sizeof\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        Xvr_Bucket* bucketHandle = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        res = test_type_emission(&bucketHandle);
        Xvr_freeBucket(&bucketHandle);
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "TYPE EMISSION: PASSED aman cik atmint\n" XVR_CC_RESET);
        }
        total += res;
    }
    return total;
}
