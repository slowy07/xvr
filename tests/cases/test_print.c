#include <stdio.h>

#include "xvr_console_colors.h"
#include "xvr_print.h"

int counter = 0;

void count(const char* msg) { counter++; }

int test_callback() {
    {
        Xvr_setPrintCallback(count);

        Xvr_print("");

        if (counter != 1) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed to set print callback rek\n" XVR_CC_RESET);
            return -1;
        }

        Xvr_resetPrintCallback();
        Xvr_print("");

        if (counter != 1) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed to reset print callback rek\n" XVR_CC_RESET);
            return -1;
        }

        counter = 0;
    }

    return 0;
}

int main() {
    printf(XVR_CC_WARN "TESTING: XVR PRINT\n" XVR_CC_RESET);
    int total = 0, res = 0;

    {
        res = test_callback();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "CALLBACK: PASSED nice one rek\n" XVR_CC_RESET);
        }
        total += res;
    }

    return total;
}
