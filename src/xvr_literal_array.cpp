#include "xvr_literal_array.h"

#include <stdio.h>

#include "xvr_literal.h"
#include "xvr_memory.h"

void Xvr_initLiteralArray(Xvr_LiteralArray* array) {
    array->capacity = 0;
    array->count = 0;
    array->literals = NULL;
}

void Xvr_freeLiteralArray(Xvr_LiteralArray* array) {
    // clean up memory
    for (int i = 0; i < array->count; i++) {
        Xvr_freeLiteral(array->literals[i]);
    }

    XVR_FREE_ARRAY(Xvr_Literal, array->literals, array->capacity);
    Xvr_initLiteralArray(array);
}

int Xvr_pushLiteralArray(Xvr_LiteralArray* array, Xvr_Literal literal) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;

        array->capacity = XVR_GROW_CAPACITY(oldCapacity);
        array->literals = XVR_GROW_ARRAY(Xvr_Literal, array->literals,
                                         oldCapacity, array->capacity);
    }

    array->literals[array->count] = Xvr_copyLiteral(literal);
    return array->count++;
}

Xvr_Literal Xvr_popLiteralArray(Xvr_LiteralArray* array) {
    if (array->count <= 0) {
        return XVR_TO_NULL_LITERAL;
    }

    Xvr_Literal ret = array->literals[array->count - 1];

    array->literals[array->count - 1] = XVR_TO_NULL_LITERAL;

    array->count--;
    return ret;
}

int Xvr_findLiteralIndex(Xvr_LiteralArray* array, Xvr_Literal literal) {
    for (int i = 0; i < array->count; i++) {
        if (array->literals[i].type != literal.type) {
            continue;
        }

        if (Xvr_literalsAreEqual(array->literals[i], literal)) {
            return i;
        }
    }

    return -1;
}

bool Xvr_setLiteralArray(Xvr_LiteralArray* array, Xvr_Literal index,
                         Xvr_Literal value) {
    if (!XVR_IS_INTEGER(index)) {
        return false;
    }

    int idx = XVR_AS_INTEGER(index);

    if (idx < 0 || idx >= array->count) {
        return false;
    }

    Xvr_freeLiteral(array->literals[idx]);
    array->literals[idx] = Xvr_copyLiteral(value);

    return true;
}

Xvr_Literal Xvr_getLiteralArray(Xvr_LiteralArray* array, Xvr_Literal index) {
    if (!XVR_IS_INTEGER(index)) {
        return XVR_TO_NULL_LITERAL;
    }

    int idx = XVR_AS_INTEGER(index);

    if (idx < 0 || idx >= array->count) {
        return XVR_TO_NULL_LITERAL;
    }

    return Xvr_copyLiteral(array->literals[idx]);
}
