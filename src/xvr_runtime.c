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

typedef struct {
    void* buffer;
    int size;
    int capacity;
} XvrArray;

XvrArray* xvr_array_create_int(int initial_capacity) {
    XvrArray* arr = (XvrArray*)malloc(sizeof(XvrArray));
    if (!arr) return NULL;

    arr->size = 0;
    arr->capacity = initial_capacity > 0 ? initial_capacity : 1;
    arr->buffer = malloc(sizeof(int) * arr->capacity);
    if (!arr->buffer) {
        free(arr);
        return NULL;
    }
    return arr;
}

void xvr_array_insert_int(XvrArray* arr, int value) {
    if (!arr || !arr->buffer) return;

    if (arr->size >= arr->capacity) {
        int new_capacity = arr->capacity * 2;
        void* new_buffer = realloc(arr->buffer, sizeof(int) * new_capacity);
        if (!new_buffer) return;
        arr->buffer = new_buffer;
        arr->capacity = new_capacity;
    }

    ((int*)arr->buffer)[arr->size++] = value;
}

int xvr_array_get_int(XvrArray* arr, int index) {
    if (!arr || !arr->buffer) return 0;
    if (index < 0 || index >= arr->size) return 0;
    return ((int*)arr->buffer)[index];
}

void xvr_array_destroy(XvrArray* arr) {
    if (!arr) return;
    if (arr->buffer) free(arr->buffer);
    free(arr);
}
