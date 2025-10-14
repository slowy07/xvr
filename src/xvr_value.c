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

#include "xvr_value.h"

#include <stdio.h>
#include <stdlib.h>

#include "xvr_array.h"
#include "xvr_console_colors.h"
#include "xvr_print.h"
#include "xvr_string.h"

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
        return hashUInt(*((int*)(&XVR_VALUE_AS_FLOAT(value))));

    case XVR_VALUE_STRING:
        return Xvr_hashString(XVR_VALUE_AS_STRING(value));

    case XVR_VALUE_ARRAY: {
        Xvr_Array* array = XVR_VALUE_AS_ARRAY(value);
        unsigned int hash = 0;

        for (unsigned int i = 0; i < array->count; i++) {
            hash ^= Xvr_hashValue(array->data[i]);
        }
        return hash;
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_UNKNOWN:
        fprintf(stderr, XVR_CC_ERROR
                "Error: can't hash an unknown value type, exit\n" XVR_CC_RESET);
        exit(-1);
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
        Xvr_String* string = XVR_VALUE_AS_STRING(value);
        return XVR_VALUE_FROM_STRING(Xvr_copyString(string));
    }

    case XVR_VALUE_ARRAY: {
        Xvr_Array* array = XVR_VALUE_AS_ARRAY(value);
        Xvr_Array* result = Xvr_resizeArray(NULL, array->capacity);

        for (unsigned int i = 0; i < array->count; i++) {
            result->data[i] = Xvr_copyValue(array->data[i]);
        }

        result->capacity = array->capacity;
        result->count = array->count;
        return XVR_VALUE_FROM_ARRAY(result);
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_UNKNOWN:
        fprintf(stderr, XVR_CC_ERROR
                "Error: can't copy unknown value type, exit\n" XVR_CC_RESET);
        exit(-1);
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
        Xvr_String* string = XVR_VALUE_AS_STRING(value);
        Xvr_freeString(string);
        break;
    }

    case XVR_VALUE_ARRAY: {
        Xvr_Array* array = XVR_VALUE_AS_ARRAY(value);

        for (unsigned int i = 0; i < array->count; i++) {
            Xvr_freeValue(array->data[i]);
        }
        XVR_ARRAY_FREE(array);
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_UNKNOWN:
        fprintf(stderr, XVR_CC_ERROR
                "Error: can't free an unknown value type, exit\n" XVR_CC_RESET);
        exit(-1);
    }
}

bool Xvr_checkValueIsTruthy(Xvr_Value value) {
    if (XVR_VALUE_IS_NULL(value)) {
        Xvr_error(XVR_CC_ERROR
                  "Error: `null` is neither true or false\n" XVR_CC_RESET);
        return false;
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

    case XVR_VALUE_ARRAY: {
        Xvr_Array* leftArray = XVR_VALUE_AS_ARRAY(left);
        Xvr_Array* rightArray = XVR_VALUE_AS_ARRAY(right);

        if (leftArray->count != rightArray->count) {
            return false;
        }

        for (unsigned int i = 0; i < leftArray->count; i++) {
            if (Xvr_checkValuesAreEqual(leftArray->data[i],
                                        rightArray->data[i])) {
                return false;
            }
        }
        return true;
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_UNKNOWN:
        fprintf(stderr, XVR_CC_ERROR
                "Error: unknown types in value equality, exit\n" XVR_CC_RESET);
        exit(-1);
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
        return false;

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_UNKNOWN:
        fprintf(stderr, XVR_CC_ERROR
                "unknown types in value comparison check, exit\n" XVR_CC_RESET);
        exit(-1);
    }

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
        break;

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_UNKNOWN:
        fprintf(stderr, XVR_CC_ERROR
                "unknown types in value comparison, exit\n" XVR_CC_RESET);
        exit(-1);
    }

    return -1;
}

Xvr_String* Xvr_stringifyValue(Xvr_Bucket** bucketHandle, Xvr_Value value) {
    switch (value.type) {
    case XVR_VALUE_NULL:
        return Xvr_createString(bucketHandle, "null");

    case XVR_VALUE_BOOLEAN:
        return Xvr_createString(bucketHandle,
                                XVR_VALUE_AS_BOOLEAN(value) ? "true" : "false");

    case XVR_VALUE_INTEGER: {
        char buffer[16];
        sprintf(buffer, "%d", XVR_VALUE_AS_INTEGER(value));
        return Xvr_createString(bucketHandle, buffer);
    }

    case XVR_VALUE_FLOAT: {
        char buffer[16];
        sprintf(buffer, "%f", XVR_VALUE_AS_FLOAT(value));
        return Xvr_createString(bucketHandle, buffer);
    }

    case XVR_VALUE_STRING: {
        return Xvr_copyString(XVR_VALUE_AS_STRING(value));
    }

    case XVR_VALUE_ARRAY: {
        Xvr_Array* array = XVR_VALUE_AS_ARRAY(value);
        Xvr_String* string = Xvr_createStringLength(bucketHandle, "[", 1);
        Xvr_String* comma = Xvr_createStringLength(bucketHandle, ",", 1);

        for (unsigned int i = 0; i < array->count; i++) {
            Xvr_String* tmp = Xvr_concatStrings(
                bucketHandle, string,
                Xvr_stringifyValue(bucketHandle, array->data[i]));
            Xvr_freeString(string);
            string = tmp;

            if (i + 1 < array->count) {
                Xvr_String* tmp =
                    Xvr_concatStrings(bucketHandle, string, comma);
                Xvr_freeString(string);
                string = tmp;
            }
        }
        Xvr_freeString(comma);
        return string;
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_UNKNOWN:
        fprintf(stderr, XVR_CC_ERROR
                "unknown types in value stringify, exiting\n" XVR_CC_RESET);
        exit(-1);
    }

    return NULL;
}

const char* Xvr_private_getValueTypeAsCString(Xvr_ValueType type) {
    switch (type) {
    case XVR_VALUE_NULL:
        return "null";
    case XVR_VALUE_BOOLEAN:
        return "bool";
    case XVR_VALUE_INTEGER:
        return "int";
    case XVR_VALUE_FLOAT:
        return "float";
    case XVR_VALUE_STRING:
        return "string";
    case XVR_VALUE_ARRAY:
        return "array";
    case XVR_VALUE_TABLE:
        return "table";
    case XVR_VALUE_FUNCTION:
        return "function";
    case XVR_VALUE_OPAQUE:
        return "opaque";
    case XVR_VALUE_TYPE:
        return "type";
    case XVR_VALUE_ANY:
        return "any";
    case XVR_VALUE_UNKNOWN:
        return "unknown";
    }

    return NULL;
}
