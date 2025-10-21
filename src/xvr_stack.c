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

Xvr_Stack* Xvr_allocateStack(void) {
    Xvr_Stack* stack = malloc(XVR_STACK_INITIAL_CAPACITY * sizeof(Xvr_Value) +
                              sizeof(Xvr_Stack));

    if (stack == NULL) {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Failed to allocate a 'Xvr_Stack' of %d "
                "capacity (%d space in memory)\n" XVR_CC_RESET,
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
        free(stack);
    }
}

void Xvr_pushStack(Xvr_Stack** stack, Xvr_Value value) {
    if ((*stack)->count >= XVR_STACK_OVERFLOW_THRESHOLD) {
        fprintf(stderr, XVR_CC_ERROR "ERROR: Stack overflow\n" XVR_CC_RESET);
        exit(-1);
    }

    // expand the capacity if needed
    if ((*stack)->count + 1 > (*stack)->capacity) {
        while ((*stack)->count + 1 > (*stack)->capacity) {
            (*stack)->capacity =
                (*stack)->capacity < XVR_STACK_INITIAL_CAPACITY
                    ? XVR_STACK_INITIAL_CAPACITY
                    : (*stack)->capacity * XVR_STACK_EXPANSION_RATE;
        }

        unsigned int newCapacity = (*stack)->capacity;

        (*stack) = realloc((*stack),
                           newCapacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack));

        if ((*stack) == NULL) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Failed to reallocate a 'Xvr_Stack' of %d"
                    "capacity (%d space in memory)\n" XVR_CC_RESET,
                    (int)newCapacity,
                    (int)(newCapacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack)));
            exit(1);
        }
    }

    // Note: "pointer arithmetic in C/C++ is type-relative"
    ((Xvr_Value*)((*stack) + 1))[(*stack)->count++] = value;
}

Xvr_Value Xvr_peekStack(Xvr_Stack** stack) {
    if ((*stack)->count == 0) {
        fprintf(stderr, XVR_CC_ERROR "ERROR: Stack underflow\n" XVR_CC_RESET);
        exit(-1);
    }

    return ((Xvr_Value*)((*stack) + 1))[(*stack)->count - 1];
}

Xvr_Value Xvr_popStack(Xvr_Stack** stack) {
    if ((*stack)->count == 0) {
        fprintf(stderr, XVR_CC_ERROR "ERROR: Stack underflow\n" XVR_CC_RESET);
        exit(-1);
    }

    // shrink if possible
    if ((*stack)->capacity > XVR_STACK_INITIAL_CAPACITY &&
        (*stack)->count <=
            (*stack)->capacity / XVR_STACK_CONTRACTION_THRESHOLD) {
        (*stack)->capacity /= XVR_STACK_CONTRACTION_THRESHOLD;
        unsigned int newCapacity = (*stack)->capacity;

        (*stack) = realloc((*stack), (*stack)->capacity * sizeof(Xvr_Value) +
                                         sizeof(Xvr_Stack));

        if ((*stack) == NULL) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Failed to reallocate a 'Xvr_Stack' of %d "
                    "capacity (%d space in memory)\n" XVR_CC_RESET,
                    (int)newCapacity,
                    (int)(newCapacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack)));
            exit(1);
        }
    }

    return ((Xvr_Value*)((*stack) + 1))[--(*stack)->count];
}
