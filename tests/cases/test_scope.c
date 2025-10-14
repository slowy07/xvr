#include <stdio.h>

#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_scope.h"
#include "xvr_string.h"
#include "xvr_value.h"

int test_scope_allocation() {
    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        Xvr_Scope* scope = Xvr_pushScope(&bucket, NULL);

        if (scope == NULL || scope->next != NULL || scope->table == NULL ||
            scope->table->capacity != 8 || scope->refCount != 1 || false) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: failed to allocate Xvr_Scope\n" XVR_CC_RESET);
            Xvr_popScope(scope);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_popScope(scope);
        Xvr_freeBucket(&bucket);
    }

    return 0;
}

int test_scope_elements() {
    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        Xvr_Scope* scope = Xvr_pushScope(&bucket, NULL);

        Xvr_String* hello1 = Xvr_createNameStringLength(&bucket, "hello", 5,
                                                        XVR_VALUE_ANY, false);
        Xvr_String* hello2 = Xvr_createNameStringLength(&bucket, "hello", 5,
                                                        XVR_VALUE_ANY, false);

        if (Xvr_isDeclaredScope(scope, hello2)) {
            fprintf(
                stderr, XVR_CC_ERROR
                "ERROR: Unexpected entry found in Xvr_Scope\n" XVR_CC_RESET);
            Xvr_freeString(hello2);
            Xvr_freeString(hello1);
            Xvr_popScope(scope);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_declareScope(scope, hello1, XVR_VALUE_FROM_INTEGER(42));

        if (!Xvr_isDeclaredScope(scope, hello2)) {
            fprintf(
                stderr, XVR_CC_ERROR
                "ERROR: Unexpected missing entry in Xvr_Scope\n" XVR_CC_RESET);
            Xvr_freeString(hello2);
            Xvr_freeString(hello1);
            Xvr_popScope(scope);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_Value result = Xvr_accessScope(scope, hello2);

        if (scope == NULL || scope->next != NULL || scope->table == NULL ||
            scope->table->capacity != 8 || scope->refCount != 1 ||

            XVR_VALUE_IS_INTEGER(result) != true ||
            XVR_VALUE_AS_INTEGER(result) != 42 ||

            false) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: Failed to declare in Xvr_Scope\n" XVR_CC_RESET);
            Xvr_freeString(hello2);
            Xvr_freeString(hello1);
            Xvr_popScope(scope);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_assignScope(scope, hello1, XVR_VALUE_FROM_FLOAT(3.1415f));

        Xvr_Value resultTwo = Xvr_accessScope(scope, hello2);

        if (scope == NULL || scope->next != NULL || scope->table == NULL ||
            scope->table->capacity != 8 || scope->refCount != 1 ||

            XVR_VALUE_IS_FLOAT(resultTwo) != true ||
            XVR_VALUE_AS_FLOAT(resultTwo) != 3.1415f ||

            false) {
            fprintf(stderr, XVR_CC_ERROR
                    "ERROR: Failed to assign in Xvr_Scope\n" XVR_CC_RESET);
            Xvr_freeString(hello2);
            Xvr_freeString(hello1);
            Xvr_popScope(scope);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_freeString(hello2);
        Xvr_freeString(hello1);
        Xvr_popScope(scope);
        Xvr_freeBucket(&bucket);
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
        Xvr_Scope* scope = Xvr_pushScope(&bucket, NULL);

        Xvr_String* hello = Xvr_createNameStringLength(&bucket, "hello", 5,
                                                       XVR_VALUE_ANY, false);

        Xvr_declareScope(scope, hello, XVR_VALUE_FROM_INTEGER(42));

        scope = Xvr_pushScope(&bucket, scope);
        scope = Xvr_pushScope(&bucket, scope);

        {
            Xvr_Value result1 = Xvr_accessScope(scope, hello);

            if (XVR_VALUE_IS_INTEGER(result1) != true ||
                XVR_VALUE_AS_INTEGER(result1) != 42) {
                fprintf(stderr, XVR_CC_ERROR
                        "ERROR: Failed to access from an ancestor "
                        "Xvr_Scope\n" XVR_CC_RESET);
                Xvr_freeString(hello);
                while ((scope = Xvr_popScope(scope)) != NULL);
                Xvr_freeBucket(&bucket);
                return -1;
            }
        }

        Xvr_declareScope(scope, hello, XVR_VALUE_FROM_FLOAT(3.1415f));

        {
            Xvr_Value result2 = Xvr_accessScope(scope, hello);

            if (XVR_VALUE_IS_FLOAT(result2) != true ||
                XVR_VALUE_AS_FLOAT(result2) != 3.1415f) {
                fprintf(stderr, XVR_CC_ERROR
                        "ERROR: Failed to shadow an entry in "
                        "Xvr_Scope\n" XVR_CC_RESET);
                Xvr_freeString(hello);
                while ((scope = Xvr_popScope(scope)) != NULL);
                Xvr_freeBucket(&bucket);
                return -1;
            }
        }

        scope = Xvr_popScope(scope);

        {
            Xvr_Value result3 = Xvr_accessScope(scope, hello);

            if (XVR_VALUE_IS_INTEGER(result3) != true ||
                XVR_VALUE_AS_INTEGER(result3) != 42) {
                fprintf(stderr, XVR_CC_ERROR
                        "ERROR: Failed to recover an entry in "
                        "Xvr_Scope\n" XVR_CC_RESET);
                Xvr_freeString(hello);
                while ((scope = Xvr_popScope(scope)) != NULL);
                Xvr_freeBucket(&bucket);
                return -1;
            }
        }

        Xvr_assignScope(scope, hello, XVR_VALUE_FROM_INTEGER(8891));

        {
            Xvr_Value result4 = Xvr_accessScope(scope, hello);

            if (XVR_VALUE_IS_INTEGER(result4) != true ||
                XVR_VALUE_AS_INTEGER(result4) != 8891) {
                fprintf(stderr, XVR_CC_ERROR
                        "ERROR: Failed to assign to an ancestor "
                        "in Xvr_Scope\n" XVR_CC_RESET);
                Xvr_freeString(hello);
                while ((scope = Xvr_popScope(scope)) != NULL);
                Xvr_freeBucket(&bucket);
                return -1;
            }
        }

        scope = Xvr_popScope(scope);

        {
            Xvr_Value result5 = Xvr_accessScope(scope, hello);

            if (XVR_VALUE_IS_INTEGER(result5) != true ||
                XVR_VALUE_AS_INTEGER(result5) != 8891) {
                fprintf(stderr, XVR_CC_ERROR
                        "ERROR: Failed to access an altered entry of an "
                        "ancestor in Xvr_Scope\n" XVR_CC_RESET);
                Xvr_freeString(hello);
                while ((scope = Xvr_popScope(scope)) != NULL);
                Xvr_freeBucket(&bucket);
                return -1;
            }
        }

        Xvr_freeString(hello);
        while ((scope = Xvr_popScope(scope)) != NULL);
        Xvr_freeBucket(&bucket);
    }

    return 0;
}

int main() {
    printf(XVR_CC_WARN "TESTING: PASSED XVR SCOPE\n" XVR_CC_RESET);
    int total = 0, res = 0;

    {
        res = test_scope_allocation();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "SCOPE ALLOCATION: PASSED nice one cik\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        res = test_scope_elements();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "SCOPE ELEMENTS: PASSED nice one cik\n" XVR_CC_RESET);
        }
        total += res;
    }

    return total;
}
