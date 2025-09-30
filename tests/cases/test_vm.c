#include "xvr_bytecode.h"
#include "xvr_console_colors.h"
#include "xvr_lexer.h"
#include "xvr_parser.h"
#include "xvr_vm.h"
#include <stdio.h>
#include <string.h>

Xvr_Bytecode makeBytecodeFromSource(Xvr_Bucket **bucket, const char *source) {
  Xvr_Lexer lexer;
  Xvr_bindLexer(&lexer, source);
  Xvr_Parser parser;
  Xvr_bindParser(&parser, &lexer);

  Xvr_Ast *ast = Xvr_scanParser(bucket, &parser);
  Xvr_Bytecode bc = Xvr_compileBytecode(ast);

  return bc;
}

int test_setup_and_teardown(Xvr_Bucket **bucket) {
  {
    const char *source = "(1 + 2) * (3 + 4);";

    Xvr_Lexer lexer;
    Xvr_bindLexer(&lexer, source);

    Xvr_Parser parser;
    Xvr_bindParser(&parser, &lexer);

    Xvr_Ast *ast = Xvr_scanParser(bucket, &parser);
    Xvr_Bytecode bc = Xvr_compileBytecode(ast);

    Xvr_VM vm;
    Xvr_bindVM(&vm, bc.ptr);

    int headerSize = 3 + strlen(XVR_VERSION_BUILD) + 1;

    if (headerSize % 4 != 0) {
      headerSize += 4 - (headerSize % 4);
    }

    if (vm.routine - vm.bc != headerSize || vm.routineSize != 72 ||
        vm.paramCount != 0 || vm.jumpsCount != 0 || vm.dataCount != 0 ||
        vm.dataCount != 0 || vm.subsCount != 0) {
      fprintf(stderr,
              XVR_CC_ERROR
              "error: failed to setup Xvr_VM, source: %s\n" XVR_CC_RESET,
              source);
      Xvr_freeVM(&vm);
      return -1;
    }
    Xvr_freeVM(&vm);
  }
  return 0;
}

int main() {

  int total = 0, res = 0;

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(sizeof(Xvr_Ast) * 32);
    res = test_setup_and_teardown(&bucket);
    Xvr_freeBucket(&bucket);
    if (res == 0) {
      printf(XVR_CC_NOTICE "Xvr_allocateBucket(): nice one rek\n" XVR_CC_RESET);
    }
    total += res;
  }

  return total;
}
