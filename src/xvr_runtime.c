/**
 * XVR Runtime Functions
 *
 * Provides runtime support functions for the XVR compiler.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char* xvr_string_concat(const char* lhs, const char* rhs) {
    if (!lhs) lhs = "";
    if (!rhs) rhs = "";

    size_t lhs_len = strlen(lhs);
    size_t rhs_len = strlen(rhs);
    size_t total_len = lhs_len + rhs_len;

    char* result = (char*)malloc(total_len + 1);
    if (!result) return NULL;

    memcpy(result, lhs, lhs_len);
    memcpy(result + lhs_len, rhs, rhs_len);
    result[total_len] = '\0';

    return result;
}

int xvr_str_len(const char* str) {
    if (!str) return 0;
    return (int)strlen(str);
}

int xvr_array_len(void* arr) { return 0; }
