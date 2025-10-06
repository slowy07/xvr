#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_bytecode.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_opcodes.h"
#include "xvr_parser.h"

#include <stdio.h>
#include <string.h>

int test_bytecode_header(Xvr_Bucket **bucketHandle) {
  {
    Xvr_Ast *ast = NULL;
    Xvr_private_emitAstPass(bucketHandle, &ast);
    Xvr_Bytecode bc = Xvr_compileBytecode(ast);

    if (bc.ptr[0] != XVR_VERSION_MAJOR || bc.ptr[1] != XVR_VERSION_MINOR ||
        bc.ptr[2] != XVR_VERSION_PATCH ||
        strcmp((char *)(bc.ptr + 3), XVR_VERSION_BUILD) != 0) {
      fprintf(
          stderr, XVR_CC_ERROR
          "Error: failed to write bytecode header corretly:\n" XVR_CC_RESET);
      fprintf(stderr, XVR_CC_ERROR "\t%d.%d.%d.%s\n" XVR_CC_RESET,
              (int)(bc.ptr[0]), (int)(bc.ptr[1]), (int)(bc.ptr[2]),
              (char *)(bc.ptr + 3));
      fprintf(stderr, XVR_CC_ERROR "\t%d.%d.%d.%s\n" XVR_CC_RESET,
              XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH,
              XVR_VERSION_BUILD);

      Xvr_freeBytecode(bc);
      return -1;
    }

    if (bc.count % 4 != 0) {
      fprintf(stderr,
              XVR_CC_ERROR "Error: bytecode size is not a multiple of 4, size "
                           "is: %d\n" XVR_CC_RESET,
              (int)bc.count);

      Xvr_freeBytecode(bc);
      return -1;
    }

    Xvr_freeBytecode(bc);
  }
  return 0;
}

int main() {
  printf(XVR_CC_WARN "testing: xvr bytecode\n" XVR_CC_RESET);
  int total = 0, res = 0;

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    res = test_bytecode_header(&bucket);
    Xvr_freeBucket(&bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "test_bytecode_header(): nice one cik\n" XVR_CC_RESET);
    }
    total += res;
  }

  return total;
}
