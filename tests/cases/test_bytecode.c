#include "../../src/xvr_bytecode.h"
#include "../../src/xvr_console_color.h"

#include <stdio.h>
#include <string.h>

int test_bytecode_header(Xvr_Bucket *bucket) {
  {
    Xvr_Ast *ast = NULL;
    Xvr_private_emitAstPass(&bucket, &ast);

    Xvr_Bytecode bc = Xvr_compileByteCode(ast);

    if (bc.ptr[0] != XVR_VERSION_MAJOR || bc.ptr[1] != XVR_VERSION_MINOR ||
        bc.ptr[2] != XVR_VERSION_PATCH ||
        strcmp((char *)(bc.ptr + 3), XVR_VERSION_BUILD) != 0) {
      fprintf(
          stderr, XVR_CC_ERROR
          "error: failed to write bytecode header correctly\n" XVR_CC_RESET);
      Xvr_freeBytecode(bc);
      return -1;
    }

    Xvr_freeBytecode(bc);
  }
  return 0;
}

int main() {
  // TODO: fixing bytecode implementation
  int total = 0, res = 0;

  {
    Xvr_Bucket *bucket = NULL;
    XVR_BUCKET_INIT(Xvr_Ast, bucket, 32);
    res = test_bytecode_header(bucket);
    XVR_BUCKET_FREE(bucket);

    if (res == 0) {
      printf(XVR_CC_NOTICE "nice one gass\n" XVR_CC_RESET);
    }
    total += res;
  }
  return total;
}
