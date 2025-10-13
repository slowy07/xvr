#include "xvr_ast.h"
#include "xvr_bucket.h"
#include "xvr_bytecode.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_parser.h"
#include "xvr_stack.h"
#include "xvr_value.h"
#include "xvr_vm.h"
#include <stdio.h>
#include <string.h>

Xvr_Bytecode makeBytecodeFromSource(Xvr_Bucket **bucketHandle,
                                    const char *source) {
  Xvr_Lexer lexer;
  Xvr_bindLexer(&lexer, source);

  Xvr_Parser parser;
  Xvr_bindParser(&parser, &lexer);

  Xvr_Ast *ast = Xvr_scanParser(bucketHandle, &parser);
  Xvr_Bytecode bc = Xvr_compileBytecode(ast);

  return bc;
}

int test_setup_and_teardown(Xvr_Bucket **bucketHandle) {
  {
    const char *source = "(1 + 2) * (3 + 4);";

    Xvr_Lexer lexer;
    Xvr_bindLexer(&lexer, source);

    Xvr_Parser parser;
    Xvr_bindParser(&parser, &lexer);

    Xvr_Ast *ast = Xvr_scanParser(bucketHandle, &parser);

    Xvr_Bytecode bc = Xvr_compileBytecode(ast);

    Xvr_VM vm;
    Xvr_initVM(&vm);
    Xvr_bindVM(&vm, &bc);

    int headerSize = 3 + strlen(XVR_VERSION_BUILD) + 1;
    if (headerSize % 4 != 0) {
      headerSize += 4 - (headerSize % 4);
    }

    if (vm.module - bc.ptr != headerSize || vm.moduleSize != 72 ||
        vm.paramSize != 0 || vm.jumpsSize != 0 || vm.dataSize != 0 ||
        vm.subsSize != 0) {
      fprintf(stderr,
              XVR_CC_ERROR "ERROR: failed to setup and teadown Xvr_VM, source: "
                           "%s\n" XVR_CC_RESET,
              source);

      Xvr_freeVM(&vm);
      Xvr_freeBytecode(bc);
      return -1;
    }

    Xvr_freeVM(&vm);
    Xvr_freeBytecode(bc);
  }

  return 0;
}

int test_simple_execution(Xvr_Bucket **bucketHandle) {
  {
    const char *source = "(1 + 2) * (3 + 4) / (5 + 6);";

    Xvr_Lexer lexer;
    Xvr_bindLexer(&lexer, source);

    Xvr_Parser parser;
    Xvr_bindParser(&parser, &lexer);

    Xvr_Ast *ast = Xvr_scanParser(bucketHandle, &parser);

    Xvr_Bytecode bc = Xvr_compileBytecode(ast);

    Xvr_VM vm;
    Xvr_initVM(&vm);
    Xvr_bindVM(&vm, &bc);

    Xvr_runVM(&vm);

    if (vm.stack == NULL || vm.stack->count != 1 ||
        XVR_VALUE_IS_INTEGER(Xvr_peekStack(&vm.stack)) != true ||
        XVR_VALUE_AS_INTEGER(Xvr_peekStack(&vm.stack)) != 1) {
      fprintf(stderr,
              XVR_CC_ERROR
              "Error: Unexpected result in `Xvr_VM`, souce: %s\n" XVR_CC_RESET,
              source);
      Xvr_freeVM(&vm);
      Xvr_freeBytecode(bc);
      return -1;
    }
    Xvr_freeVM(&vm);
    Xvr_freeBytecode(bc);
  }
  return 0;
}

int main() {
  printf(XVR_CC_WARN "testing: xvr VM\n" XVR_CC_RESET);
  int total = 0, res = 0;

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    res = test_setup_and_teardown(&bucket);
    Xvr_freeBucket(&bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE "test_setup_and_teardown(): nice one rek jalan loh "
                           "ya cik\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    res = test_simple_execution(&bucket);
    Xvr_freeBucket(&bucket);
    if (res == 0) {
      printf(
          XVR_CC_NOTICE
          "test_simple_execution(): nice one cik jalan loh ya\n" XVR_CC_RESET);
    }
    total += res;
  }

  return total;
}
