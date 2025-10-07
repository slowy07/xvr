#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_parser.h"
#include "xvr_value.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

Xvr_Ast *makeAstFromSource(Xvr_Bucket **bucket, const char *source) {
  Xvr_Lexer lexer;
  Xvr_bindLexer(&lexer, source);

  Xvr_Parser parser;
  Xvr_bindParser(&parser, &lexer);

  return Xvr_scanParser(bucket, &parser);
}

int test_simple_empty_parsers(Xvr_Bucket **bucket) {
  {
    const char *source = "";
    Xvr_Lexer lexer;
    Xvr_bindLexer(&lexer, source);

    Xvr_Parser parser;
    Xvr_bindParser(&parser, &lexer);

    Xvr_Ast *ast = Xvr_scanParser(bucket, &parser);

    if (ast == NULL || ast->type != XVR_AST_END) {
      fprintf(stderr,
              XVR_CC_ERROR "test_parser(XVR_AST): woilah cik failed to run the "
                           "parser with empty source lho ya\n" XVR_CC_RESET);
      return -1;
    }
  }

  {
    const char *source = ";";
    Xvr_Lexer lexer;
    Xvr_bindLexer(&lexer, source);

    Xvr_Parser parser;
    Xvr_bindParser(&parser, &lexer);

    Xvr_Ast *ast = Xvr_scanParser(bucket, &parser);

    if (ast == NULL || ast->type != XVR_AST_BLOCK || ast->block.child == NULL ||
        ast->block.child->type != XVR_AST_PASS) {
      fprintf(stderr,
              XVR_CC_ERROR "test_parser(XVR_AST):woilah cik failed to run the "
                           "parser with one semicolon\n" XVR_CC_RESET);
      return -1;
    }
  }

  {
    const char *source = ";;;;;";
    Xvr_Lexer lexer;
    Xvr_bindLexer(&lexer, source);

    Xvr_Parser parser;
    Xvr_bindParser(&parser, &lexer);

    Xvr_Ast *ast = Xvr_scanParser(bucket, &parser);
    Xvr_Ast *iter = ast;

    while (iter != NULL) {
      if (iter == NULL || iter->type != XVR_AST_BLOCK ||
          iter->block.child == NULL ||
          iter->block.child->type != XVR_AST_PASS) {
        fprintf(stderr, XVR_CC_ERROR
                "test_parser(XVR_AST): woilah cik failed to run the parser "
                "with multiple semicolon\n" XVR_CC_RESET);
        return -1;
      }
      iter = iter->block.next;
    }
  }

  return 0;
}

int test_var_declare(Xvr_Bucket **bucketHandle) {
  {
    const char *source = "var hasil = 42;";
    Xvr_Ast *ast = makeAstFromSource(bucketHandle, source);

    if (ast == NULL || ast->type != XVR_AST_BLOCK || ast->block.child == NULL ||
        ast->block.child->type != XVR_AST_VAR_DECLARE ||
        strcmp(ast->block.child->varDeclare.name->as.name.data, "hasil") != 0 ||
        ast->block.child->varDeclare.expr == NULL ||
        ast->block.child->varDeclare.expr->type != XVR_AST_VALUE ||
        XVR_VALUE_IS_INTEGER(ast->block.child->varDeclare.expr->value.value) ==
            false ||
        XVR_VALUE_AS_INTEGER(ast->block.child->varDeclare.expr->value.value) !=
            42) {
      fprintf(stderr,
              XVR_CC_ERROR
              "Error: declare AST failed: source %s\n" XVR_CC_RESET,
              source);
      return -1;
    }
  }

  return 0;
}

int test_var_assign(Xvr_Bucket **bucketHandle) {
#define TEST_VAR_ASSIGN(ARG_SOURCE, ARG_FLAG, ARG_NAME, ARG_VALUE)             \
  {                                                                            \
    const char *source = ARG_SOURCE;                                           \
    Xvr_Ast *ast = makeAstFromSource(bucketHandle, source);                    \
    if (ast == NULL || ast->type != XVR_AST_BLOCK ||                           \
        ast->block.child == NULL ||                                            \
        ast->block.child->type != XVR_AST_VAR_ASSIGN ||                        \
        ast->block.child->varAssign.flag != ARG_FLAG ||                        \
        ast->block.child->varAssign.name == NULL ||                            \
        ast->block.child->varAssign.name->type != XVR_STRING_NAME ||           \
        strcmp(ast->block.child->varAssign.name->as.name.data, ARG_NAME) !=    \
            0 ||                                                               \
        ast->block.child->varAssign.expr == NULL ||                            \
        ast->block.child->varAssign.expr->type != XVR_AST_VALUE ||             \
        XVR_VALUE_IS_INTEGER(ast->block.child->varAssign.expr->value.value) == \
            false ||                                                           \
        XVR_VALUE_AS_INTEGER(ast->block.child->varAssign.expr->value.value) != \
            ARG_VALUE) {                                                       \
      fprintf(stderr,                                                          \
              XVR_CC_ERROR                                                     \
              "Error: assign ast failed, source %s\n" XVR_CC_RESET,            \
              source);                                                         \
    }                                                                          \
  }
  return -1;

  TEST_VAR_ASSIGN("hasil = 42;", XVR_AST_FLAG_ASSIGN, "hasil", 42);

  return 0;
}

int main() {
  printf(XVR_CC_WARN "testing: xvr parser\n" XVR_CC_RESET);
  int total = 0, res = 0;

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(sizeof(Xvr_Ast) * 32);
    res = test_simple_empty_parsers(&bucket);
    Xvr_freeBucket(&bucket);

    if (res == 0) {
      printf(XVR_CC_NOTICE "test_parser(): nice one reks\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    res = test_var_declare(&bucket);
    Xvr_freeBucket(&bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE "test_var_declare(): nice one cik\n" XVR_CC_RESET);
    }
    total += res;
  }

  // {
  //   Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
  //   res = test_var_assign(&bucket);
  //   Xvr_freeBucket(&bucket);
  //   if (res == 0) {
  //     printf(XVR_CC_NOTICE "test_var_assign(): jalan loh ya\n" XVR_CC_RESET);
  //   }
  //   total += res;
  // }

  return total;
}
