#include "xvr_value.h"
#include "xvr_console_colors.h"
#include "xvr_print.h"
#include "xvr_string.h"
#include <stdlib.h>

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

  case XVR_VALUE_FLOAT:
    return hashUInt(*((int *)(&XVR_VALUE_AS_FLOAT(value))));

  case XVR_VALUE_STRING:
    return Xvr_hashString(XVR_VALUE_AS_STRING(value));

  case XVR_VALUE_ARRAY:
  case XVR_VALUE_TABLE:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
  case XVR_VALUE_TYPE:
  case XVR_VALUE_ANY:
  case XVR_VALUE_UNKNOWN:
    break;
  }
  return 0;
}

Xvr_Value Xvr_copyValue(Xvr_Value value) {
  switch (value.type) {
  case XVR_VALUE_NULL:
  case XVR_VALUE_BOOLEAN:
  case XVR_VALUE_INTEGER:
  case XVR_VALUE_FLOAT:
    return value;

  case XVR_VALUE_STRING: {
    Xvr_String *string = XVR_VALUE_AS_STRING(value);
    return XVR_VALUE_FROM_STRING(Xvr_copyString(string));
  }

  case XVR_VALUE_ARRAY:
  case XVR_VALUE_TABLE:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
  case XVR_VALUE_TYPE:
  case XVR_VALUE_ANY:
  case XVR_VALUE_UNKNOWN:
    break;
  }
  return XVR_VALUE_FROM_NULL();
}

void Xvr_freeValue(Xvr_Value value) {
  switch (value.type) {
  case XVR_VALUE_NULL:
  case XVR_VALUE_BOOLEAN:
  case XVR_VALUE_INTEGER:
  case XVR_VALUE_FLOAT:
    break;

  case XVR_VALUE_STRING: {
    Xvr_String *string = XVR_VALUE_AS_STRING(value);
    Xvr_freeString(string);
    break;
  }

  case XVR_VALUE_ARRAY:
  case XVR_VALUE_TABLE:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
  case XVR_VALUE_TYPE:
  case XVR_VALUE_ANY:
  case XVR_VALUE_UNKNOWN:
    Xvr_error(XVR_CC_ERROR "Error: can't free an unknown type\n" XVR_CC_RESET);
  }
}

bool Xvr_checkValueIsTruthy(Xvr_Value value) {
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

bool Xvr_checkValuesAreEqual(Xvr_Value left, Xvr_Value right) {
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
  case XVR_VALUE_TABLE:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
  case XVR_VALUE_TYPE:
  case XVR_VALUE_ANY:
  case XVR_VALUE_UNKNOWN:
    break;
  }
  return 0;
}

bool Xvr_checkValuesAreCompareable(Xvr_Value left, Xvr_Value right) {
  switch (left.type) {
  case XVR_VALUE_NULL:
    return false;

  case XVR_VALUE_BOOLEAN:
    return XVR_VALUE_IS_BOOLEAN(right);

  case XVR_VALUE_INTEGER:
  case XVR_VALUE_FLOAT:
    return XVR_VALUE_IS_INTEGER(right) || XVR_VALUE_IS_FLOAT(right);

  case XVR_VALUE_STRING:
    return XVR_VALUE_IS_STRING(right);

  case XVR_VALUE_ARRAY:
  case XVR_VALUE_TABLE:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
  case XVR_VALUE_TYPE:
  case XVR_VALUE_ANY:
  case XVR_VALUE_UNKNOWN:
    break;
  }

  Xvr_error(XVR_CC_ERROR
            "Error: unknown types in value in comparison check\n" XVR_CC_RESET);

  return false;
}

int Xvr_compareValues(Xvr_Value left, Xvr_Value right) {
  switch (left.type) {
  case XVR_VALUE_NULL:
  case XVR_VALUE_BOOLEAN:
    break;

  case XVR_VALUE_INTEGER:
    if (XVR_VALUE_IS_INTEGER(right)) {
      return XVR_VALUE_AS_INTEGER(left) - XVR_VALUE_AS_INTEGER(right);
    } else if (XVR_VALUE_IS_FLOAT(right)) {
      return XVR_VALUE_AS_INTEGER(left) - XVR_VALUE_AS_FLOAT(right);

    } else {
      break;
    }

  case XVR_VALUE_FLOAT:
    if (XVR_VALUE_IS_INTEGER(right)) {
      return XVR_VALUE_AS_FLOAT(left) - XVR_VALUE_AS_INTEGER(right);
    } else if (XVR_VALUE_IS_FLOAT(right)) {
      return XVR_VALUE_AS_FLOAT(left) - XVR_VALUE_AS_FLOAT(right);
    } else {
      break;
    }

  case XVR_VALUE_STRING:
    if (XVR_VALUE_IS_STRING(right)) {
      return Xvr_compareStrings(XVR_VALUE_AS_STRING(left),
                                XVR_VALUE_AS_STRING(right));
    }

  case XVR_VALUE_ARRAY:
  case XVR_VALUE_TABLE:
  case XVR_VALUE_FUNCTION:
  case XVR_VALUE_OPAQUE:
  case XVR_VALUE_TYPE:
  case XVR_VALUE_ANY:
  case XVR_VALUE_UNKNOWN:
    break;
  }

  Xvr_error(XVR_CC_ERROR
            "Error: unknown types in value comparison\n" XVR_CC_RESET);
  return -1;
}
