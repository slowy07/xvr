#include "xvr_stack.h"
#include "xvr_console_colors.h"

#include <stdio.h>
#include <stdlib.h>

#define MIN_CAPACITY 64

Xvr_Stack *Xvr_allocateStack() {
  Xvr_Stack *stack =
      malloc(MIN_CAPACITY * sizeof(Xvr_Value) + sizeof(Xvr_Stack));

  if (stack == NULL) {
    fprintf(stderr,
            XVR_CC_ERROR "ERROR: Failed to allocate a 'Xvr_Stack' of %d "
                         "capacity (%d space in memory)\n" XVR_CC_RESET,
            MIN_CAPACITY,
            (int)(MIN_CAPACITY * sizeof(Xvr_Value) + sizeof(Xvr_Stack)));
    exit(1);
  }

  stack->capacity = MIN_CAPACITY;
  stack->count = 0;

  return stack;
}

void Xvr_freeStack(Xvr_Stack *stack) {
  if (stack != NULL) {
    free(stack);
  }
}

void Xvr_pushStack(Xvr_Stack **stack, Xvr_Value value) {
  if ((*stack)->count >= 1024 * 1024 / sizeof(Xvr_Value)) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Stack overflow\n" XVR_CC_RESET);
    exit(-1);
  }

  // expand the capacity if needed
  if ((*stack)->count + 1 > (*stack)->capacity) {
    while ((*stack)->count + 1 > (*stack)->capacity) {
      (*stack)->capacity = (*stack)->capacity < MIN_CAPACITY
                               ? MIN_CAPACITY
                               : (*stack)->capacity * 2;
    }

    unsigned int newCapacity = (*stack)->capacity;

    (*stack) =
        realloc((*stack), newCapacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack));

    if ((*stack) == NULL) {
      fprintf(stderr,
              XVR_CC_ERROR "ERROR: Failed to reallocate a 'Xvr_Stack' of %d "
                           "capacity (%d space in memory)\n" XVR_CC_RESET,
              (int)newCapacity,
              (int)(newCapacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack)));
      exit(1);
    }
  }

  // Note: "pointer arithmetic in C/C++ is type-relative"
  ((Xvr_Value *)((*stack) + 1))[(*stack)->count++] = value;
}

Xvr_Value Xvr_peekStack(Xvr_Stack **stack) {
  if ((*stack)->count == 0) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Stack underflow\n" XVR_CC_RESET);
    exit(-1);
  }

  return ((Xvr_Value *)((*stack) + 1))[(*stack)->count - 1];
}

Xvr_Value Xvr_popStack(Xvr_Stack **stack) {
  if ((*stack)->count == 0) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Stack underflow\n" XVR_CC_RESET);
    exit(-1);
  }

  // shrink if possible
  if ((*stack)->count > MIN_CAPACITY &&
      (*stack)->count < (*stack)->capacity / 4) {
    (*stack)->capacity /= 2;
    unsigned int newCapacity = (*stack)->capacity;

    (*stack) = realloc((*stack), (*stack)->capacity * sizeof(Xvr_Value) +
                                     sizeof(Xvr_Stack));

    if ((*stack) == NULL) {
      fprintf(stderr,
              XVR_CC_ERROR "ERROR: Failed to reallocate a 'Xvr_Stack' of %d "
                           "capacity (%d space in memory)\n" XVR_CC_RESET,
              (int)newCapacity,
              (int)(newCapacity * sizeof(Xvr_Value) + sizeof(Xvr_Stack)));
      exit(1);
    }
  }

  return ((Xvr_Value *)((*stack) + 1))[--(*stack)->count];
}
