#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_value.h"

#include <stdio.h>
#include <string.h>

int test_sizeof_ast_64bit() {
#define TEST_SIZEOF(type, size)                                                \
  if (sizeof(type) != size) {                                                  \
    fprintf(stderr,                                                            \
            XVR_CC_ERROR "ERROR: sizeof(" #type                                \
                         ") is %d, expected %d\n" XVR_CC_RESET,                \
            (int)sizeof(type), size);                                          \
    ++err;                                                                     \
  }

  int err = 0;

  TEST_SIZEOF(Xvr_AstType, 4);
  TEST_SIZEOF(Xvr_AstBlock, 32);
  TEST_SIZEOF(Xvr_AstVarDeclare, 24);
  TEST_SIZEOF(Xvr_AstValue, 24);
  TEST_SIZEOF(Xvr_AstUnary, 16);
  TEST_SIZEOF(Xvr_AstBinary, 24);
  TEST_SIZEOF(Xvr_AstGroup, 16);
  TEST_SIZEOF(Xvr_AstPrint, 16);
  TEST_SIZEOF(Xvr_AstPass, 4);
  TEST_SIZEOF(Xvr_AstError, 4);
  TEST_SIZEOF(Xvr_AstEnd, 4);
  TEST_SIZEOF(Xvr_Ast, 32);
#undef TEST_SIZEOF

  return -err;
}

int test_sizeof_ast_32bit() {
#define TEST_SIZEOF(type, size)                                                \
  if (sizeof(type) != size) {                                                  \
    fprintf(stderr,                                                            \
            XVR_CC_ERROR "ERROR: sizeof(" #type                                \
                         ") is %d, expected %d\n" XVR_CC_RESET,                \
            (int)sizeof(type), size);                                          \
    ++err;                                                                     \
  }

  int err = 0;

  TEST_SIZEOF(Xvr_AstType, 4);
  TEST_SIZEOF(Xvr_AstBlock, 16);
  TEST_SIZEOF(Xvr_AstVarDeclare, 12);
  TEST_SIZEOF(Xvr_AstValue, 12);
  TEST_SIZEOF(Xvr_AstUnary, 12);
  TEST_SIZEOF(Xvr_AstBinary, 16);
  TEST_SIZEOF(Xvr_AstGroup, 8);
  TEST_SIZEOF(Xvr_AstPrint, 8);
  TEST_SIZEOF(Xvr_AstPass, 4);
  TEST_SIZEOF(Xvr_AstError, 4);
  TEST_SIZEOF(Xvr_AstEnd, 4);
  TEST_SIZEOF(Xvr_Ast, 16);

#undef TEST_SIZEOF

  return -err;
}

int test_type_emission(Xvr_Bucket **bucketHandle) {
  {
    Xvr_Ast *ast = NULL;
    Xvr_private_emitAstValue(bucketHandle, &ast, XVR_VALUE_FROM_INTEGER(42));

    if (ast == NULL || ast->type != XVR_AST_VALUE ||
        XVR_VALUE_AS_INTEGER(ast->value.value) != 42) {
      fprintf(stderr, XVR_CC_ERROR "Error: failed to emit a value as `Xvr_Ast` "
                                   "state unknown\n" XVR_CC_RESET);
      return -1;
    }
  }

  {
    Xvr_Ast *ast = NULL;
    Xvr_private_emitAstValue(bucketHandle, &ast, XVR_VALUE_FROM_INTEGER(42));
    Xvr_private_emitAstUnary(bucketHandle, &ast, XVR_AST_FLAG_NEGATE);

    if (ast == NULL || ast->type != XVR_AST_UNARY ||
        ast->unary.flag != XVR_AST_FLAG_NEGATE ||
        ast->unary.child->type != XVR_AST_VALUE ||
        XVR_VALUE_AS_INTEGER(ast->unary.child->value.value) != 42) {
      fprintf(stderr, XVR_CC_ERROR "ERROR: failed to emit a unary as "
                                   "`Xvr_Ast` state unknown\n" XVR_CC_RESET);
      return -1;
    }
  }

  {
    Xvr_Ast *block = NULL;
    Xvr_private_initAstBlock(bucketHandle, &block);

    for (int i = 0; i < 5; i++) {
      Xvr_Ast *ast = NULL;
      Xvr_Ast *right = NULL;
      Xvr_private_emitAstValue(bucketHandle, &ast, XVR_VALUE_FROM_INTEGER(42));
      Xvr_private_emitAstValue(bucketHandle, &right,
                               XVR_VALUE_FROM_INTEGER(69));
      Xvr_private_emitAstBinary(bucketHandle, &ast, XVR_AST_FLAG_ADD, right);
      Xvr_private_emitAstGroup(bucketHandle, &ast);

      Xvr_private_appendAstBlock(bucketHandle, block, ast);
    }

    Xvr_Ast *iter = block;

    while (iter != NULL) {
      if (iter->type != XVR_AST_BLOCK || iter->block.child == NULL ||
          iter->block.child->type != XVR_AST_GROUP ||
          iter->block.child->group.child == NULL ||
          iter->block.child->group.child->type != XVR_AST_BINARY ||
          iter->block.child->group.child->binary.flag != XVR_AST_FLAG_ADD ||
          iter->block.child->group.child->binary.left->type != XVR_AST_VALUE ||
          XVR_VALUE_AS_INTEGER(
              iter->block.child->group.child->binary.left->value.value) != 42 ||
          iter->block.child->group.child->binary.right->type != XVR_AST_VALUE ||
          XVR_VALUE_AS_INTEGER(
              iter->block.child->group.child->binary.right->value.value) !=
              69) {
        fprintf(stderr, XVR_CC_ERROR "ERROR: failed to emit a block as "
                                     "`Xvr_Ast` state unknown\n" XVR_CC_RESET);
        return -1;
      }

      iter = iter->block.next;
    }
  }

  {
    Xvr_Ast *ast = NULL;
    Xvr_String *name =
        Xvr_createNameStringLength(bucketHandle, "foobar", 6, XVR_VALUE_NULL);

    Xvr_private_emitAstVariableDeclaration(bucketHandle, &ast, name, NULL);

    if (ast == NULL || ast->type != XVR_AST_VAR_DECLARE ||

        ast->varDeclare.name == NULL ||
        ast->varDeclare.name->type != XVR_STRING_NAME ||
        strcmp(ast->varDeclare.name->as.name.data, "foobar") != 0 ||

        ast->varDeclare.expr != NULL) {
      fprintf(stderr, XVR_CC_ERROR "ERROR: failed to emit a var declare as "
                                   "`Xvr_Ast` state unknown\n" XVR_CC_RESET);
      Xvr_freeString(name);
      return -1;
    }

    Xvr_freeString(name);
  }

  return 0;
}

int main() {
  printf(XVR_CC_WARN "testing: xvr ast\n" XVR_CC_RESET);
  int total = 0, res = 0;

#if XVR_BITNESS == 64
  res = test_sizeof_ast_64bit();
  total += res;
  if (res == 0) {
    printf(XVR_CC_NOTICE
           "test_sizeof_ast_64bit(): aman loh ya cik\n" XVR_CC_RESET);
  }
#elif XVR_BITNESS == 32
  res = test_sizeof_ast_32bit();
  total += res;

  if (res == 0) {
    printf(XVR_CC_NOTICE "All good\n" XVR_CC_RESET);
  }

#else
  fprintf(stderr,
          XVR_CC_WARN
          "WARNING: Skipping test_sizeof_ast_*bit(); Can't determine the "
          "'bitness' of this platform (seems to be %d)\n" XVR_CC_RESET,
          XVR_BITNESS);

#endif /* if XVR_BITNESS == 64 */

  {
    Xvr_Bucket *bucketHandle = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    res = test_type_emission(&bucketHandle);
    Xvr_freeBucket(&bucketHandle);
    if (res == 0) {
      printf(XVR_CC_NOTICE "test_type_emission(): aman cik, gak bad mood loh "
                           "ya atmint\n" XVR_CC_RESET);
    }
    total += res;
  }
  return total;
}
