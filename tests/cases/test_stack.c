#include "xvr_console_colors.h"
#include "xvr_stack.h"
#include "xvr_value.h"

#include <stdio.h>

int test_stack_basics() {
  {
    Xvr_Stack *stack = Xvr_allocateStack();

    if (stack == NULL || stack->capacity != 8 || stack->count != 0) {
      fprintf(stderr, XVR_CC_ERROR
              "Error: failed to allocate Xvr_Stack\n" XVR_CC_ERROR);
      Xvr_freeStack(stack);
      return -1;
    }

    Xvr_freeStack(stack);
  }
  return 0;
}

int test_stack_stress() {
  {
    Xvr_Stack *stack = Xvr_allocateStack();

    for (int i = 0; i < 500; i++) {
      Xvr_pushStack(&stack, XVR_VALUE_FROM_INTEGER(i));
    }

    if (stack == NULL || stack->capacity != 512 || stack->count != 500) {
      fprintf(stderr, XVR_CC_ERROR
              "Error: failed to stress the Xvr_Stack\n" XVR_CC_RESET);
      Xvr_freeStack(stack);
      return -1;
    }

    Xvr_freeStack(stack);
  }
  return 0;
}

int main() {
  printf(XVR_CC_WARN "testing: xvr stack\n" XVR_CC_RESET);

  int total = 0, res = 0;

  {
    res = test_stack_basics();
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "test_stack_basics(): jalan loh ya cik\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    res = test_stack_stress();
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "test_stack_stress(): jalan loh ya juga cik\n" XVR_CC_RESET);
    }
    total += res;
  }

  return total;
}
