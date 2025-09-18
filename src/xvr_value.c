#include "xvr_value.h"
#include "xvr_console_color.h"
#include <stdio.h>

bool Xvr_private_isTruthy(Xvr_Value value) {
  if (XVR_VALUE_IS_NULL(value)) {
    fprintf(stderr, XVR_CC_ERROR
            "Error: 'null' is neither true or false\n" XVR_CC_RESET);
    return false;
  }

  if (XVR_VALUE_IS_BOOLEAN(value)) {
    return XVR_VALUE_AS_BOOLEAN(value);
  }

  return true;
}
