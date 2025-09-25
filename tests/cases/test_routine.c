#include "../../src/xvr_console_color.h"
#include "../../src/xvr_routine.h"
#include <stdio.h>

int test_routine_header(Xvr_Bucket *bucket) {
  {
    Xvr_Ast *ast = NULL;
    Xvr_private_emitAstPass(&bucket, &ast);

    void *buffer = Xvr_compileRoutine(ast);
    int len = ((int *)buffer)[0];

    XVR_FREE_ARRAY(unsigned char, buffer, len);
  }

  return 0;
}

int main() {
  fprintf(stderr,
          XVR_CC_WARN "warn: routine test still working on\n" XVR_CC_RESET);

  int total = 0, res = 0;

  {
    Xvr_Bucket *bucket = NULL;
    XVR_BUCKET_INIT(Xvr_Ast, bucket, 32);
    res = test_routine_header(bucket);
    XVR_BUCKET_FREE(bucket);

    if (res == 0) {
      printf(XVR_CC_NOTICE "nice one gass\n" XVR_CC_RESET);
    }

    total += res;
  }

  return total;
}
