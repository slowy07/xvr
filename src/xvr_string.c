#include "xvr_string.h"
#include "xvr_console_colors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utils
static void deepCopyUtil(char *dest, Xvr_String *str) {
  if (str->type == XVR_STRING_NODE) {
    deepCopyUtil(dest, str->as.node.left);
    deepCopyUtil(dest + str->as.node.left->length, str->as.node.right);
  }

  else {
    memcpy(dest, str->as.leaf.data, str->length);
  }
}

static void incrementRefCount(Xvr_String *str) {
  str->refCount++;
  if (str->type == XVR_STRING_NODE) {
    incrementRefCount(str->as.node.left);
    incrementRefCount(str->as.node.right);
  }
}

static void decrementRefCount(Xvr_String *str) {
  str->refCount--;
  if (str->type == XVR_STRING_NODE) {
    decrementRefCount(str->as.node.left);
    decrementRefCount(str->as.node.right);
  }
}

// exposed functions
Xvr_String *Xvr_createString(Xvr_Bucket **bucket, const char *cstring) {
  int length = strlen(cstring);

  return Xvr_createStringLength(bucket, cstring, length);
}

Xvr_String *Xvr_createStringLength(Xvr_Bucket **bucket, const char *cstring,
                                   int length) {
  Xvr_String *ret = (Xvr_String *)Xvr_partitionBucket(
      bucket, sizeof(Xvr_String) + length + 1);

  ret->type = XVR_STRING_LEAF;
  ret->length = length;
  ret->refCount = 1;
  memcpy(ret->as.leaf.data, cstring, length);
  ret->as.leaf.data[length] = '\0';

  return ret;
}

Xvr_String *Xvr_copyString(Xvr_Bucket **bucket, Xvr_String *str) {
  if (str->refCount == 0) {
    fprintf(stderr, XVR_CC_ERROR
            "ERROR: Can't copy a string with refcount of zero\n" XVR_CC_RESET);
    exit(-1);
  }
  incrementRefCount(str);
  return str;
}

Xvr_String *Xvr_deepCopyString(Xvr_Bucket **bucket, Xvr_String *str) {
  if (str->refCount == 0) {
    fprintf(
        stderr, XVR_CC_ERROR
        "ERROR: Can't deep copy a string with refcount of zero\n" XVR_CC_RESET);
    exit(-1);
  }
  Xvr_String *ret = (Xvr_String *)Xvr_partitionBucket(
      bucket, sizeof(Xvr_String) + str->length + 1);
  ret->type = XVR_STRING_LEAF;
  ret->length = str->length;
  ret->refCount = 1;
  deepCopyUtil(ret->as.leaf.data, str); // copy each leaf into the buffer
  ret->as.leaf.data[ret->length] = '\0';

  return ret;
}

Xvr_String *Xvr_concatString(Xvr_Bucket **bucket, Xvr_String *left,
                             Xvr_String *right) {
  if (left->refCount == 0 || right->refCount == 0) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Can't concatenate a string with "
                                 "refcount of zero\n" XVR_CC_RESET);
    exit(-1);
  }

  Xvr_String *ret =
      (Xvr_String *)Xvr_partitionBucket(bucket, sizeof(Xvr_String));

  ret->type = XVR_STRING_NODE;
  ret->length = left->length + right->length;
  ret->refCount = 1;
  ret->as.node.left = left;
  ret->as.node.right = right;

  incrementRefCount(left);
  incrementRefCount(right);

  return ret;
}

void Xvr_freeString(Xvr_String *str) { decrementRefCount(str); }

int Xvr_getStringLength(Xvr_String *str) { return str->length; }

int Xvr_getStringRefCount(Xvr_String *str) { return str->refCount; }

char *Xvr_getStringRawBuffer(Xvr_String *str) {
  if (str->refCount == 0) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Can't get raw string buffer of a "
                                 "string with refcount of zero\n" XVR_CC_RESET);
    exit(-1);
  }

  char *buffer = malloc(str->length + 1);

  deepCopyUtil(buffer, str);
  buffer[str->length] = '\0';

  return buffer;
}
