/**
MIT License

Copyright (c) 2025 arfy slowy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "xvr_array.h"

#include <stdio.h>
#include <stdlib.h>

#include "xvr_console_colors.h"
#include "xvr_value.h"

Xvr_Array* Xvr_resizeArray(Xvr_Array* paramArray, unsigned int capacity) {
    if (paramArray != NULL && paramArray->count > capacity) {
        for (unsigned int i = capacity; i < paramArray->count; i++) {
            Xvr_freeValue(paramArray->data[i]);
        }
    }

    if (capacity == 0) {
        free(paramArray);
        return NULL;
    }

    unsigned int originalCapacity =
        paramArray == NULL ? 0 : paramArray->capacity;
    Xvr_Array* array =
        realloc(paramArray, capacity * sizeof(Xvr_Value) + sizeof(Xvr_Array));

    if (array == NULL) {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Failed to resizing a 'Xvr_Array' from %d to "
                "%d capacity\n" XVR_CC_RESET,
                (int)originalCapacity, (int)capacity);
        exit(-1);
    }

    array->capacity = capacity;
    array->count =
        paramArray == NULL
            ? 0
            : (array->count > capacity ? capacity
                                       : array->count);  // truncate lost data

    return array;
}
