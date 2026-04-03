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

int xvr_array_get_int(void* arr_ptr, int index) {
    XvrArrayInt* arr = (XvrArrayInt*)arr_ptr;
    if (!arr || index < 0 || index >= arr->size) return 0;
    return arr->data[index];
}

void xvr_array_set_int(void* arr_ptr, int index, int value) {
    XvrArrayInt* arr = (XvrArrayInt*)arr_ptr;
    if (!arr || index < 0 || index >= arr->size) return;
    arr->data[index] = value;
}
