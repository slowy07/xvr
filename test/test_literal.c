#include <stdio.h>

#include "xvr_console_colors.h"
#include "xvr_literal.h"
#include "xvr_refstring.h"

int main(void) {
    {
        Xvr_Literal literal = XVR_TO_NULL_LITERAL;

        if (!XVR_IS_NULL(literal)) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: null literal failed cik\n" XVR_CC_RESET);
            return -1;
        }
    }

    {
        Xvr_Literal t = XVR_TO_BOOLEAN_LITERAL(true);
        Xvr_Literal f = XVR_TO_BOOLEAN_LITERAL(false);

        if (!XVR_IS_TRUTHY(t) || XVR_IS_TRUTHY(f)) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR; boolean literal failed loh ya\n" XVR_CC_RESET);
            return -1;
        }
    }

    {
        char* buffer = "wello cik";
        Xvr_Literal literal =
            XVR_TO_STRING_LITERAL(Xvr_createRefString(buffer));
        Xvr_freeLiteral(literal);
    }

    printf(XVR_CC_NOTICE "TEST LITERAL: aman loh ya cik\n" XVR_CC_RESET);
    return 0;
}
