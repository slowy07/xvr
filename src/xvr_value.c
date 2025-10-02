#include "xvr_value.h"
#include "xvr_console_colors.h"
#include "xvr_print.h"
#include "xvr_string.h"
#include <stdlib.h>

bool Xvr_private_isTruthy(Xvr_Value value) {
  if (XVR_VALUE_IS_NULL(value)) {
    Xvr_error(XVR_CC_ERROR
              "Error: `null` is neither true or false\n" XVR_CC_RESET);
  }

  // only 'false' is falsy
  if (XVR_VALUE_IS_BOOLEAN(value)) {
    return XVR_VALUE_AS_BOOLEAN(value);
  }

  // anything else is truthy
  return true;
}

bool Xvr_private_isEqual(Xvr_Value left, Xvr_Value right) {
  // temp check
  if (right.type > XVR_VALUE_STRING) {
    Xvr_error(
        XVR_CC_ERROR
        "Error: unknown types in value equality comparison\n" XVR_CC_RESET);
  }

  switch (left.type) {
  case XVR_VALUE_NULL:
    return XVR_VALUE_IS_NULL(right);

  case XVR_VALUE_BOOLEAN:
    return XVR_VALUE_IS_BOOLEAN(right) &&
           XVR_VALUE_AS_BOOLEAN(left) == XVR_VALUE_AS_BOOLEAN(right);

  case XVR_VALUE_INTEGER:
    if (XVR_VALUE_AS_INTEGER(right)) {
      return XVR_VALUE_AS_INTEGER(left) == XVR_VALUE_AS_INTEGER(right);
    }
    if (XVR_VALUE_AS_FLOAT(right)) {
      return XVR_VALUE_AS_INTEGER(left) == XVR_VALUE_AS_FLOAT(right);
    }
    return false;

  case XVR_VALUE_FLOAT:
    if (XVR_VALUE_AS_FLOAT(right)) {
      return XVR_VALUE_AS_FLOAT(left) == XVR_VALUE_AS_FLOAT(right);
    }
    if (XVR_VALUE_AS_INTEGER(right)) {
      return XVR_VALUE_AS_FLOAT(left) == XVR_VALUE_AS_INTEGER(right);
    }
    return false;

  case XVR_VALUE_STRING:
    if (XVR_VALUE_IS_STRING(right)) {
      return Xvr_compareStrings(XVR_VALUE_AS_STRING(left),
                                XVR_VALUE_AS_STRING(right)) == 0;
    }
    return false;
  case XVR_VALUE_ARRAY:
  case XVR_VALUE_DICTIONARY:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
  default:
    Xvr_error(
        XVR_CC_ERROR
        "Error: unknown types in value equality comparison\n" XVR_CC_RESET);
  }
  return 0;
}

// hash utils
static unsigned int hashCString(const char *string) {
  unsigned int hash = 2166136261u;

  for (unsigned int i = 0; string[i]; i++) {
    hash *= string[i];
    hash ^= 16777619;
  }

  return hash;
}

static unsigned int hashUInt(unsigned int x) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return x;
}

unsigned int Xvr_hashValue(Xvr_Value value) {
  switch (value.type) {
  case XVR_VALUE_NULL:
    return 0;

  case XVR_VALUE_BOOLEAN:
    return XVR_VALUE_AS_BOOLEAN(value) ? 1 : 0;

  case XVR_VALUE_INTEGER:
    return hashUInt(XVR_VALUE_AS_INTEGER(value));

  case XVR_VALUE_STRING: {
    Xvr_String *str = XVR_VALUE_AS_STRING(value);

    if (str->cachedHash != 0) {
      return str->cachedHash;
    } else if (str->type == XVR_STRING_NODE) {
      char *buffer = Xvr_getStringRawBuffer(str);
      str->cachedHash = hashCString(buffer);
      free(buffer);
    } else if (str->type == XVR_STRING_LEAF) {
      str->cachedHash = hashCString(str->as.leaf.data);
    } else if (str->type == XVR_STRING_NAME) {
      str->cachedHash = hashCString(str->as.name.data);
    }

    return str->cachedHash;
  }

  case XVR_VALUE_FLOAT:
    return hashUInt(*((int *)(&XVR_VALUE_AS_FLOAT(value))));

  case XVR_VALUE_ARRAY:
  case XVR_VALUE_DICTIONARY:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
  default:
    Xvr_error(XVR_CC_ERROR "Error: cant't hash unknown type\n" XVR_CC_RESET);
  }
  return 0;
}
