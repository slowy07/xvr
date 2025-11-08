#include <stdio.h>

#include "xvr_console_colors.h"
#include "xvr_literal.h"
#include "xvr_memory.h"
#include "xvr_scope.h"

int main(void) {
    {
        Xvr_Scope* scope = Xvr_pushScope(NULL);
        scope = Xvr_popScope(scope);
    }

    {
        char* idn_raw = "xvr data";
        Xvr_Literal identifier =
            XVR_TO_IDENTIFIER_LITERAL(Xvr_createRefString(idn_raw));
        Xvr_Literal value = XVR_TO_INTEGER_LITERAL(42);
        Xvr_Literal type = XVR_TO_TYPE_LITERAL(value.type, false);

        Xvr_Scope* scope = Xvr_pushScope(NULL);

        if (!Xvr_declareScopeVariable(scope, identifier, type)) {
            printf(XVR_CC_ERROR
                   "Woilah cik failed to declare scope variable loh "
                   "ya\n" XVR_CC_RESET);
            return -1;
        }

        if (!Xvr_setScopeVariable(scope, identifier, value, true)) {
            printf(XVR_CC_ERROR
                   "Woilah cik failed to set scope variable loh "
                   "ya\n" XVR_CC_RESET);
            return -1;
        }

        scope = Xvr_popScope(scope);
        Xvr_freeLiteral(identifier);
        Xvr_freeLiteral(value);
        Xvr_freeLiteral(type);
    }

    printf(XVR_CC_NOTICE "SCOPE TEST: aman cik\n" XVR_CC_RESET);
    return 0;
}
