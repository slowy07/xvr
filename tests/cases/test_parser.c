#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_parser.h"
#include "xvr_string.h"
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

int test_keywords(Xvr_Bucket **bucketHandle) {
  {
    char *source = "assert true;";
    Xvr_Ast *ast = makeAstFromSource(bucketHandle, source);

    if (ast == NULL || ast->type != XVR_AST_BLOCK || ast->block.child == NULL ||
        ast->block.child->type != XVR_AST_ASSERT ||
        ast->block.child->assert.child == NULL ||
        ast->block.child->assert.child->type != XVR_AST_VALUE ||
        XVR_VALUE_IS_BOOLEAN(ast->block.child->assert.child->value.value) ==
            false ||
        XVR_VALUE_AS_BOOLEAN(ast->block.child->assert.child->value.value) !=
            true ||
        ast->block.child->assert.message != NULL) {
      fprintf(stderr,
              XVR_CC_ERROR
              "Error: failed to runt the parser, source '%s'\n" XVR_CC_RESET,
              source);
      return -1;
    }
  }

  {
    char *source = "print \"woilah cik\";";
    Xvr_Ast *ast = makeAstFromSource(bucketHandle, source);

    if (ast == NULL || ast->type != XVR_AST_BLOCK || ast->block.child == NULL ||
        ast->block.child->type != XVR_AST_PRINT ||
        ast->block.child->print.child == NULL ||
        ast->block.child->print.child->type != XVR_AST_VALUE ||
        XVR_VALUE_IS_STRING(ast->block.child->print.child->value.value) ==
            false ||
        XVR_VALUE_AS_STRING(ast->block.child->print.child->value.value)->type !=
            XVR_STRING_LEAF ||
        strncmp(XVR_VALUE_AS_STRING(ast->block.child->print.child->value.value)
                    ->as.leaf.data,
                "woilah cik", 10) != 0) {
      fprintf(stderr,
              XVR_CC_ERROR
              "Error: failed to run the parser, source: %s\n" XVR_CC_RESET,
              source);
      return -1;
    }
  }

  return 0;
}

int test_compound(Xvr_Bucket **bucketHandle) {
  {
    char *source = "1, 2, 3;";
    Xvr_Ast *ast = makeAstFromSource(bucketHandle, source);

    if (ast == NULL || ast->type != XVR_AST_BLOCK || ast->block.child == NULL ||

        ast->block.child->type != XVR_AST_COMPOUND ||
        ast->block.child->compound.flag != XVR_AST_FLAG_COMPOUND_COLLECTION ||

        ast->block.child->compound.left == NULL ||
        ast->block.child->compound.left->type != XVR_AST_VALUE ||
        XVR_VALUE_IS_INTEGER(ast->block.child->compound.left->value.value) !=
            true ||
        XVR_VALUE_AS_INTEGER(ast->block.child->compound.left->value.value) !=
            1 ||

        ast->block.child->compound.right == NULL ||
        ast->block.child->compound.right->type != XVR_AST_COMPOUND ||
        ast->block.child->compound.right->compound.flag !=
            XVR_AST_FLAG_COMPOUND_COLLECTION ||

        ast->block.child->compound.right->compound.left == NULL ||
        ast->block.child->compound.right->compound.left->type !=
            XVR_AST_VALUE ||
        XVR_VALUE_IS_INTEGER(
            ast->block.child->compound.right->compound.left->value.value) !=
            true ||
        XVR_VALUE_AS_INTEGER(
            ast->block.child->compound.right->compound.left->value.value) !=
            2 ||

        ast->block.child->compound.right->compound.right == NULL ||
        ast->block.child->compound.right->compound.right->type !=
            XVR_AST_VALUE ||
        XVR_VALUE_IS_INTEGER(
            ast->block.child->compound.right->compound.right->value.value) !=
            true ||
        XVR_VALUE_AS_INTEGER(
            ast->block.child->compound.right->compound.right->value.value) !=
            3 ||

        false) {

      fprintf(stderr,
              XVR_CC_ERROR
              "Error: failed to run the parser, source: %s\n" XVR_CC_RESET,
              source);
      return -1;
    }
  }

  return 0;
}

int main() {
  printf(XVR_CC_WARN "TESTING: XVR PARSER\n" XVR_CC_RESET);
  int total = 0, res = 0;

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(sizeof(Xvr_Ast) * 32);
    res = test_simple_empty_parsers(&bucket);
    Xvr_freeBucket(&bucket);

    if (res == 0) {
      printf(XVR_CC_NOTICE "PARSER: PASSED nice one reks\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    res = test_var_declare(&bucket);
    Xvr_freeBucket(&bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE "VAR DECLARE: PASSED nice one cik\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    res = test_keywords(&bucket);
    Xvr_freeBucket(&bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "KEYWORDS: PASSED woilah cik jalan loh ya\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    res = test_compound(&bucket);
    Xvr_freeBucket(&bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "COMPOUND: PASSED woilah cik jalan loh ya\n" XVR_CC_RESET);
    }
    total += res;
  }

  return total;
}
