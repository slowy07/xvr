#include <stdio.h>

#include "xvr_array.h"
#include "xvr_console_colors.h"
#include "xvr_value.h"

int test_resizeArray(void) {
    {
        Xvr_Array* array = Xvr_resizeArray(NULL, 1);
        array = Xvr_resizeArray(array, 0);
    }

    {
        Xvr_Array* array = Xvr_resizeArray(NULL, 10);
        array->data[1] = XVR_VALUE_FROM_INTEGER(42);
        Xvr_resizeArray(array, 0);
    }

    {
        Xvr_Array* array1 = Xvr_resizeArray(NULL, 10);
        Xvr_Array* array2 = Xvr_resizeArray(NULL, 10);

        array1->data[1] = XVR_VALUE_FROM_INTEGER(42);
        array2->data[1] = XVR_VALUE_FROM_INTEGER(42);

        Xvr_resizeArray(array1, 0);
        Xvr_resizeArray(array2, 0);
    }

    return 0;
}

int main(void) {
    printf(XVR_CC_WARN "TESTING: XVR ARRAY\n" XVR_CC_RESET);
    int total = 0, res = 0;

    {
        res = test_resizeArray();
        total += res;

        if (res == 0) {
            printf(XVR_CC_NOTICE "RESIZE ARRAY: PASSED cik\n" XVR_CC_RESET);
        }
    }

    return total;
}
