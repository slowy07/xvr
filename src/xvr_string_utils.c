/**
 * XVR String Utilities Implementation
 *
 * Provides safe string handling functions to prevent buffer over-reads
 * and other security vulnerabilities.
 */

#include "xvr_string_utils.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

size_t xvr_safe_strlen(const char* str, size_t max_len) {
    if (!str) return 0;
    size_t len = 0;
    while (str[len] != '\0' && len < max_len) {
        len++;
    }
    return len;
}

size_t xvr_safe_strlen_bounded(const char* str, size_t max_len) {
    if (!str) return 0;
    size_t len = 0;
    while (str[len] != '\0' && len < max_len) {
        len++;
    }
    return len;
}

int xvr_safe_strcmp(const char* a, const char* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
        i++;
    }
    if (a[i] == '\0' && b[i] == '\0') {
        return 0;
    }
    return a[i] == '\0' ? -1 : 1;
}

int xvr_safe_strncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
        if (a[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

char* xvr_safe_strdup(const char* str, size_t max_len) {
    if (!str || max_len == 0) return NULL;
    size_t len = xvr_safe_strlen(str, max_len);
    if (len == 0 || len >= max_len) return NULL;
    if (len > SIZE_MAX - 1) return NULL;

    size_t alloc_size = len + 1;
    if (alloc_size < len || alloc_size == 0) return NULL;

    char* result = malloc(alloc_size);
    if (!result) return NULL;

    for (size_t i = 0; i < len; i++) {
        result[i] = str[i];
    }
    result[len] = '\0';
    return result;
}
