#include "../../src/xvr_console_color.h"
#include "../../src/xvr_parser.h"
#include <stdio.h>

Xvr_Ast *makeAstFromSource(Xvr_Bucket **bucket, const char *source) {
  Xvr_lexer lexer;
  Xvr_bindLexer(&lexer, source);

  Xvr_Parser parser;
  Xvr_bindParser(&parser, &lexer);

  return Xvr_scanParser(bucket, &parser);
}

int test_values(Xvr_Bucket *bucket) {
  {
    Xvr_Ast *ast = makeAstFromSource(&bucket, "true;");

    if (ast == NULL || ast->type != XVR_AST_BLOCK || ast->block.child == NULL ||
        ast->block.child->type != XVR_AST_VALUE ||
        XVR_VALUE_IS_BOOLEAN(ast->block.child->value.value) == false ||
        XVR_VALUE_AS_BOOLEAN(ast->block.child->value.value) != true) {
      fprintf(stderr, XVR_CC_ERROR "error: failed to running parser with "
                                   "boolean value true\n" XVR_CC_RESET);
      return -1;
    } else {
      printf("just debug\n");
    }
  }

  {
    Xvr_Ast *ast = makeAstFromSource(&bucket, "false;");

    if (ast == NULL || ast->type != XVR_AST_BLOCK || ast->block.child == NULL ||
        ast->block.child->type != XVR_AST_VALUE ||
        XVR_VALUE_IS_BOOLEAN(ast->block.child->value.value) == false ||
        XVR_VALUE_AS_BOOLEAN(ast->block.child->value.value) != false) {
      fprintf(stderr, XVR_CC_ERROR "error: failed to running parser with "
                                   "boolean value false\n" XVR_CC_RESET);
      return -1;
    }
  }

  return 0;
}

int main() {
  int total = 0, res = 0;

  {
    Xvr_Bucket *bucket = NULL;
    XVR_BUCKET_INIT(Xvr_Ast, bucket, 32);
    res = test_values(bucket);
    XVR_BUCKET_FREE(bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "parser test_value(bucket) nice one reks\n" XVR_CC_RESET);
    }
    total += res;
  }

  return 0;
}
