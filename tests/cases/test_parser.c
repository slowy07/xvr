#include "xvr_console_colors.h"
#include "xvr_parser.h"

#include <stddef.h>
#include <stdio.h>

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

int main() {
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

  return total;
}
