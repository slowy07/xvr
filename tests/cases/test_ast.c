#include "../../src/xvr_ast.h"
#include "../../src/xvr_console_color.h"

#include <stdio.h>

int test_sizeof_ast() {
#define TEST_SIZEOF(type, size)                                                \
  if (sizeof(type) != size) {                                                  \
    fprintf(stderr,                                                            \
            XVR_CC_ERROR "Error: sizeof(" #type ") are %d\n" XVR_CC_RESET,     \
            (int)sizeof(type), size);                                          \
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
  TEST_SIZEOF(Xvr_Ast, 16);
#undef TEST_SIZEOF
  return -err;
}

int main() {
  int total = 0, res = 0;
  res = test_sizeof_ast();
  total += res;

  if (res == 0) {
    printf(XVR_CC_NOTICE "everything was fine gass\n" XVR_CC_RESET);
  }

  return total;
}
