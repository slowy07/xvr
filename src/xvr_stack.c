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

#include "xvr_stack.h"

#include <stdio.h>
#include <stdlib.h>

#include "xvr_console_colors.h"
#include "xvr_value.h"

Xvr_Stack* Xvr_allocateStack(void) {
    Xvr_Stack* stack = malloc(XVR_STACK_INITIAL_CAPACITY * sizeof(Xvr_Value) +
                              sizeof(Xvr_Stack));

    if (stack == NULL) {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Failed to allocate a 'Xvr_Stack' of %d capacity (%d "
                "space in memory)\n" XVR_CC_RESET,
                XVR_STACK_INITIAL_CAPACITY,
                (int)(XVR_STACK_INITIAL_CAPACITY * sizeof(Xvr_Value) +
                      sizeof(Xvr_Stack)));
        exit(1);
    }

    stack->capacity = XVR_STACK_INITIAL_CAPACITY;
    stack->count = 0;

    return stack;
}

void Xvr_freeStack(Xvr_Stack* stack) {
    if (stack != NULL) {
        for (unsigned int i = 0; i < stack->count; i++) {
            Xvr_freeValue(stack->data[i]);
        }

        free(stack);
    }
}

void Xvr_resetStack(Xvr_Stack** stackHandle) {
    if ((*stackHandle) == NULL) {
        return;
    }

    // if some values will be removed, free them first
    for (unsigned int i = 0; i < (*stackHandle)->count; i++) {
        Xvr_freeValue((*stackHandle)->data[i]);
    }

    // reset to the stack's default state
    if ((*stackHandle)->capacity > XVR_STACK_INITIAL_CAPACITY) {
        (*stackHandle) = realloc(
            (*stackHandle),
            XVR_STACK_INITIAL_CAPACITY * sizeof(Xvr_Value) + sizeof(Xvr_Stack));

        (*stackHandle)->capacity = XVR_STACK_INITIAL_CAPACITY;
    }

    (*stackHandle)->count = 0;
}

void Xvr_pushStack(Xvr_Stack** stackHandle, Xvr_Value value) {
    if ((*stackHandle)->count >= XVR_STACK_OVERFLOW_THRESHOLD) {
        fprintf(stderr, XVR_CC_ERROR "ERROR: Stack overflow\n" XVR_CC_RESET);
        exit(-1);
    }

    if ((*stackHandle)->count + 1 > (*stackHandle)->capacity) {
        while ((*stackHandle)->count + 1 > (*stackHandle)->capacity) {
            (*stackHandle)->capacity =
                (*stackHandle)->capacity < XVR_STACK_INITIAL_CAPACITY
                    ? XVR_STACK_INITIAL_CAPACITY
                    : (*stackHandle)->capacity * XVR_STACK_EXPANSION_RATE;
        }

        unsigned int newCapacity = (*stackHandle)->capacity;

        (*stackHandle) =
            realloc((*stackHandle),
                    newCapacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack));

        if ((*stackHandle) == NULL) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Failed to reallocate a 'Xvr_Stack' of %d capacity "
                    "(%d space in memory)\n" XVR_CC_RESET,
                    (int)newCapacity,
                    (int)(newCapacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack)));
            exit(1);
        }
    }

    ((Xvr_Value*)((*stackHandle) + 1))[(*stackHandle)->count++] = value;
}

Xvr_Value Xvr_peekStack(Xvr_Stack** stackHandle) {
    if ((*stackHandle)->count == 0) {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Stack underflow when peeking\n" XVR_CC_RESET);
        exit(-1);
    }

    return ((Xvr_Value*)((*stackHandle) + 1))[(*stackHandle)->count - 1];
}

Xvr_Value Xvr_popStack(Xvr_Stack** stackHandle) {
    if ((*stackHandle)->count == 0) {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Stack underflow when popping\n" XVR_CC_RESET);
        exit(-1);
    }

    if ((*stackHandle)->capacity > XVR_STACK_INITIAL_CAPACITY &&
        (*stackHandle)->count <=
            (*stackHandle)->capacity / XVR_STACK_CONTRACTION_THRESHOLD) {
        (*stackHandle)->capacity /= XVR_STACK_CONTRACTION_THRESHOLD;
        unsigned int newCapacity = (*stackHandle)->capacity;

        (*stackHandle) = realloc(
            (*stackHandle),
            (*stackHandle)->capacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack));

        if ((*stackHandle) == NULL) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Failed to reallocate a 'Xvr_Stack' of %d capacity "
                    "(%d space in memory)\n" XVR_CC_RESET,
                    (int)newCapacity,
                    (int)(newCapacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack)));
            exit(1);
        }
    }

    return ((Xvr_Value*)((*stackHandle) + 1))[--(*stackHandle)->count];
}
