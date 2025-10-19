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
#include <string.h>

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

Xvr_Value Xvr_unwrapValue(Xvr_Value value) {
    if (value.type == XVR_VALUE_REFERENCE) {
        return Xvr_unwrapValue(*(value.as.reference));
    } else {
        return value;
    }
}

unsigned int Xvr_hashValue(Xvr_Value value) {
    value = Xvr_unwrapValue(value);

    switch (value.type) {
    case XVR_VALUE_NULL:
        return 0;

    case XVR_VALUE_BOOLEAN:
        return value.as.boolean ? 1 : 0;

    case XVR_VALUE_INTEGER:
        return hashUInt((unsigned int)value.as.integer);

    case XVR_VALUE_FLOAT:
        return hashUInt(*((unsigned int*)(&value.as.number)));

    case XVR_VALUE_STRING:
        return Xvr_hashString(value.as.string);

    case XVR_VALUE_ARRAY: {
        Xvr_Array* ptr = value.as.array;
        unsigned int hash = 0;

        for (unsigned int i = 0; i < ptr->count; i++) {
            hash ^= Xvr_hashValue(ptr->data[i]);
        }

        return hash;
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_REFERENCE:
    case XVR_VALUE_UNKNOWN:
        fprintf(
            stderr, XVR_CC_ERROR
            "ERROR: Can't hash an unknown value type, exiting\n" XVR_CC_RESET);
        exit(-1);
    }

    return 0;
}

Xvr_Value Xvr_copyValue(Xvr_Value value) {
    value = Xvr_unwrapValue(value);

    switch (value.type) {
    case XVR_VALUE_NULL:
    case XVR_VALUE_BOOLEAN:
    case XVR_VALUE_INTEGER:
    case XVR_VALUE_FLOAT:
        return value;

    case XVR_VALUE_STRING: {
        return XVR_VALUE_FROM_STRING(Xvr_copyString(value.as.string));
    }

    case XVR_VALUE_ARRAY: {
        Xvr_Array* ptr = value.as.array;
        Xvr_Array* result = Xvr_resizeArray(NULL, ptr->capacity);

        for (unsigned int i = 0; i < ptr->count; i++) {
            result->data[i] = Xvr_copyValue(ptr->data[i]);
        }

        result->capacity = ptr->capacity;
        result->count = ptr->count;

        return XVR_VALUE_FROM_ARRAY(result);
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_REFERENCE:
    case XVR_VALUE_UNKNOWN:
        fprintf(
            stderr, XVR_CC_ERROR
            "ERROR: Can't copy an unknown value type, exiting\n" XVR_CC_RESET);
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
        Xvr_freeString(value.as.string);
        break;
    }

    case XVR_VALUE_ARRAY: {
        Xvr_Array* ptr = value.as.array;

        for (unsigned int i = 0; i < ptr->count; i++) {
            Xvr_freeValue(ptr->data[i]);
        }

        XVR_ARRAY_FREE(ptr);
        break;
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_REFERENCE:
    case XVR_VALUE_UNKNOWN:
        fprintf(
            stderr, XVR_CC_ERROR
            "ERROR: Can't free an unknown value type, exiting\n" XVR_CC_RESET);
        exit(-1);
    }
}

bool Xvr_checkValueIsTruthy(Xvr_Value value) {
    value = Xvr_unwrapValue(value);

    if (value.type == XVR_VALUE_NULL) {
        Xvr_error("'null' is neither true nor false");
        return false;
    }

    // only 'false' is falsy
    if (value.type == XVR_VALUE_BOOLEAN) {
        return value.as.boolean;
    }

    // anything else is truthy
    return true;
}

bool Xvr_checkValuesAreEqual(Xvr_Value left, Xvr_Value right) {
    left = Xvr_unwrapValue(left);
    right = Xvr_unwrapValue(right);

    switch (left.type) {
    case XVR_VALUE_NULL:
        return right.type == XVR_VALUE_NULL;

    case XVR_VALUE_BOOLEAN:
        return right.type == XVR_VALUE_NULL &&
               left.as.boolean == right.as.boolean;

    case XVR_VALUE_INTEGER:
        if (right.type == XVR_VALUE_INTEGER) {
            return left.as.integer == right.as.integer;
        } else if (right.type == XVR_VALUE_FLOAT) {
            return left.as.integer == right.as.number;
        } else {
            break;
        }

    case XVR_VALUE_FLOAT:
        if (right.type == XVR_VALUE_INTEGER) {
            return left.as.number == right.as.integer;
        } else if (right.type == XVR_VALUE_FLOAT) {
            return left.as.number == right.as.number;
        } else {
            break;
        }

    case XVR_VALUE_STRING:
        if (right.type == XVR_VALUE_STRING) {
            return Xvr_compareStrings(left.as.string, right.as.string) == 0;
        } else {
            break;
        }

    case XVR_VALUE_ARRAY: {
        if (right.type == XVR_VALUE_ARRAY) {
            Xvr_Array* leftArray = left.as.array;
            Xvr_Array* rightArray = right.as.array;

            if (leftArray->count != rightArray->count) {
                return false;
            }

            for (unsigned int i = 0; i < leftArray->count; i++) {
                if (Xvr_checkValuesAreEqual(leftArray->data[i],
                                            rightArray->data[i]) != true) {
                    return false;
                }
            }
        } else {
            break;
        }

        return true;
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_REFERENCE:
    case XVR_VALUE_UNKNOWN:
        fprintf(
            stderr, XVR_CC_ERROR
            "ERROR: Unknown types in value equality, exiting\n" XVR_CC_RESET);
        exit(-1);
    }

    return false;
}

bool Xvr_checkValuesAreComparable(Xvr_Value left, Xvr_Value right) {
    left = Xvr_unwrapValue(left);
    right = Xvr_unwrapValue(right);
    switch (left.type) {
    case XVR_VALUE_NULL:
        return false;

    case XVR_VALUE_BOOLEAN:
        return right.type == XVR_VALUE_BOOLEAN;

    case XVR_VALUE_INTEGER:
    case XVR_VALUE_FLOAT:
        return right.type == XVR_VALUE_INTEGER || right.type == XVR_VALUE_FLOAT;

    case XVR_VALUE_STRING:
        return right.type == XVR_VALUE_STRING;

    case XVR_VALUE_ARRAY:
        return false;

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_REFERENCE:
    case XVR_VALUE_UNKNOWN:
        fprintf(
            stderr, XVR_CC_ERROR
            "Unknown types in value comparison check, exiting\n" XVR_CC_RESET);
        exit(-1);
    }

    return false;
}

int Xvr_compareValues(Xvr_Value left, Xvr_Value right) {
    left = Xvr_unwrapValue(left);
    right = Xvr_unwrapValue(right);
    switch (left.type) {
    case XVR_VALUE_NULL:
    case XVR_VALUE_BOOLEAN:
        break;

    case XVR_VALUE_INTEGER:
        if (right.type == XVR_VALUE_INTEGER) {
            return left.as.integer - right.as.integer;
        } else if (right.type == XVR_VALUE_FLOAT) {
            return left.as.integer - right.as.number;
        } else {
            break;
        }

    case XVR_VALUE_FLOAT:
        if (right.type == XVR_VALUE_INTEGER) {
            return left.as.number - right.as.integer;
        } else if (right.type == XVR_VALUE_FLOAT) {
            return left.as.number - right.as.number;
        } else {
            break;
        }

    case XVR_VALUE_STRING:
        if (right.type == XVR_VALUE_STRING) {
            return Xvr_compareStrings(left.as.string, right.as.string);
        }

    case XVR_VALUE_ARRAY:
        break;

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_REFERENCE:
    case XVR_VALUE_UNKNOWN:
        break;
    }

    fprintf(stderr, XVR_CC_ERROR
            "Unknown types in value comparison, exiting\n" XVR_CC_RESET);
    exit(-1);

    return ~0;
}

Xvr_String* Xvr_stringifyValue(Xvr_Bucket** bucketHandle, Xvr_Value value) {
    value = Xvr_unwrapValue(value);

    switch (value.type) {
    case XVR_VALUE_NULL:
        return Xvr_createString(bucketHandle, "null");

    case XVR_VALUE_BOOLEAN:
        return Xvr_createString(bucketHandle,
                                value.as.boolean ? "true" : "false");

    case XVR_VALUE_INTEGER: {
        char buffer[16];
        sprintf(buffer, "%d", value.as.integer);
        return Xvr_createString(bucketHandle, buffer);
    }

    case XVR_VALUE_FLOAT: {
        char buffer[16];
        sprintf(buffer, "%f", value.as.number);

        unsigned int length = strlen(buffer);

        unsigned int decimal = 0;
        while (decimal != length && buffer[decimal] != '.') decimal++;

        while (decimal != length && buffer[length - 1] == '0')
            buffer[--length] = '\0';

        return Xvr_createStringLength(bucketHandle, buffer, length);
    }

    case XVR_VALUE_STRING:
        return Xvr_copyString(value.as.string);

    case XVR_VALUE_ARRAY: {
        Xvr_Array* ptr = value.as.array;
        Xvr_String* string = Xvr_createStringLength(bucketHandle, "[", 1);
        Xvr_String* comma = Xvr_createStringLength(bucketHandle, ",", 1);

        for (unsigned int i = 0; i < ptr->count; i++) {
            Xvr_String* tmp = Xvr_concatStrings(
                bucketHandle, string,
                Xvr_stringifyValue(bucketHandle, ptr->data[i]));
            Xvr_freeString(string);
            string = tmp;

            if (i + 1 < ptr->count) {
                Xvr_String* tmp =
                    Xvr_concatStrings(bucketHandle, string, comma);
                Xvr_freeString(string);
                string = tmp;
            }
        }

        Xvr_String* tmp = Xvr_concatStrings(
            bucketHandle, string, Xvr_createStringLength(bucketHandle, "]", 1));
        Xvr_freeString(string);
        string = tmp;

        Xvr_freeString(comma);

        return string;
    }

    case XVR_VALUE_TABLE:
    case XVR_VALUE_FUNCTION:
    case XVR_VALUE_OPAQUE:
    case XVR_VALUE_TYPE:
    case XVR_VALUE_ANY:
    case XVR_VALUE_REFERENCE:
    case XVR_VALUE_UNKNOWN:
        fprintf(stderr, XVR_CC_ERROR
                "Unknown types in value stringify, exiting\n" XVR_CC_RESET);
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
    case XVR_VALUE_REFERENCE:
        return "reference";
    case XVR_VALUE_UNKNOWN:
        return "unknown";
    }

    return NULL;
}
