#include "../../src/xvr_console_color.h"
#include "../../src/xvr_lexer.h"
#include <stdio.h>
#include <string.h>

int main() {
  {
    char *source = "print null";

    Xvr_Lexer lexer;

    Xvr_bindLexer(&lexer, source);

    Xvr_Token print = Xvr_private_scanLexer(&lexer);
    Xvr_Token null = Xvr_private_scanLexer(&lexer);
    Xvr_Token semi = Xvr_private_scanLexer(&lexer);
    Xvr_Token eof = Xvr_private_scanLexer(&lexer);

    if (strncmp(print.lexeme, "print", print.length)) {
      fprintf(stderr, XVR_CC_ERROR "ERROR: print lexeme error: %s" XVR_CC_RESET,
              print.lexeme);
      return -1;
    }

    if (strncmp(null.lexeme, "null", null.length)) {
      fprintf(stderr, XVR_CC_ERROR "ERROR: null lexeme error: %s" XVR_CC_RESET,
              null.lexeme);
      return -1;
    }

    if (strncmp(semi.lexeme, ";", semi.length)) {
      fprintf(stderr,
              XVR_CC_ERROR "ERROR: semicolon lexeme error: %s" XVR_CC_RESET,
              semi.lexeme);
      return -1;
    }

    if (eof.type != XVR_TOKEN_EOF) {
      fprintf(stderr,
              XVR_CC_ERROR "ERROR: failed to find EOF token" XVR_CC_RESET);
      return -1;
    }
  }

  printf(XVR_CC_NOTICE "everything nice one gass\n" XVR_CC_RESET);
  return 0;
}
