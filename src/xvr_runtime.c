/**
 * XVR Runtime Functions
 *
 * Provides runtime support functions for the XVR compiler.
 */

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef XVR_CC_ERROR
#    define XVR_CC_ERROR "\x1b[31m"
#    define XVR_CC_NOTICE "\x1b[38;2;140;207;126m"
#    define XVR_CC_RESET "\x1b[0m"
#endif

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

typedef struct {
    int* data;
    int size;
    int capacity;
} XvrArrayInt;

int xvr_array_len(void* arr_ptr) {
    if (!arr_ptr) return 0;
    XvrArrayInt* arr = (XvrArrayInt*)arr_ptr;
    return arr->size;
}

XvrArrayInt* xvr_array_create_int() {
    XvrArrayInt* arr = (XvrArrayInt*)malloc(sizeof(XvrArrayInt));
    if (!arr) return NULL;
    arr->data = NULL;
    arr->size = 0;
    arr->capacity = 0;
    return arr;
}

void xvr_array_insert_int(void* arr_ptr, int value) {
    XvrArrayInt* arr = (XvrArrayInt*)arr_ptr;
    if (!arr) return;
    if (arr->size >= arr->capacity) {
        int new_capacity = arr->capacity == 0 ? 4 : arr->capacity * 2;
        int* new_data = (int*)realloc(arr->data, sizeof(int) * new_capacity);
        if (!new_data) return;
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
    arr->data[arr->size++] = value;
}

static void xvr_array_error(const char* msg, int line) {
    fprintf(stderr, XVR_CC_ERROR "error: " XVR_CC_RESET "%s\n", msg);
    if (line > 0) {
        fprintf(stderr, "  --> line %d\n", line);
    }
}

static void xvr_array_error_idx(int idx, int size) {
    fprintf(stderr,
            XVR_CC_ERROR "error: " XVR_CC_RESET
                         "array index %d out of bounds (size: %d)\n",
            idx, size);
}

static void xvr_array_help(const char* msg) {
    fprintf(stderr, XVR_CC_NOTICE "help: " XVR_CC_RESET "%s\n", msg);
}

static void xvr_array_error_and_raise(const char* msg) {
    xvr_array_error(msg, 0);
    raise(SIGABRT);
}

int xvr_array_get_int(void* arr_ptr, int index) {
    XvrArrayInt* arr = (XvrArrayInt*)arr_ptr;
    if (!arr) return 0;
    if (arr->size == 0) {
        xvr_array_error("cannot get from empty array", 0);
        xvr_array_help("array is empty, use insert() to add elements first");
        raise(SIGABRT);
    }
    if (index < 0) {
        xvr_array_error("array index is negative", 0);
        xvr_array_help("use a non-negative index (0 or greater)");
        raise(SIGABRT);
    }
    if (index >= arr->size) {
        xvr_array_error_idx(index, arr->size);
        fprintf(stderr,
                XVR_CC_NOTICE "help: " XVR_CC_RESET
                              "valid index range is 0 to %d\n",
                arr->size - 1);
        raise(SIGABRT);
    }
    return arr->data[index];
}

void xvr_array_set_int(void* arr_ptr, int index, int value) {
    XvrArrayInt* arr = (XvrArrayInt*)arr_ptr;
    if (!arr) return;
    if (arr->size == 0) {
        xvr_array_error("cannot set in empty array", 0);
        xvr_array_help("array is empty, use insert() to add elements first");
        raise(SIGABRT);
    }
    if (index < 0) {
        xvr_array_error("array index is negative", 0);
        xvr_array_help("use a non-negative index (0 or greater)");
        raise(SIGABRT);
    }
    if (index >= arr->size) {
        xvr_array_error_idx(index, arr->size);
        fprintf(stderr,
                XVR_CC_NOTICE "help: " XVR_CC_RESET
                              "valid index range is 0 to %d\n",
                arr->size - 1);
        raise(SIGABRT);
    }
    arr->data[index] = value;
}
