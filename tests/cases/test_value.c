#include "xvr_console_colors.h"
#include "xvr_value.h"

#include <stdio.h>

int main() {
  printf(XVR_CC_WARN "testing: xvr value\n" XVR_CC_RESET);

  {
#if XVR_BITNESS == 64
    if (sizeof(Xvr_Value) != 16) {
#else
    if (sizeof(Xvr_Value) != 8) {
#endif
      fprintf(
          stderr, XVR_CC_ERROR
          "ERROR: 'Xvr_Value' is an unexpected size in memory\n" XVR_CC_RESET);
      return -1;
    }
  }

  {
    Xvr_Value v = XVR_VALUE_FROM_NULL();

    if (!XVR_VALUE_IS_NULL(v)) {
      fprintf(stderr, XVR_CC_ERROR
              "ERROR: creating a 'null' value failed\n" XVR_CC_RESET);
      return -1;
    }
  }

  {
    Xvr_Value t = XVR_VALUE_FROM_BOOLEAN(true);
    Xvr_Value f = XVR_VALUE_FROM_BOOLEAN(false);

    if (!XVR_VALUE_IS_TRUTHY(t) || XVR_VALUE_IS_TRUTHY(f)) {
      fprintf(stderr,
              XVR_CC_ERROR "Error: 'boolean' value failed\n" XVR_CC_RESET);
      return -1;
    }
  }

  {
    Xvr_Value answer = XVR_VALUE_FROM_INTEGER(42);
    Xvr_Value question = XVR_VALUE_FROM_INTEGER(42);
    Xvr_Value nice = XVR_VALUE_FROM_INTEGER(69);

    if (!XVR_VALUES_ARE_EQUAL(answer, question)) {
      fprintf(stderr, XVR_CC_ERROR
              "Error: equality check failed, expected true\n" XVR_CC_RESET);
      return -1;
    }

    if (XVR_VALUES_ARE_EQUAL(answer, nice)) {
      fprintf(stderr, XVR_CC_ERROR
              "Error: equality check failed, expected false\n" XVR_CC_RESET);
      return -1;
    }
  }

  printf(XVR_CC_NOTICE "Xvr value: jalan loh ya cik\n" XVR_CC_RESET);

  return 0;
}
