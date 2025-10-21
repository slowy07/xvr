#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_array.h"
#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_string.h"
#include "xvr_value.h"

#define XVR_ARRAY_EXPAND(array)                                              \
    (array = (array != NULL && (array)->count + 1 > (array)->capacity        \
                  ? Xvr_resizeArray(                                         \
                        array, (array)->capacity * XVR_ARRAY_EXPANSION_RATE) \
                  : array))
#define XVR_ARRAY_PUSHBACK(array, value) \
    (XVR_ARRAY_EXPAND(array), (array)->data[(array)->count++] = (value))

int test_value_creation(void) {
    {
#if XVR_BITNESS == 64
        if (sizeof(Xvr_Value) != 16) {
#else
        if (sizeof(Xvr_Value) != 8) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "Error: `Xvr_Value` is an unxpected size in memory, "
                    "expected %d found %d\n" XVR_CC_RESET,
                    XVR_BITNESS, sizeof(Xvr_Value));
        }
    }
#endif /* if XVR_BITNESS == 64 */

            {
                Xvr_Value v = XVR_VALUE_FROM_NULL();

                if (!XVR_VALUE_IS_NULL(v)) {
                    fprintf(
                        stderr, XVR_CC_ERROR
                        "Error: creating `null` value failed\n" XVR_CC_RESET);
                    return -1;
                }
            }

            {
                // TEST: make boolean
                Xvr_Value t = XVR_VALUE_FROM_BOOLEAN(true);
                Xvr_Value f = XVR_VALUE_FROM_BOOLEAN(false);

                if (!Xvr_checkValueIsTruthy(t) || Xvr_checkValueIsTruthy(f)) {
                    fprintf(stderr, XVR_CC_ERROR
                            "Error: `boolean` value failed\n" XVR_CC_RESET);
                }
            }
        }
    }

    {
        Xvr_Array* array = Xvr_resizeArray(NULL, XVR_ARRAY_INITIAL_CAPACITY);
        XVR_ARRAY_PUSHBACK(array, XVR_VALUE_FROM_INTEGER(42));
        XVR_ARRAY_PUSHBACK(array, XVR_VALUE_FROM_INTEGER(69));
        XVR_ARRAY_PUSHBACK(array, XVR_VALUE_FROM_INTEGER(8891));

        Xvr_Value v = XVR_VALUE_FROM_ARRAY(array);

        if (XVR_VALUE_AS_ARRAY(v) == false ||
            XVR_VALUE_AS_ARRAY(v)->capacity != 8 ||
            XVR_VALUE_AS_ARRAY(v)->count != 3 ||
            XVR_VALUE_IS_INTEGER(XVR_VALUE_AS_ARRAY(v)->data[0]) != true ||
            XVR_VALUE_AS_INTEGER(XVR_VALUE_AS_ARRAY(v)->data[0]) != 42 ||
            XVR_VALUE_IS_INTEGER(XVR_VALUE_AS_ARRAY(v)->data[1]) != true ||
            XVR_VALUE_AS_INTEGER(XVR_VALUE_AS_ARRAY(v)->data[1]) != 69 ||
            XVR_VALUE_IS_INTEGER(XVR_VALUE_AS_ARRAY(v)->data[2]) != true ||
            XVR_VALUE_AS_INTEGER(XVR_VALUE_AS_ARRAY(v)->data[2]) != 8891) {
            fprintf(stderr,
                    XVR_CC_ERROR "Error: `array` value failed\n" XVR_CC_RESET);
            Xvr_resizeArray(array, 0);
            return -1;
        }

        Xvr_resizeArray(array, 0);
    }

    return 0;
}

int test_comparison(void) {
    {
        Xvr_Value ans = XVR_VALUE_FROM_INTEGER(42);
        Xvr_Value quest = XVR_VALUE_FROM_INTEGER(42);
        Xvr_Value no = XVR_VALUE_FROM_NULL();

        if (Xvr_checkValuesAreComparable(ans, quest) != true) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: value comparison check failed, expected "
                    "true\n" XVR_CC_RESET);
            return -1;
        }

        if (Xvr_checkValuesAreComparable(ans, no) != false) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: value comparison check failed, expected "
                    "false\n" XVR_CC_RESET);
            return -1;
        }
    }

    return 0;
}

int test_value_stringify(void) {
    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_SMALL);
        Xvr_Value value = XVR_VALUE_FROM_NULL();

        Xvr_String* string = Xvr_stringifyValue(&bucket, value);

        if (string->info.type != XVR_STRING_LEAF ||
            strcmp(string->leaf.data, "null") != 0) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: stringify `null` failed\n" XVR_CC_RESET);
            Xvr_freeString(string);
            Xvr_freeValue(value);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_freeString(string);
        Xvr_freeValue(value);
        Xvr_freeBucket(&bucket);
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_SMALL);
        Xvr_Value value = XVR_VALUE_FROM_BOOLEAN(true);

        Xvr_String* string = Xvr_stringifyValue(&bucket, value);

        if (string->info.type != XVR_STRING_LEAF ||
            strcmp(string->leaf.data, "true") != 0) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: stringify boolean `true` failed\n" XVR_CC_ERROR);
            Xvr_freeString(string);
            Xvr_freeValue(value);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_freeString(string);
        Xvr_freeValue(value);
        Xvr_freeBucket(&bucket);
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_SMALL);

        Xvr_Array* array = Xvr_resizeArray(NULL, XVR_ARRAY_INITIAL_CAPACITY);
        XVR_ARRAY_PUSHBACK(array, XVR_VALUE_FROM_INTEGER(42));
        XVR_ARRAY_PUSHBACK(array, XVR_VALUE_FROM_INTEGER(60));
        XVR_ARRAY_PUSHBACK(array, XVR_VALUE_FROM_INTEGER(8891));

        Xvr_Value value = XVR_VALUE_FROM_ARRAY(array);
        Xvr_String* string = Xvr_stringifyValue(&bucket, value);
        char* buffer = Xvr_getStringRawBuffer(string);

        if (buffer == NULL || strcmp(buffer, "[42,60,8891]") != 0) {
            fprintf(
                stderr, XVR_CC_ERROR
                " ERROR: stringify array [42, 60, 8891] failed\n" XVR_CC_RESET);
            free(buffer);
            Xvr_resizeArray(array, 0);
            Xvr_freeBucket(&bucket);
            return -1;
        }
        free(buffer);
        Xvr_resizeArray(array, 0);
        Xvr_freeBucket(&bucket);
    }

    return 0;
}

int test_value_copying(void) {
    {
        Xvr_Value original = XVR_VALUE_FROM_INTEGER(42);
        Xvr_Value result = Xvr_copyValue(original);

        if (!XVR_VALUE_IS_INTEGER(result) ||
            XVR_VALUE_AS_INTEGER(result) != 42) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: copy an integer value failed\n" XVR_CC_RESET);
            return -1;
        }
    }

    {
        Xvr_Bucket* bucket = Xvr_allocateBucket(XVR_BUCKET_SMALL);
        Xvr_Value original =
            XVR_VALUE_FROM_STRING(Xvr_createString(&bucket, "woilah cik"));
        Xvr_Value result = Xvr_copyValue(original);

        if (XVR_VALUE_IS_STRING(result) == false ||
            XVR_VALUE_AS_STRING(result)->info.type != XVR_STRING_LEAF ||
            strcmp(XVR_VALUE_AS_STRING(result)->leaf.data, "woilah cik") != 0 ||
            XVR_VALUE_AS_STRING(result)->info.refCount != 2) {
            fprintf(stderr, XVR_CC_ERROR
                    "Error: copy a strinb calue failed\n" XVR_CC_RESET);
            Xvr_freeValue(original);
            Xvr_freeValue(result);
            Xvr_freeBucket(&bucket);
            return -1;
        }

        Xvr_freeValue(original);
        Xvr_freeValue(result);
        Xvr_freeBucket(&bucket);
    }

    return 0;
}

int main(void) {
    printf(XVR_CC_WARN "TESTING: XVR VALUE\n" XVR_CC_RESET);

    int total = 0, res = 0;

    {
        res = test_value_creation();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "VALUE CREATION: PASSED aman loh ya cik\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        res = test_comparison();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "COMPARISON: PASSED aman loh ya cik\n" XVR_CC_RESET);
        }
        total += res;
    }

    {
        res = test_value_copying();
        if (res == 0) {
            printf(XVR_CC_NOTICE
                   "VALUE COPYING: PASSED aman loh ya cik\n" XVR_CC_RESET);
        }
        total += res;
    }

    return total;
}
