#include "../../src/xvr_console_color.h"
#include "../../src/xvr_value.h"

#include <assert.h>
#include <stdio.h>

int main() {
  {
    if (sizeof(Xvr_Value) != 8) {
      fprintf(stderr, XVR_CC_ERROR
              "Error: `Xvr_Value` an unexpected size in memory\n" XVR_CC_RESET);
      return -1;
    }
  }

  {
    Xvr_Value v = XVR_VALUE_TO_NULL();

    if (!XVR_VALUE_IS_NULL(v)) {
      fprintf(stderr, XVR_CC_ERROR
              "Error: creating a `null` value failed\n" XVR_CC_RESET);
      return -1;
    }
  }

  printf(XVR_CC_NOTICE "everything nice one gass\n" XVR_CC_RESET);
  return 0;
}
