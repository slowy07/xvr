#include "../../src/xvr_console_color.h"
#include "../../src/xvr_lexer.h"
#include "../../src/xvr_parser.h"
#include "../../src/xvr_routine.h"
#include <stdio.h>

int test_routine_header(Xvr_Bucket *bucket) {
  {
    Xvr_Ast *ast = NULL;
    Xvr_private_emitAstPass(&bucket, &ast);

    void *buffer = Xvr_compileRoutine(ast);
    int len = ((int *)buffer)[0];

    int *ptr = (int *)buffer;

    if ((ptr++)[0] != 28 || (ptr++)[0] != 0 || (ptr++)[0] != 0 ||
        (ptr++)[0] != 0 || (ptr++)[0]) {
      fprintf(stderr,
              XVR_CC_ERROR "error: failed to producing expected routine "
                           "gheader, and AST: pass\n" XVR_CC_RESET);
      XVR_FREE_ARRAY(unsigned char, buffer, len);
      return -1;
    }
    XVR_FREE_ARRAY(unsigned char, buffer, len);
  }

  {
    const char *source = ";";
    Xvr_Lexer lexer;
    Xvr_Parser parser;

    Xvr_bindLexer(&lexer, source);
    Xvr_Ast *ast = Xvr_scanParser(&bucket, &parser);

    void *buffer = Xvr_compileRoutine(ast);
    int len = ((int *)buffer)[0];

    int *ptr = (int *)buffer;

    if ((ptr++)[0] != 28 || (ptr++)[0] != 0 || (ptr++)[0] != 0 ||
        (ptr++)[0] != 0 || (ptr++)[0]) {
      fprintf(stderr,
              XVR_CC_ERROR "error: failed to producing expected routine "
                           "herader, source: %s\n" XVR_CC_RESET,
              source);
      XVR_FREE_ARRAY(unsigned char, buffer, len);
      return -1;
    }
    XVR_FREE_ARRAY(unsigned char, buffer, len);
  }

  return 0;
}

int test_routine_binary(Xvr_Bucket *bucket) {
  {
    const char *source = "3 + 5;";
    Xvr_Lexer lexer;
    Xvr_Parser parser;

    Xvr_bindLexer(&lexer, source);
    Xvr_bindParser(&parser, &lexer);
    Xvr_Ast *ast = Xvr_scanParser(&bucket, &parser);

    void *buffer = Xvr_compileRoutine(ast);
    int len = ((int *)buffer)[0];

    int *ptr = (int *)buffer;

    if ((ptr++)[0] != 48 || (ptr++)[0] != 0 || (ptr++)[0] != 0 ||
        (ptr++)[0] != 0 || (ptr++)[0]) {
      fprintf(stderr,
              XVR_CC_ERROR "error: failed to producing expected routine "
                           "header, source: %s\n" XVR_CC_RESET,
              source);

      XVR_FREE_ARRAY(unsigned char, buffer, len);
      return -1;
    }

    XVR_FREE_ARRAY(unsigned char, buffer, len);
  }

  {
    const char *source = "3 == 5;";
    Xvr_Lexer lexer;
    Xvr_Parser parser;

    Xvr_bindLexer(&lexer, source);
    Xvr_bindParser(&parser, &lexer);
    Xvr_Ast *ast = Xvr_scanParser(&bucket, &parser);

    void *buffer = Xvr_compileRoutine(ast);
    int len = ((int *)buffer)[0];

    int *ptr = (int *)buffer;
    if ((ptr++)[0] != 48 || (ptr++)[0] != 0 || (ptr++)[0] != 0 ||
        (ptr++)[0] != 0 || (ptr++)[0]) {
      fprintf(stderr,
              XVR_CC_ERROR "error: failed to producing expected rotine header, "
                           "source: %s\n" XVR_CC_RESET,
              source);
      XVR_FREE_ARRAY(unsigned char, buffer, len);
      return -1;
    }
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
    } else {
      printf(XVR_CC_ERROR "something wrong with you code\n" XVR_CC_RESET);
    }

    total += res;
  }

  {
    Xvr_Bucket *bucket = NULL;
    XVR_BUCKET_INIT(Xvr_Ast, bucket, 32);
    res = test_routine_binary(bucket);
    XVR_BUCKET_FREE(bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE "nice one gass\n" XVR_CC_RESET);
    }
    total += res;
  }

  return total;
}
