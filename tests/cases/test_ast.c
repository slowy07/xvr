#include "../../src/xvr_ast.h"
#include "../../src/xvr_console_color.h"

#include <stdio.h>

int test_sizeof_ast_32bit() {
#define TEST_SIZEOF(type, size)                                                \
  if (sizeof(type) != size) {                                                  \
    fprintf(stderr,                                                            \
            XVR_CC_ERROR "Error: sizeof(" #type                                \
                         ") is %d, expected %d\n" XVR_CC_RESET,                \
            (int)sizeof(type), (int)(size));                                   \
    ++err;                                                                     \
  }

  int err = 0;

  TEST_SIZEOF(Xvr_AstType, 4);
  TEST_SIZEOF(Xvr_AstBlock, 16);
  TEST_SIZEOF(Xvr_AstValue, 12);
  TEST_SIZEOF(Xvr_AstUnary, 12);
  TEST_SIZEOF(Xvr_AstBinary, 16);
  TEST_SIZEOF(Xvr_AstGroup, 8);
  TEST_SIZEOF(Xvr_AstPass, 4);
  TEST_SIZEOF(Xvr_AstError, 4);
  TEST_SIZEOF(Xvr_AstEnd, 4);
  TEST_SIZEOF(Xvr_Ast, 16);

#undef TEST_SIZEOF
  return -err;
}

int test_type_emission(Xvr_Bucket *bucket) {
  {
    Xvr_Bucket *bucket = NULL;
    XVR_BUCKET_INIT(Xvr_Ast, bucket, 8);

    Xvr_Ast *ast = NULL;
    Xvr_private_emitAstValue(&bucket, &ast, XVR_VALUE_TO_INTEGER(42));
  }

  {
    Xvr_Bucket *bucket = NULL;
    XVR_BUCKET_INIT(Xvr_Ast, bucket, 8);

    Xvr_Ast *ast = NULL;
    Xvr_Ast *child = NULL;

    Xvr_private_emitAstValue(&bucket, &child, XVR_VALUE_TO_INTEGER(42));
    Xvr_private_emitAstUnary(&bucket, &ast, XVR_AST_FLAG_NEGATE);

    if (ast == NULL || ast->type != XVR_AST_UNARY ||
        ast->unary.flag != XVR_AST_FLAG_NEGATE ||
        ast->unary.child->type != XVR_AST_VALUE) {
      fprintf(stderr, "Error: failed to emitting a unary as "
                      "`Xvr_Ast`, state unknown\n");
      return -1;
    }

    XVR_BUCKET_FREE(bucket);
  }

  {
    Xvr_Ast *ast = NULL;
    Xvr_Ast *right = NULL;
    Xvr_private_emitAstValue(&bucket, &ast, XVR_VALUE_TO_INTEGER(42));
    Xvr_private_emitAstValue(&bucket, &right, XVR_VALUE_TO_INTEGER(69));
    Xvr_private_emitAstBinary(&bucket, &ast, XVR_AST_FLAG_ADD, right);

    if (ast == NULL || ast->type != XVR_AST_BINARY ||
        ast->binary.flag != XVR_AST_FLAG_ADD ||
        ast->binary.left->type != XVR_AST_VALUE ||
        XVR_VALUE_AS_INTEGER(ast->binary.left->value.value) != 42 ||
        ast->binary.right->type != XVR_AST_VALUE ||
        XVR_VALUE_AS_INTEGER(ast->binary.right->value.value) != 69) {
      fprintf(stderr, XVR_CC_ERROR "error: failed to emitting binary as "
                                   "`Xvr_Ast`, state unknown\n" XVR_CC_RESET);
      return -1;
    }
  }

  return 0;
}

int main() {
  int total = 0, res = 0;

#if XVR_BITNESS == 32
  res = test_sizeof_ast_32bit();
  total += res;

  if (res == 0) {
    printf(XVR_CC_NOTICE "everything was fine gass\n" XVR_CC_RESET);
  }
#else
  fprintf(stderr,
          XVR_CC_WARN "WARNING: skip test_sizeof_ast_32bit();\n" XVR_CC_RESET);
#endif // XVR_BITNESS == 32

  {
    Xvr_Bucket *bucket = NULL;
    XVR_BUCKET_INIT(Xvr_Ast, bucket, 32);
    res = test_type_emission(bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE "everything is fine gass\n" XVR_CC_RESET);
    }
    total += res;
  }

  return total;
}
