#include "../../src/xvr_array.h"
#include "../../src/xvr_console_colors.h"

#include <stdio.h>

int test_resizeArray() {
  {
    Xvr_Array *array = XVR_ALLOCATE_ARRAY(int, 1);
    XVR_FREE_ARRAY(int, array);
  }

  {
    Xvr_Array *array = XVR_ALLOCATE_ARRAY(int, 10);
    array->data[1] = 42;
    XVR_FREE_ARRAY(int, array);
  }

  {
    Xvr_Array *array1 = XVR_ALLOCATE_ARRAY(int, 10);
    Xvr_Array *array2 = XVR_ALLOCATE_ARRAY(int, 10);

    array1->data[1] = 42;
    array2->data[1] = 42;

    XVR_FREE_ARRAY(int, array1);
    XVR_FREE_ARRAY(int, array2);
  }

  return 0;
}

int main() {
  int total = 0, res = 0;

  {
    res = test_resizeArray();
    total += res;

    if (res == 0) {
      printf(XVR_CC_NOTICE "test_resizeArray(): nice one rek\n" XVR_CC_RESET);
    }
  }

  return total;
}
