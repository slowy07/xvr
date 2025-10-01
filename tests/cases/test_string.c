#include "xvr_bucket.h"
#include "xvr_console_colors.h"
#include "xvr_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_sizeof_string_64bit() {
  {
    if (sizeof(Xvr_String) != 32) {
      fprintf(stderr,
              XVR_CC_ERROR "Error: `Xvr_String` unexpected size in memory: "
                           "expected 32, found %d \n" XVR_CC_RESET,
              (int)sizeof(Xvr_String));
      return -1;
    }
  }
  return 0;
}

int test_sizeof_string_32bit() {
  {
    if (sizeof(Xvr_String) != 24) {
      fprintf(stderr,
              XVR_CC_ERROR "Error: `Xvr_String` unexpected size in memory: "
                           "expected 24, found %d \n" XVR_CC_RESET,
              (int)sizeof(Xvr_String));
      return -1;
    }
  }
  return 0;
}

int test_string_equal() {
  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(1024);
    Xvr_String *helloWorld = Xvr_createString(&bucket, "hello world");
    Xvr_String *helloWorldC = Xvr_createString(&bucket, "hello world");

    int result = 0;

    if ((result = Xvr_compareString(helloWorld, helloWorldC)) != 0) {
      char *leftBuffer = Xvr_getStringRawBuffer(helloWorld);
      char *rightBuffer = Xvr_getStringRawBuffer(helloWorldC);
      fprintf(
          stderr,
          XVR_CC_ERROR
          "Error: string equal `%s` == `%s` incorrect, found %s\n" XVR_CC_RESET,
          leftBuffer, rightBuffer,
          result < 0    ? "<"
          : result == 0 ? "=="
                        : ">");
      free(leftBuffer);
      free(rightBuffer);
      Xvr_freeBucket(&bucket);
      return -1;
    }
  }

  return 0;
}

int test_string_allocation() {
  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(1024);
    const char *cstring = "hello world";
    Xvr_String *str = Xvr_createString(&bucket, cstring);

    if (str->type != XVR_STRING_LEAF || str->length != 11 ||
        str->refCount != 1 || strcmp(str->as.leaf.data, "hello world") != 0) {
      fprintf(stderr, XVR_CC_ERROR "Error: failed to allocate `Xvr_String` "
                                   "with private bucket\n" XVR_CC_RESET);
      Xvr_freeBucket(&bucket);
      return -1;
    }

    Xvr_freeString(str);

    if (bucket->capacity != 1024 || bucket->count != sizeof(Xvr_String) + 12 ||
        bucket->next != NULL) {
      fprintf(stderr, XVR_CC_ERROR "Error: unexpected bucket state after "
                                   "string was free\n" XVR_CC_RESET);
      Xvr_freeBucket(&bucket);
      return -1;
    }

    if (Xvr_getStringRefCount(str) != 0) {
      fprintf(
          stderr, XVR_CC_ERROR
          "Error: unexpected string state after it was free\n" XVR_CC_RESET);
      Xvr_freeBucket(&bucket);
      return -1;
    }

    Xvr_freeBucket(&bucket);
  }

  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(1024);

    const char *cstring = "hello world";
    Xvr_String *str = Xvr_createNameString(&bucket, cstring);

    Xvr_String *shallow = Xvr_copyString(&bucket, str);
    Xvr_String *deep = Xvr_deepCopyString(&bucket, str);

    if (str != shallow || str == deep || shallow->refCount != 2 ||
        deep->refCount != 1 ||
        strcmp(shallow->as.name.data, deep->as.name.data) != 0) {
      fprintf(stderr,
              XVR_CC_ERROR "Error: failed copy name string \n" XVR_CC_RESET);
      Xvr_freeBucket(&bucket);
      return -1;
    }
  }

  return 0;
}

int test_string_diffs() {
  {
    Xvr_Bucket *bucket = Xvr_allocateBucket(1024);
    Xvr_String *pangram = Xvr_concatStrings(
        &bucket, Xvr_createString(&bucket, "The quick brown "),
        Xvr_concatStrings(&bucket, Xvr_createString(&bucket, "fox jumps o"),
                          Xvr_createString(&bucket, "ver the lazy dog.")));

    Xvr_String *neckbeard = Xvr_concatStrings(
        &bucket, Xvr_createString(&bucket, "The quick brown fox jumps over"),
        Xvr_concatStrings(&bucket, Xvr_createString(&bucket, "the lazy"),
                          Xvr_createString(&bucket, "dog.")));

    int result = 0;

    if (((result = Xvr_compareString(pangram, neckbeard)) < 0) == false) {
      char *leftBuffer = Xvr_getStringRawBuffer(pangram);
      char *rightBuffer = Xvr_getStringRawBuffer(neckbeard);
      fprintf(
          stderr,
          XVR_CC_ERROR
          "Error: string diff `%s` == `%s` incorrect, found %s\n" XVR_CC_RESET,
          leftBuffer, rightBuffer,
          result < 0    ? "<"
          : result == 0 ? "=="
                        : ">");
      free(leftBuffer);
      free(rightBuffer);
      Xvr_freeBucket(&bucket);
      return -1;
    }

    Xvr_freeBucket(&bucket);
  }

  return 0;
}

int main() {
  printf(XVR_CC_WARN "testing: xvr string\n" XVR_CC_RESET);
  int total = 0, res = 0;

  {
#if XVR_BITNESS == 64
    res = test_sizeof_string_64bit();
#else
    res = test_sizeof_string_32bit();
#endif /* if XVR_BITNESS == 64 */
    if (res == 0) {
      printf(XVR_CC_NOTICE "test_sizeof(): nice one reks\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    res = test_string_equal();
    if (res == 0) {
      printf(XVR_CC_NOTICE "test_string_equal(): nice one reks\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    res = test_string_allocation();
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "test_string_allocation(): nice one aman rek\n" XVR_CC_RESET);
    }
    total += res;
  }

  {
    res = test_string_diffs();
    if (res == 0) {
      printf(XVR_CC_NOTICE
             "test_string_diffs(): nice one loh ya rek\n" XVR_CC_RESET);
    }
    total += res;
  }

  return total;
}
