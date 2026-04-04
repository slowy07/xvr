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

#ifndef XVR_CC_ERROR
#    define XVR_CC_ERROR "\x1b[31m"
#    define XVR_CC_NOTICE "\x1b[38;2;140;207;126m"
#    define XVR_CC_RESET "\x1b[0m"
#endif

typedef struct {
    int* data;
    int size;
    int capacity;
} XvrArrayInt;

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

static void xvr_array_error(const char* msg) {
    fprintf(stderr, XVR_CC_ERROR "error: " XVR_CC_RESET);
    fprintf(stderr, "%s\n", msg);
}

static void xvr_array_error_idx(int idx, int size) {
    fprintf(stderr, XVR_CC_ERROR "error: " XVR_CC_RESET "array index ");
    fprintf(stderr, "%d", idx);
    fprintf(stderr, " out of bounds (size: ");
    fprintf(stderr, "%d", size);
    fprintf(stderr, ")\n");
}

static void xvr_array_help(const char* msg) {
    fprintf(stderr, XVR_CC_NOTICE "help: " XVR_CC_RESET "%s\n", msg);
}

int xvr_array_len(void* arr_ptr) {
    XvrArrayInt* arr = (XvrArrayInt*)arr_ptr;
    if (!arr) return 0;
    return arr->size;
}

int xvr_array_get_int(void* arr_ptr, int index) {
    XvrArrayInt* arr = (XvrArrayInt*)arr_ptr;
    if (!arr) return 0;
    if (arr->size == 0) {
        xvr_array_error("cannot get from empty array");
        xvr_array_help("array is empty, use insert() to add elements first");
        raise(SIGABRT);
    }
    if (index < 0) {
        xvr_array_error("array index is negative");
        xvr_array_help("use a non-negative index (0 or greater)");
        raise(SIGABRT);
    }
    if (index >= arr->size) {
        xvr_array_error_idx(index, arr->size);
        fprintf(stderr, XVR_CC_NOTICE "help: " XVR_CC_RESET
                                      "valid index range is 0 to ");
        int upper = arr->size - 1;
        fprintf(stderr, "%d", upper);
        fprintf(stderr, "\n");
        raise(SIGABRT);
    }
    return arr->data[index];
}

void xvr_array_set_int(void* arr_ptr, int index, int value) {
    XvrArrayInt* arr = (XvrArrayInt*)arr_ptr;
    if (!arr) return;
    if (arr->size == 0) {
        xvr_array_error("cannot set in empty array");
        xvr_array_help("array is empty, use insert() to add elements first");
        raise(SIGABRT);
    }
    if (index < 0) {
        xvr_array_error("array index is negative");
        xvr_array_help("use a non-negative index (0 or greater)");
        raise(SIGABRT);
    }
    if (index >= arr->size) {
        xvr_array_error_idx(index, arr->size);
        fprintf(stderr, XVR_CC_NOTICE "help: " XVR_CC_RESET
                                      "valid index range is 0 to ");
        int upper = arr->size - 1;
        fprintf(stderr, "%d", upper);
        fprintf(stderr, "\n");
        raise(SIGABRT);
    }
    arr->data[index] = value;
}
