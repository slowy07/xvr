#include <stdio.h>
#include <stdlib.h>

#include "../../src/xvr_array.h"

int main(int argc, char* argv[]) {
    (void)argc;
    unsigned int iterations = atoi(argv[1]);

    Xvr_Array* array = XVR_ARRAY_ALLOCATE();

    printf("found %d iteration\n", iterations);

    for (unsigned int i = 0; i < iterations; i++) {
        XVR_ARRAY_PUSHBACK(array, XVR_VALUE_FROM_INTEGER(i));
    }

    XVR_ARRAY_FREE(array);
    return 0;
}
