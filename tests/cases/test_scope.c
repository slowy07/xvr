#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_scope.h"
#include <stdio.h>

int test_scope_allocation() {
  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    Xvr_Scope *scope = Xvr_pushScope(&bucket, NULL);

    if (scope == NULL || scope->next != NULL || scope->table == NULL ||
        scope->table->capacity != 16 || scope->refCount != 1 || false) {
      fprintf(stderr, XVR_CC_ERROR
              "Error: failed to allocate Xvr_Scope\n" XVR_CC_RESET);
      Xvr_popScope(scope);
      Xvr_freeBucket(&bucket);
      return -1;
    }

    Xvr_popScope(scope);
    Xvr_freeBucket(&bucket);
  }

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(XVR_BUCKET_IDEAL);
    Xvr_Scope *scope = NULL;

    for (int i = 0; i < 5; i++) {
      scope = Xvr_pushScope(&bucket, scope);
    }

    if (scope == NULL || scope->next == NULL || scope->table == NULL ||
        scope->table->capacity != 16 || scope->refCount != 1 ||
        scope->next->next == NULL || scope->next->table == NULL ||
        scope->next->table->capacity != 16 || scope->next->refCount != 2 ||
        scope->next->next->next == NULL || scope->next->next->table == NULL ||
        scope->next->next->table->capacity != 16 ||
        scope->next->next->refCount != 3 ||
        scope->next->next->next->next == NULL ||
        scope->next->next->next->table == NULL ||
        scope->next->next->next->table->capacity != 16 ||
        scope->next->next->next->next->refCount != 5 || false) {
      fprintf(stderr, XVR_CC_ERROR
              "Error: failed to allocate list of Xvr_scope\n" XVR_CC_RESET);
      while (scope) {
        scope = Xvr_popScope(scope);
      }

      Xvr_freeBucket(&bucket);
      return -1;
    }

    while (scope) {
      scope = Xvr_popScope(scope);
    }

    Xvr_freeBucket(&bucket);
  }

  return 0;
}

int main() {
  printf(XVR_CC_WARN "testing: xvr scope\n" XVR_CC_RESET);
  int total = 0, res = 0;

  {
    res = test_scope_allocation();
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "test_scope_allocation(): nice one cik\n" XVR_CC_RESET);
    }
    total += res;
  }

  return total;
}
