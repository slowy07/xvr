#include "xvr_string.h"
#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_value.h"

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

static unsigned int hashCString(const char *string) {
  unsigned int hash = 2166136261u;

  for (unsigned int i = 0; string[i]; i++) {
    hash *= string[i];
    hash ^= 16777619;
  }

  return hash;
}

// exposed functions
Xvr_String *Xvr_createString(Xvr_Bucket **bucket, const char *cstring) {
  int length = strlen(cstring);

  return Xvr_createStringLength(bucket, cstring, length);
}

Xvr_String *Xvr_createStringLength(Xvr_Bucket **bucket, const char *cstring,
                                   int length) {

  if (length > XVR_STRING_MAX_LENGTH) {
    fprintf(stderr,
            XVR_CC_ERROR
            "Error: can't create string longer than %d\n" XVR_CC_RESET,
            XVR_STRING_MAX_LENGTH);
    exit(-1);
  }

  Xvr_String *ret = (Xvr_String *)Xvr_partitionBucket(
      bucket, sizeof(Xvr_String) + length + 1);

  ret->type = XVR_STRING_LEAF;
  ret->length = length;
  ret->refCount = 1;
  ret->cachedHash = 0;
  memcpy(ret->as.leaf.data, cstring, length + 1);
  ret->as.leaf.data[length] = '\0';

  return ret;
}

XVR_API Xvr_String *Xvr_createNameString(Xvr_Bucket **bucketHandle,
                                         const char *cname,
                                         Xvr_ValueType type) {
  int length = strlen(cname);

  if (length > XVR_STRING_MAX_LENGTH) {
    fprintf(stderr,
            XVR_CC_ERROR
            "error: can't create name string longer than %d\n" XVR_CC_RESET,
            XVR_STRING_MAX_LENGTH);
    exit(-1);
  }

  Xvr_String *ret = (Xvr_String *)Xvr_partitionBucket(
      bucketHandle, sizeof(Xvr_String) + length + 1);

  ret->type = XVR_STRING_NAME;
  ret->length = length;
  ret->refCount = 1;
  ret->cachedHash = 0;
  memcpy(ret->as.name.data, cname, length + 1);
  ret->as.name.data[length] = '\0';
  ret->as.name.type = type;

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

  if (str->type == XVR_STRING_NODE || str->type == XVR_STRING_LEAF) {
    ret->type = XVR_STRING_LEAF;
    ret->length = str->length;
    ret->refCount = 1;
    ret->cachedHash = 0;
    deepCopyUtil(ret->as.leaf.data, str); // copy each leaf into the buffer
    ret->as.leaf.data[ret->length] = '\0';
  } else {
    ret->type = XVR_STRING_NAME;
    ret->length = str->length;
    ret->refCount = 1;
    ret->cachedHash = 0;
    memcpy(ret->as.name.data, str->as.name.data, str->length + 1);
    ret->as.name.data[ret->length] = '\0';
  }

  return ret;
}

Xvr_String *Xvr_concatStrings(Xvr_Bucket **bucket, Xvr_String *left,
                              Xvr_String *right) {

  if (left->type == XVR_STRING_NAME || right->type == XVR_STRING_NAME) {
    fprintf(stderr,
            XVR_CC_ERROR "Error: can't concat name string\n" XVR_CC_RESET);
    exit(-1);
  }

  if (left->refCount == 0 || right->refCount == 0) {
    fprintf(stderr, XVR_CC_ERROR "ERROR: Can't concatenate a string with "
                                 "refcount of zero\n" XVR_CC_RESET);
    exit(-1);
  }

  if (left->length + right->length > XVR_STRING_MAX_LENGTH) {
    fprintf(stderr,
            XVR_CC_ERROR
            "Error: can't concat string longer than %d\n" XVR_CC_RESET,
            XVR_STRING_MAX_LENGTH);
    exit(-1);
  }

  Xvr_String *ret =
      (Xvr_String *)Xvr_partitionBucket(bucket, sizeof(Xvr_String));

  ret->type = XVR_STRING_NODE;
  ret->length = left->length + right->length;
  ret->refCount = 1;
  ret->cachedHash = 0;
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
  if (str->type == XVR_STRING_NAME) {
    fprintf(stderr, XVR_CC_ERROR
            "Error: can't get raw string buffer of name string\n" XVR_CC_RESET);
    exit(-1);
  }

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

static int deepCompareUtil(Xvr_String *left, Xvr_String *right,
                           const char **leftHead, const char **rightHead) {
  int result = 0;

  if (left == right) {
    return result;
  }

  if (left->type == XVR_STRING_LEAF && (*leftHead) != NULL &&
      (**leftHead) != '\0' &&
      ((*leftHead) < left->as.leaf.data ||
       (*leftHead) > (left->as.leaf.data + strlen(left->as.leaf.data)))) {
    return result;
  }

  if (right->type == XVR_STRING_LEAF && (*rightHead) != NULL &&
      (**rightHead) != '\0' &&
      ((*rightHead) < right->as.leaf.data ||
       (*rightHead) > (right->as.leaf.data + strlen(right->as.leaf.data)))) {
    return result;
  }

  if (left->type == XVR_STRING_NODE) {
    if ((result = deepCompareUtil(left->as.node.left, right, leftHead,
                                  rightHead)) != 0) {
      return result;
    }

    if ((result = deepCompareUtil(left->as.node.right, right, leftHead,
                                  rightHead)) != 0) {
      return result;
    }

    return result;
  }

  if (right->type == XVR_STRING_NODE) {
    if ((result = deepCompareUtil(left, right->as.node.left, leftHead,
                                  rightHead)) != 0) {
      return result;
    }
    if ((result = deepCompareUtil(left, right->as.node.right, leftHead,
                                  rightHead)) != 0) {
      return result;
    }

    return result;
  }

  if (left->type == XVR_STRING_LEAF && right->type == XVR_STRING_LEAF) {
    if ((*leftHead) == NULL || (**leftHead) == '\0') {
      (*leftHead) = left->as.leaf.data;
    }

    if ((*rightHead) == NULL || (**rightHead) == '\0') {
      (*rightHead) = right->as.leaf.data;
    }

    while (**leftHead && (**leftHead == **rightHead)) {
      (*leftHead)++;
      (*rightHead)++;
    }

    if ((**leftHead == '\0' || **rightHead == '\0') == false) {
      result = *(const unsigned char *)(*leftHead) -
               *(const unsigned char *)(*rightHead);
    }
  }

  return result;
}

int Xvr_compareStrings(Xvr_String *left, Xvr_String *right) {
  if (left->length == 0 || right->length == 0) {
    return left->length - right->length;
  }

  if (left->type == XVR_STRING_NAME || right->type == XVR_STRING_NAME) {
    if (left->type != right->type) {
      fprintf(
          stderr, XVR_CC_ERROR
          "Error: can't compare name string to non-name string\n" XVR_CC_RESET);
      exit(-1);
    }

    return strcmp(left->as.name.data, right->as.name.data);
  }

  const char *leftHead = NULL;
  const char *rightHead = NULL;

  return deepCompareUtil(left, right, &leftHead, &rightHead);
}

unsigned int Xvr_hashString(Xvr_String *str) {

  if (str->cachedHash != 0) {
    return str->cachedHash;
  } else if (str->type == XVR_STRING_NODE) {
    char *buffer = Xvr_getStringRawBuffer(str);
    str->cachedHash = hashCString(str->as.leaf.data);
    free(buffer);
  } else if (str->type == XVR_STRING_LEAF) {
    str->cachedHash = hashCString(str->as.leaf.data);
  } else if (str->type == XVR_STRING_NAME) {
    str->cachedHash = hashCString(str->as.name.data);
  }

  return str->cachedHash;
}
