#include "xvr_builtin.h"

#include <stdio.h>

#include "xvr_interpreter.h"
#include "xvr_literal.h"
#include "xvr_literal_array.h"
#include "xvr_literal_dictionary.h"
#include "xvr_memory.h"
#include "xvr_refstring.h"
#include "xvr_scope.h"

static Xvr_Literal addition(Xvr_Interpreter* interpreter, Xvr_Literal lhs,
                            Xvr_Literal rhs) {
    if (XVR_IS_STRING(lhs) && XVR_IS_STRING(rhs)) {
        int totalLength =
            XVR_AS_STRING(lhs)->length + XVR_AS_STRING(rhs)->length;
        if (totalLength > XVR_MAX_STRING_LENGTH) {
            interpreter->errorOutput(
                "Can't concatenate these strings (result was too long)\n");
            return XVR_TO_NULL_LITERAL;
        }

        char buffer[XVR_MAX_STRING_LENGTH];
        snprintf(buffer, XVR_MAX_STRING_LENGTH, "%s%s",
                 Xvr_toCString(XVR_AS_STRING(lhs)),
                 Xvr_toCString(XVR_AS_STRING(rhs)));
        Xvr_Literal literal = XVR_TO_STRING_LITERAL(
            Xvr_createRefStringLength(buffer, totalLength));

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);

        return literal;
    }

    if (XVR_IS_FLOAT(lhs) && XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    if (XVR_IS_INTEGER(lhs) && XVR_IS_FLOAT(rhs)) {
        lhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(lhs));
    }

    Xvr_Literal result = XVR_TO_NULL_LITERAL;

    if (XVR_IS_INTEGER(lhs) && XVR_IS_INTEGER(rhs)) {
        result =
            XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) + XVR_AS_INTEGER(rhs));
        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);
        return result;
    }

    interpreter->errorOutput("Bad arithmetic argument ");
    Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
    interpreter->errorOutput(" and ");
    Xvr_printLiteralCustom(rhs, interpreter->errorOutput);
    interpreter->errorOutput("\n");

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return XVR_TO_NULL_LITERAL;
}

static Xvr_Literal subtraction(Xvr_Interpreter* interpreter, Xvr_Literal lhs,
                               Xvr_Literal rhs) {
    if (XVR_IS_FLOAT(lhs) && XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    if (XVR_IS_INTEGER(lhs) && XVR_IS_FLOAT(rhs)) {
        lhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(lhs));
    }

    Xvr_Literal result = XVR_TO_NULL_LITERAL;

    if (XVR_IS_INTEGER(lhs) && XVR_IS_INTEGER(rhs)) {
        result =
            XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) - XVR_AS_INTEGER(rhs));
        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);

        return result;
    }

    if (XVR_IS_FLOAT(lhs) && XVR_IS_FLOAT(rhs)) {
        result = XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) - XVR_AS_FLOAT(rhs));

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);

        return result;
    }

    interpreter->errorOutput("Bad arithmetic argument ");
    Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
    interpreter->errorOutput(" and ");
    Xvr_printLiteralCustom(rhs, interpreter->errorOutput);
    interpreter->errorOutput("\n");

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return XVR_TO_NULL_LITERAL;
}

static Xvr_Literal multiplication(Xvr_Interpreter* interpreter, Xvr_Literal lhs,
                                  Xvr_Literal rhs) {
    if (XVR_IS_FLOAT(lhs) && XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    if (XVR_IS_INTEGER(lhs) && XVR_IS_FLOAT(rhs)) {
        lhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(lhs));
    }

    Xvr_Literal result = XVR_TO_NULL_LITERAL;

    if (XVR_IS_INTEGER(lhs) && XVR_IS_INTEGER(rhs)) {
        result =
            XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) * XVR_AS_INTEGER(rhs));

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);

        return result;
    }

    if (XVR_IS_FLOAT(lhs) && XVR_IS_FLOAT(rhs)) {
        result = XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) * XVR_AS_FLOAT(rhs));
        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);

        return result;
    }

    interpreter->errorOutput("Bad arithmetic argument ");
    Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
    interpreter->errorOutput(" and ");
    Xvr_printLiteralCustom(rhs, interpreter->errorOutput);
    interpreter->errorOutput("\n");

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return XVR_TO_NULL_LITERAL;
}

static Xvr_Literal division(Xvr_Interpreter* interpreter, Xvr_Literal lhs,
                            Xvr_Literal rhs) {
    if ((XVR_IS_INTEGER(rhs) && XVR_AS_INTEGER(rhs) == 0) ||
        (XVR_IS_FLOAT(rhs) && XVR_AS_FLOAT(rhs) == 0)) {
        interpreter->errorOutput("Can't division by zero");
    }

    if (XVR_IS_FLOAT(lhs) && XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    if (XVR_IS_INTEGER(lhs) && XVR_IS_FLOAT(rhs)) {
        lhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(lhs));
    }

    Xvr_Literal result = XVR_TO_NULL_LITERAL;

    if (XVR_IS_INTEGER(lhs) && XVR_IS_INTEGER(rhs)) {
        result =
            XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) + XVR_AS_INTEGER(rhs));

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);

        return result;
    }

    if (XVR_IS_FLOAT(lhs) && XVR_IS_FLOAT(rhs)) {
        result = XVR_TO_FLOAT_LITERAL(XVR_AS_FLOAT(lhs) + XVR_AS_FLOAT(rhs));
        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);
        return result;
    }

    interpreter->errorOutput("Bad arithmetic argument ");
    Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
    interpreter->errorOutput(" and ");
    Xvr_printLiteralCustom(rhs, interpreter->errorOutput);
    interpreter->errorOutput("\n");

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return XVR_TO_NULL_LITERAL;
}

static Xvr_Literal modulo(Xvr_Interpreter* interpreter, Xvr_Literal lhs,
                          Xvr_Literal rhs) {
    if ((XVR_IS_INTEGER(rhs) && XVR_AS_INTEGER(rhs) == 0) ||
        (XVR_IS_FLOAT(rhs) && XVR_AS_FLOAT(rhs) == 0)) {
        interpreter->errorOutput("Can't divide by zero");
    }

    if (XVR_IS_FLOAT(lhs) && XVR_IS_INTEGER(rhs)) {
        rhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(rhs));
    }

    if (XVR_IS_INTEGER(lhs) && XVR_IS_FLOAT(rhs)) {
        lhs = XVR_TO_FLOAT_LITERAL(XVR_AS_INTEGER(lhs));
    }

    Xvr_Literal result = XVR_TO_NULL_LITERAL;

    if (XVR_IS_INTEGER(lhs) && XVR_IS_INTEGER(rhs)) {
        result =
            XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(lhs) + XVR_AS_INTEGER(rhs));

        Xvr_freeLiteral(lhs);
        Xvr_freeLiteral(rhs);

        return result;
    }

    interpreter->errorOutput("Bad arithmetic argument ");
    Xvr_printLiteralCustom(lhs, interpreter->errorOutput);
    interpreter->errorOutput(" and ");
    Xvr_printLiteralCustom(rhs, interpreter->errorOutput);
    interpreter->errorOutput("\n");

    Xvr_freeLiteral(lhs);
    Xvr_freeLiteral(rhs);

    return XVR_TO_NULL_LITERAL;
}

int Xvr_private_index(Xvr_Interpreter* interpreter,
                      Xvr_LiteralArray* arguments) {
    //_index(compound, first, second, third, assignValue, op)
    Xvr_Literal op = Xvr_popLiteralArray(arguments);
    Xvr_Literal assign = Xvr_popLiteralArray(arguments);
    Xvr_Literal third = Xvr_popLiteralArray(arguments);
    Xvr_Literal second = Xvr_popLiteralArray(arguments);
    Xvr_Literal first = Xvr_popLiteralArray(arguments);
    Xvr_Literal compound = Xvr_popLiteralArray(arguments);

    Xvr_Literal value = XVR_TO_NULL_LITERAL;

    // dictionary - no slicing
    if (XVR_IS_DICTIONARY(compound)) {
        if (XVR_IS_IDENTIFIER(first)) {
            Xvr_Literal idn = first;
            Xvr_parseIdentifierToValue(interpreter, &first);
            Xvr_freeLiteral(idn);
        }

        if (XVR_IS_IDENTIFIER(second)) {
            Xvr_Literal idn = second;
            Xvr_parseIdentifierToValue(interpreter, &second);
            Xvr_freeLiteral(idn);
        }

        if (XVR_IS_IDENTIFIER(third)) {
            Xvr_Literal idn = third;
            Xvr_parseIdentifierToValue(interpreter, &third);
            Xvr_freeLiteral(idn);
        }

        value = Xvr_getLiteralDictionary(XVR_AS_DICTIONARY(compound), first);

        // dictionary
        if (XVR_IS_NULL(op)) {
            Xvr_pushLiteralArray(&interpreter->stack, value);

            Xvr_freeLiteral(op);
            Xvr_freeLiteral(assign);
            Xvr_freeLiteral(third);
            Xvr_freeLiteral(second);
            Xvr_freeLiteral(first);
            Xvr_freeLiteral(compound);
            Xvr_freeLiteral(value);

            return 1;
        }

        else if (Xvr_equalsRefStringCString(XVR_AS_STRING(op), "=")) {
            Xvr_setLiteralDictionary(XVR_AS_DICTIONARY(compound), first,
                                     assign);
        }

        else if (Xvr_equalsRefStringCString(XVR_AS_STRING(op), "+=")) {
            Xvr_Literal lit = addition(interpreter, value, assign);
            Xvr_setLiteralDictionary(XVR_AS_DICTIONARY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }

        else if (Xvr_equalsRefStringCString(XVR_AS_STRING(op), "-=")) {
            Xvr_Literal lit = subtraction(interpreter, value, assign);
            Xvr_setLiteralDictionary(XVR_AS_DICTIONARY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }

        else if (Xvr_equalsRefStringCString(XVR_AS_STRING(op), "*=")) {
            Xvr_Literal lit = multiplication(interpreter, value, assign);
            Xvr_setLiteralDictionary(XVR_AS_DICTIONARY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }

        else if (Xvr_equalsRefStringCString(XVR_AS_STRING(op), "/=")) {
            Xvr_Literal lit = division(interpreter, value, assign);
            Xvr_setLiteralDictionary(XVR_AS_DICTIONARY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }

        else if (Xvr_equalsRefStringCString(XVR_AS_STRING(op), "%=")) {
            Xvr_Literal lit = modulo(interpreter, value, assign);
            Xvr_setLiteralDictionary(XVR_AS_DICTIONARY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }
    }

    // array - slicing
    if (XVR_IS_ARRAY(compound)) {
        // array slice
        if (XVR_IS_NULL(op)) {
            // parse out the blanks & their defaults
            if (!XVR_IS_NULL(first)) {
                if (XVR_IS_INDEX_BLANK(first)) {
                    Xvr_freeLiteral(first);
                    first = XVR_TO_INTEGER_LITERAL(0);
                }

                if (XVR_IS_IDENTIFIER(first)) {
                    Xvr_Literal idn = first;
                    Xvr_parseIdentifierToValue(interpreter, &first);
                    Xvr_freeLiteral(idn);
                }
            }

            if (!XVR_IS_NULL(second)) {
                if (XVR_IS_INDEX_BLANK(second)) {
                    Xvr_freeLiteral(second);
                    second = XVR_TO_INTEGER_LITERAL(
                        XVR_AS_ARRAY(compound)->count - 1);
                }

                if (XVR_IS_IDENTIFIER(second)) {
                    Xvr_Literal idn = second;
                    Xvr_parseIdentifierToValue(interpreter, &second);
                    Xvr_freeLiteral(idn);
                }
            }

            if (XVR_IS_NULL(third) || XVR_IS_INDEX_BLANK(third)) {
                Xvr_freeLiteral(third);
                third = XVR_TO_INTEGER_LITERAL(1);
            }

            if (XVR_IS_IDENTIFIER(third)) {
                Xvr_Literal idn = third;
                Xvr_parseIdentifierToValue(interpreter, &third);
                Xvr_freeLiteral(idn);
            }

            // handle each null case
            if (XVR_IS_NULL(first) || !XVR_IS_INTEGER(first)) {
                // something is weird - skip out
                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return -1;
            }

            if (XVR_IS_NULL(second)) {  // assign only a single character
                // get the "first" within the array, then skip out

                Xvr_freeLiteral(value);
                value = Xvr_getLiteralArray(XVR_AS_ARRAY(compound), first);
                Xvr_pushLiteralArray(&interpreter->stack, value);

                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return 1;
            }

            if (!XVR_IS_INTEGER(second) ||
                (!XVR_IS_NULL(third) && !XVR_IS_INTEGER(third)) ||
                XVR_AS_INTEGER(second) < 0 ||
                XVR_AS_INTEGER(second) > XVR_AS_ARRAY(compound)->count ||
                XVR_AS_INTEGER(third) == 0) {
                // something is weird - skip out
                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return -1;
            }

            // start building a new array from the old one
            Xvr_LiteralArray* result = XVR_ALLOCATE(Xvr_LiteralArray, 1);
            Xvr_initLiteralArray(result);

            int min = XVR_AS_INTEGER(third) > 0 ? XVR_AS_INTEGER(first)
                                                : XVR_AS_INTEGER(second);

            // copy compound into result
            for (int i = min;
                 i >= 0 && i <= XVR_AS_ARRAY(compound)->count &&
                 i >= XVR_AS_INTEGER(first) && i <= XVR_AS_INTEGER(second);
                 i += XVR_AS_INTEGER(third)) {
                Xvr_Literal idx = XVR_TO_INTEGER_LITERAL(i);
                Xvr_Literal tmp =
                    Xvr_getLiteralArray(XVR_AS_ARRAY(compound), idx);
                Xvr_pushLiteralArray(result, tmp);

                Xvr_freeLiteral(idx);
                Xvr_freeLiteral(tmp);
            }

            // finally, swap out the compound for the result
            Xvr_freeLiteral(compound);
            compound = XVR_TO_ARRAY_LITERAL(result);
        }

        // array slice assignment
        if (XVR_IS_STRING(op) &&
            Xvr_equalsRefStringCString(XVR_AS_STRING(op), "=")) {
            // parse out the blanks & their defaults
            if (!XVR_IS_NULL(first)) {
                if (XVR_IS_INDEX_BLANK(first)) {
                    Xvr_freeLiteral(first);
                    first = XVR_TO_INTEGER_LITERAL(0);
                }

                if (XVR_IS_IDENTIFIER(first)) {
                    Xvr_Literal idn = first;
                    Xvr_parseIdentifierToValue(interpreter, &first);
                    Xvr_freeLiteral(idn);
                }
            }

            if (!XVR_IS_NULL(second)) {
                if (XVR_IS_INDEX_BLANK(second)) {
                    Xvr_freeLiteral(second);
                    second = XVR_TO_INTEGER_LITERAL(XVR_AS_INTEGER(first));
                }

                if (XVR_IS_IDENTIFIER(second)) {
                    Xvr_Literal idn = second;
                    Xvr_parseIdentifierToValue(interpreter, &second);
                    Xvr_freeLiteral(idn);
                }
            }

            if (XVR_IS_NULL(third) || XVR_IS_INDEX_BLANK(third)) {
                Xvr_freeLiteral(third);
                third = XVR_TO_INTEGER_LITERAL(1);
            }

            if (XVR_IS_IDENTIFIER(third)) {
                Xvr_Literal idn = third;
                Xvr_parseIdentifierToValue(interpreter, &third);
                Xvr_freeLiteral(idn);
            }

            // handle each null case
            if (XVR_IS_NULL(first) || !XVR_IS_INTEGER(first)) {
                // something is weird - skip out
                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return -1;
            }

            if (XVR_IS_NULL(second)) {
                // set the "first" within the array, then skip out
                if (!Xvr_setLiteralArray(XVR_AS_ARRAY(compound), first,
                                         assign)) {
                    interpreter->errorOutput(
                        "Index assignment out of bounds\n");

                    Xvr_freeLiteral(op);
                    Xvr_freeLiteral(assign);
                    Xvr_freeLiteral(third);
                    Xvr_freeLiteral(second);
                    Xvr_freeLiteral(first);
                    Xvr_freeLiteral(compound);
                    Xvr_freeLiteral(value);

                    return -1;
                }

                Xvr_pushLiteralArray(&interpreter->stack, compound);

                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return 1;
            }

            if (!XVR_IS_INTEGER(second) ||
                (!XVR_IS_NULL(third) && !XVR_IS_INTEGER(third)) ||
                XVR_AS_INTEGER(second) < 0 ||
                XVR_AS_INTEGER(second) > XVR_AS_ARRAY(compound)->count ||
                XVR_AS_INTEGER(third) == 0) {
                // something is weird - skip out
                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return -1;
            }

            // start building a new array from the old one
            Xvr_LiteralArray* result = XVR_ALLOCATE(Xvr_LiteralArray, 1);
            Xvr_initLiteralArray(result);

            // if third is abs(1), simply insert into the correct positions
            if (XVR_AS_INTEGER(third) == 1 || XVR_AS_INTEGER(third) == -1) {
                for (int i = 0; i < XVR_AS_INTEGER(first); i++) {
                    Xvr_Literal idx = XVR_TO_INTEGER_LITERAL(i);
                    Xvr_Literal tmp =
                        Xvr_getLiteralArray(XVR_AS_ARRAY(compound), idx);
                    Xvr_pushLiteralArray(result, tmp);

                    Xvr_freeLiteral(idx);
                    Xvr_freeLiteral(tmp);
                }

                int min = XVR_AS_INTEGER(third) > 0
                              ? 0
                              : XVR_AS_ARRAY(assign)->count - 1;

                if (XVR_IS_ARRAY(
                        assign)) {  // push elements of an assigned array
                    for (int i = min; i >= 0 && i < XVR_AS_ARRAY(assign)->count;
                         i += XVR_AS_INTEGER(third)) {
                        Xvr_Literal idx = XVR_TO_INTEGER_LITERAL(i);
                        Xvr_Literal tmp = Xvr_getLiteralArray(
                            XVR_AS_ARRAY(assign), idx);  // backwards

                        // set result
                        Xvr_pushLiteralArray(result, tmp);

                        Xvr_freeLiteral(idx);
                        Xvr_freeLiteral(tmp);
                    }
                } else {  // push just one element into the array
                    Xvr_pushLiteralArray(result, assign);
                }

                for (int i = XVR_AS_INTEGER(second) + 1;
                     i < XVR_AS_ARRAY(compound)->count; i++) {
                    Xvr_Literal idx = XVR_TO_INTEGER_LITERAL(i);
                    Xvr_Literal tmp =
                        Xvr_getLiteralArray(XVR_AS_ARRAY(compound), idx);
                    Xvr_pushLiteralArray(result, tmp);

                    Xvr_freeLiteral(idx);
                    Xvr_freeLiteral(tmp);
                }
            }

            // else override elements of the array instead
            else {
                // copy compound to result
                for (int i = 0; i < XVR_AS_ARRAY(compound)->count; i++) {
                    Xvr_Literal idx = XVR_TO_INTEGER_LITERAL(i);
                    Xvr_Literal tmp =
                        Xvr_getLiteralArray(XVR_AS_ARRAY(compound), idx);

                    Xvr_pushLiteralArray(result, tmp);

                    Xvr_freeLiteral(idx);
                    Xvr_freeLiteral(tmp);
                }

                int min = XVR_AS_INTEGER(third) > 0
                              ? 0
                              : XVR_AS_ARRAY(compound)->count - 1;

                int assignIndex = 0;
                for (int i = min; i >= 0 && i < XVR_AS_ARRAY(compound)->count &&
                                  assignIndex < XVR_AS_ARRAY(assign)->count;
                     i += XVR_AS_INTEGER(third)) {
                    Xvr_Literal idx = XVR_TO_INTEGER_LITERAL(i);
                    Xvr_Literal ai = XVR_TO_INTEGER_LITERAL(assignIndex++);
                    Xvr_Literal tmp =
                        Xvr_getLiteralArray(XVR_AS_ARRAY(assign), ai);

                    Xvr_setLiteralArray(result, idx, tmp);

                    Xvr_freeLiteral(idx);
                    Xvr_freeLiteral(ai);
                    Xvr_freeLiteral(tmp);
                }
            }

            // finally, swap out the compound for the result
            Xvr_freeLiteral(compound);
            compound = XVR_TO_ARRAY_LITERAL(result);
        }

        if (XVR_IS_IDENTIFIER(first)) {
            Xvr_Literal idn = first;
            Xvr_parseIdentifierToValue(interpreter, &first);
            Xvr_freeLiteral(idn);
        }

        value = Xvr_getLiteralArray(XVR_AS_ARRAY(compound), first);

        if (XVR_IS_STRING(op) &&
            Xvr_equalsRefStringCString(XVR_AS_STRING(op), "+=")) {
            Xvr_Literal lit = addition(interpreter, value, assign);
            Xvr_setLiteralArray(XVR_AS_ARRAY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }

        if (XVR_IS_STRING(op) &&
            Xvr_equalsRefStringCString(XVR_AS_STRING(op), "-=")) {
            Xvr_Literal lit = subtraction(interpreter, value, assign);
            Xvr_setLiteralArray(XVR_AS_ARRAY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }

        if (XVR_IS_STRING(op) &&
            Xvr_equalsRefStringCString(XVR_AS_STRING(op), "*=")) {
            Xvr_Literal lit = multiplication(interpreter, value, assign);
            Xvr_setLiteralArray(XVR_AS_ARRAY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }

        if (XVR_IS_STRING(op) &&
            Xvr_equalsRefStringCString(XVR_AS_STRING(op), "/=")) {
            Xvr_Literal lit = division(interpreter, value, assign);
            Xvr_setLiteralArray(XVR_AS_ARRAY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }

        if (XVR_IS_STRING(op) &&
            Xvr_equalsRefStringCString(XVR_AS_STRING(op), "%=")) {
            Xvr_Literal lit = modulo(interpreter, value, assign);
            Xvr_setLiteralArray(XVR_AS_ARRAY(compound), first, lit);
            Xvr_freeLiteral(lit);
        }
    }

    // string - slicing
    if (XVR_IS_STRING(compound)) {
        // string slice
        if (XVR_IS_NULL(op)) {
            // parse out the blanks & their defaults
            if (!XVR_IS_NULL(first)) {
                if (XVR_IS_INDEX_BLANK(first)) {
                    Xvr_freeLiteral(first);
                    first = XVR_TO_INTEGER_LITERAL(0);
                }

                if (XVR_IS_IDENTIFIER(first)) {
                    Xvr_Literal idn = first;
                    Xvr_parseIdentifierToValue(interpreter, &first);
                    Xvr_freeLiteral(idn);
                }
            }

            int compoundLength = XVR_AS_STRING(compound)->length;
            if (!XVR_IS_NULL(second)) {
                if (XVR_IS_INDEX_BLANK(second)) {
                    Xvr_freeLiteral(second);
                    second = XVR_TO_INTEGER_LITERAL(compoundLength);
                }

                if (XVR_IS_IDENTIFIER(second)) {
                    Xvr_Literal idn = second;
                    Xvr_parseIdentifierToValue(interpreter, &second);
                    Xvr_freeLiteral(idn);
                }
            }

            if (XVR_IS_NULL(third) || XVR_IS_INDEX_BLANK(third)) {
                Xvr_freeLiteral(third);
                third = XVR_TO_INTEGER_LITERAL(1);
            }

            if (XVR_IS_IDENTIFIER(third)) {
                Xvr_Literal idn = third;
                Xvr_parseIdentifierToValue(interpreter, &third);
                Xvr_freeLiteral(idn);
            }

            // handle each null case
            if (XVR_IS_NULL(first) || !XVR_IS_INTEGER(first)) {
                // something is weird - skip out
                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return -1;
            }

            if (XVR_IS_NULL(second)) {  // assign only a single character
                char c = Xvr_toCString(
                    XVR_AS_STRING(compound))[XVR_AS_INTEGER(first)];

                char buffer[16];
                snprintf(buffer, 16, "%c", c);

                Xvr_freeLiteral(value);
                int totalLength = strlen(buffer);
                value = XVR_TO_STRING_LITERAL(
                    Xvr_createRefStringLength(buffer, totalLength));

                Xvr_pushLiteralArray(&interpreter->stack, value);

                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return 1;
            }

            if (!XVR_IS_INTEGER(second) ||
                (!XVR_IS_NULL(third) && !XVR_IS_INTEGER(third)) ||
                XVR_AS_INTEGER(second) < 0 ||
                XVR_AS_INTEGER(second) > compoundLength ||
                XVR_AS_INTEGER(third) == 0) {
                // something is weird - skip out
                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return -1;
            }

            // start building a new string from the old one
            char* result = XVR_ALLOCATE(char, XVR_MAX_STRING_LENGTH);

            int lower = XVR_AS_INTEGER(third) > 0 ? XVR_AS_INTEGER(first)
                                                  : XVR_AS_INTEGER(first) - 1;
            int min = XVR_AS_INTEGER(third) > 0 ? XVR_AS_INTEGER(first)
                                                : XVR_AS_INTEGER(second) - 1;
            int max =
                XVR_AS_INTEGER(third) > 0
                    ? XVR_AS_INTEGER(second) +
                          (XVR_AS_INTEGER(second) == compoundLength ? -1 : 0)
                    : XVR_AS_INTEGER(second);

            // copy compound into result
            int resultIndex = 0;
            for (int i = min; i >= 0 && i >= lower && i <= max;
                 i += XVR_AS_INTEGER(third)) {
                result[resultIndex++] =
                    Xvr_toCString(XVR_AS_STRING(compound))[i];
            }

            result[resultIndex] = '\0';

            // finally, swap out the compound for the result
            Xvr_freeLiteral(compound);
            compound = XVR_TO_STRING_LITERAL(
                Xvr_createRefStringLength(result, resultIndex));

            XVR_FREE_ARRAY(char, result, XVR_MAX_STRING_LENGTH);
        }

        // string slice assignment
        else if (XVR_IS_STRING(op) &&
                 Xvr_equalsRefStringCString(XVR_AS_STRING(op), "=")) {
            // parse out the blanks & their defaults
            if (!XVR_IS_NULL(first)) {
                if (XVR_IS_INDEX_BLANK(first)) {
                    Xvr_freeLiteral(first);
                    first = XVR_TO_INTEGER_LITERAL(0);
                }

                if (XVR_IS_IDENTIFIER(first)) {
                    Xvr_Literal idn = first;
                    Xvr_parseIdentifierToValue(interpreter, &first);
                    Xvr_freeLiteral(idn);
                }
            }

            int compoundLength = XVR_AS_STRING(compound)->length;
            if (!XVR_IS_NULL(second)) {
                if (XVR_IS_INDEX_BLANK(second)) {
                    Xvr_freeLiteral(second);
                    second = XVR_TO_INTEGER_LITERAL(compoundLength);
                }

                if (XVR_IS_IDENTIFIER(second)) {
                    Xvr_Literal idn = second;
                    Xvr_parseIdentifierToValue(interpreter, &second);
                    Xvr_freeLiteral(idn);
                }
            }

            if (XVR_IS_NULL(third) || XVR_IS_INDEX_BLANK(third)) {
                Xvr_freeLiteral(third);
                third = XVR_TO_INTEGER_LITERAL(1);
            }

            if (XVR_IS_IDENTIFIER(first)) {
                Xvr_Literal idn = first;
                Xvr_parseIdentifierToValue(interpreter, &first);
                Xvr_freeLiteral(idn);
            }

            // handle each null case
            if (XVR_IS_NULL(first) || !XVR_IS_INTEGER(first)) {
                // something is weird - skip out
                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return -1;
            }

            if (XVR_IS_NULL(second)) {  // assign only a single character
                // set the "first" within the array, then skip out
                if (XVR_AS_STRING(assign)->length != 1) {
                    // something is weird - skip out
                    Xvr_freeLiteral(op);
                    Xvr_freeLiteral(assign);
                    Xvr_freeLiteral(third);
                    Xvr_freeLiteral(second);
                    Xvr_freeLiteral(first);
                    Xvr_freeLiteral(compound);
                    Xvr_freeLiteral(value);

                    return -1;
                }

                Xvr_Literal copiedCompound = XVR_TO_STRING_LITERAL(
                    Xvr_deepCopyRefString(XVR_AS_STRING(compound)));

                XVR_AS_STRING(copiedCompound)->data[XVR_AS_INTEGER(first)] =
                    Xvr_toCString(XVR_AS_STRING(assign))[0];

                Xvr_pushLiteralArray(&interpreter->stack, copiedCompound);

                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return 1;
            }

            if (!XVR_IS_INTEGER(second) ||
                (!XVR_IS_NULL(third) && !XVR_IS_INTEGER(third)) ||
                XVR_AS_INTEGER(second) < 0 ||
                XVR_AS_INTEGER(second) > compoundLength ||
                XVR_AS_INTEGER(third) == 0) {
                // something is weird - skip out
                Xvr_freeLiteral(op);
                Xvr_freeLiteral(assign);
                Xvr_freeLiteral(third);
                Xvr_freeLiteral(second);
                Xvr_freeLiteral(first);
                Xvr_freeLiteral(compound);
                Xvr_freeLiteral(value);

                return -1;
            }

            // start building a new string from the old one
            char* result = XVR_ALLOCATE(char, XVR_MAX_STRING_LENGTH);

            // if third is abs(1), simply insert into the correct positions
            int resultIndex = 0;
            if (XVR_AS_INTEGER(third) == 1 || XVR_AS_INTEGER(third) == -1) {
                for (int i = 0; i < XVR_AS_INTEGER(first); i++) {
                    result[resultIndex++] =
                        Xvr_toCString(XVR_AS_STRING(compound))[i];
                }

                int assignLength = XVR_AS_STRING(assign)->length;
                int min = XVR_AS_INTEGER(third) > 0 ? 0 : assignLength - 1;

                for (int i = min; i >= 0 && i < assignLength;
                     i += XVR_AS_INTEGER(third)) {
                    result[resultIndex++] =
                        Xvr_toCString(XVR_AS_STRING(assign))[i];
                }

                for (int i = XVR_AS_INTEGER(second) + 1; i < compoundLength;
                     i++) {
                    result[resultIndex++] =
                        Xvr_toCString(XVR_AS_STRING(compound))[i];
                }

                result[resultIndex] = '\0';
            }

            // else override elements of the array instead
            else {
                // copy compound to result
                snprintf(result, XVR_MAX_STRING_LENGTH, "%s",
                         Xvr_toCString(XVR_AS_STRING(compound)));

                int assignLength = XVR_AS_STRING(assign)->length;
                int min = XVR_AS_INTEGER(third) > 0
                              ? XVR_AS_INTEGER(first)
                              : XVR_AS_INTEGER(second) - 1;

                int assignIndex = 0;
                for (int i = min;
                     i >= XVR_AS_INTEGER(first) &&
                     i <= XVR_AS_INTEGER(second) && assignIndex < assignLength;
                     i += XVR_AS_INTEGER(third)) {
                    result[i] =
                        Xvr_toCString(XVR_AS_STRING(assign))[assignIndex++];
                }
                resultIndex = strlen(result);
            }

            // finally, swap out the compound for the result
            Xvr_freeLiteral(compound);
            compound = XVR_TO_STRING_LITERAL(
                Xvr_createRefStringLength(result, resultIndex));

            XVR_FREE_ARRAY(char, result, XVR_MAX_STRING_LENGTH);
        }

        else if (XVR_IS_STRING(op) &&
                 Xvr_equalsRefStringCString(XVR_AS_STRING(op), "+=")) {
            Xvr_Literal tmp = addition(interpreter, compound, assign);
            Xvr_freeLiteral(compound);
            compound = tmp;
        }
    }

    Xvr_pushLiteralArray(&interpreter->stack, compound);

    Xvr_freeLiteral(op);
    Xvr_freeLiteral(assign);
    Xvr_freeLiteral(third);
    Xvr_freeLiteral(second);
    Xvr_freeLiteral(first);
    Xvr_freeLiteral(compound);
    Xvr_freeLiteral(value);

    return 1;
}

int Xvr_private_set(Xvr_Interpreter* interpreter, Xvr_LiteralArray* arguments) {
    if (arguments->count != 3) {
        interpreter->errorOutput("Incorrect number of argument to _set\n");
        return -1;
    }

    Xvr_Literal idn = arguments->literals[0];
    Xvr_Literal obj = arguments->literals[0];
    Xvr_Literal key = arguments->literals[1];
    Xvr_Literal val = arguments->literals[2];

    if (!XVR_IS_IDENTIFIER(idn)) {
        interpreter->errorOutput("Expected identifier in _set\n");
        return -1;
    }

    Xvr_parseIdentifierToValue(interpreter, &obj);
    bool freeKey = false;

    if (XVR_IS_IDENTIFIER(key)) {
        Xvr_parseIdentifierToValue(interpreter, &key);
        freeKey = true;
    }

    bool freeVal = false;
    if (XVR_IS_IDENTIFIER(val)) {
        Xvr_parseIdentifierToValue(interpreter, &val);
        freeVal = true;
    }

    switch (obj.type) {
    case XVR_LITERAL_ARRAY: {
        Xvr_Literal typeLiteral = Xvr_getScopeType(interpreter->scope, key);

        if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_ARRAY) {
            Xvr_Literal subtypeLiteral =
                ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[0];
            if (XVR_AS_TYPE(subtypeLiteral).typeOf != XVR_LITERAL_ANY &&
                XVR_AS_TYPE(subtypeLiteral).typeOf != val.type) {
                interpreter->errorOutput("Bad argument type in _set\n");
                return -1;
            }
        }

        if (!XVR_IS_INTEGER(key)) {
            interpreter->errorOutput("Expected integer index in _set\n");
            return -1;
        }

        if (XVR_AS_ARRAY(obj)->count <= XVR_AS_INTEGER(key) ||
            XVR_AS_INTEGER(key) < 0) {
            interpreter->errorOutput("Index out of bounds in set_\n");
            return -1;
        }

        Xvr_freeLiteral(XVR_AS_ARRAY(obj)->literals[XVR_AS_INTEGER(key)]);
        XVR_AS_ARRAY(obj)->literals[XVR_AS_INTEGER(key)] = Xvr_copyLiteral(val);

        if (!Xvr_setScopeVariable(interpreter->scope, idn, obj, true)) {
            interpreter->errorOutput(
                "Increment type assigned to array in _set\n");
            Xvr_printLiteralCustom(val, interpreter->errorOutput);
            interpreter->errorOutput("\"\n");
            return -1;
        }
        break;
    }

    case XVR_LITERAL_DICTIONARY: {
        Xvr_Literal typeLiteral = Xvr_getScopeType(interpreter->scope, key);

        if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_DICTIONARY) {
            Xvr_Literal keySubtypeLiteral =
                ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[0];
            Xvr_Literal valSubtypeLiteral =
                ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[1];

            if (XVR_AS_TYPE(keySubtypeLiteral).typeOf != XVR_LITERAL_ANY &&
                XVR_AS_TYPE(keySubtypeLiteral).typeOf != key.type) {
                interpreter->printOutput("bad argument type in _set\n");
                return -1;
            }

            if (XVR_AS_TYPE(valSubtypeLiteral).typeOf != XVR_LITERAL_ANY &&
                XVR_AS_TYPE(valSubtypeLiteral).typeOf != val.type) {
                interpreter->printOutput("bad argument type in _set\n");
                return -1;
            }
        }

        Xvr_setLiteralDictionary(XVR_AS_DICTIONARY(obj), key, val);

        if (!Xvr_setScopeVariable(interpreter->scope, idn, obj, true)) {
            interpreter->errorOutput(
                "Incorrect type assigned to dictionary in _set: \"");
            Xvr_printLiteralCustom(val, interpreter->errorOutput);
            interpreter->errorOutput("\"\n");
            return -1;
        }
        break;
    }
    default:
        interpreter->errorOutput("Incorrect compound type in _set: ");
        Xvr_printLiteralCustom(obj, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");
        return -1;
    }

    Xvr_freeLiteral(obj);

    if (freeKey) {
        Xvr_freeLiteral(key);
    }

    if (freeVal) {
        Xvr_freeLiteral(val);
    }

    return 0;
}

int Xvr_private_get(Xvr_Interpreter* interpreter, Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("Incorrect number of arguments to _get");
        return -1;
    }

    Xvr_Literal obj = arguments->literals[0];
    Xvr_Literal key = arguments->literals[1];

    bool freeObj = false;
    if (XVR_IS_IDENTIFIER(obj)) {
        Xvr_parseIdentifierToValue(interpreter, &obj);
        freeObj = true;
    }

    bool freeKey = false;
    if (XVR_IS_IDENTIFIER(key)) {
        Xvr_parseIdentifierToValue(interpreter, &key);
        freeKey = true;
    }

    switch (obj.type) {
    case XVR_LITERAL_ARRAY: {
        if (!XVR_IS_INTEGER(key)) {
            interpreter->errorOutput("Expected integer index in _get\n");
            return -1;
        }

        if (XVR_AS_ARRAY(obj)->count <= XVR_AS_INTEGER(key) ||
            XVR_AS_INTEGER(key) < 0) {
            interpreter->errorOutput("Index out of bounds in _get\n");
            return -1;
        }

        Xvr_pushLiteralArray(&interpreter->stack,
                             XVR_AS_ARRAY(obj)->literals[XVR_AS_INTEGER(key)]);

        if (freeObj) {
            Xvr_freeLiteral(obj);
        }

        if (freeKey) {
            Xvr_freeLiteral(key);
        }

        return 1;
    }

    case XVR_LITERAL_DICTIONARY: {
        Xvr_Literal dict =
            Xvr_getLiteralDictionary(XVR_AS_DICTIONARY(obj), key);
        Xvr_pushLiteralArray(&interpreter->stack, dict);
        Xvr_freeLiteral(dict);

        if (freeObj) {
            Xvr_freeLiteral(obj);
        }

        if (freeKey) {
            Xvr_freeLiteral(key);
        }
        return 1;
    }

    default:
        interpreter->errorOutput("Incorrect compound type in _get \"");
        Xvr_printLiteralCustom(obj, interpreter->errorOutput);
        interpreter->errorOutput("\"\n");
        return -1;
    }
}

int Xvr_private_push(Xvr_Interpreter* interpreter,
                     Xvr_LiteralArray* arguments) {
    if (arguments->count != 2) {
        interpreter->errorOutput("Incorrect number of arguments to _push\n");
        return -1;
    }

    Xvr_Literal idn = arguments->literals[0];
    Xvr_Literal obj = arguments->literals[0];
    Xvr_Literal val = arguments->literals[1];

    if (!XVR_IS_IDENTIFIER(idn)) {
        interpreter->errorOutput("Expected identifier in _push\n");
        return -1;
    }

    Xvr_parseIdentifierToValue(interpreter, &obj);

    bool freeVal = false;
    if (XVR_IS_IDENTIFIER(val)) {
        Xvr_parseIdentifierToValue(interpreter, &val);
        freeVal = true;
    }

    switch (obj.type) {
    case XVR_LITERAL_ARRAY: {
        Xvr_Literal typeLiteral = Xvr_getScopeType(interpreter->scope, val);

        if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_ARRAY) {
            Xvr_Literal subtypeLiteral =
                ((Xvr_Literal*)(XVR_AS_TYPE(typeLiteral).subtypes))[0];

            if (XVR_AS_TYPE(subtypeLiteral).typeOf != XVR_LITERAL_ANY &&
                XVR_AS_TYPE(subtypeLiteral).typeOf != val.type) {
                interpreter->errorOutput("Bad argument type in _push");
                return -1;
            }
        }

        Xvr_pushLiteralArray(XVR_AS_ARRAY(obj), val);

        if (!Xvr_setScopeVariable(interpreter->scope, idn, obj, true)) {
            interpreter->errorOutput(
                "Incorrect type assigned to array in _push: \"");
            Xvr_printLiteralCustom(val, interpreter->errorOutput);
            interpreter->errorOutput("\"\n");
            return -1;
        }

        Xvr_freeLiteral(obj);

        if (freeVal) {
            Xvr_freeLiteral(val);
        }
        return 0;
    }

    default:
        interpreter->errorOutput("Incorrect compound type in _push: ");
        Xvr_printLiteralCustom(obj, interpreter->errorOutput);
        interpreter->errorOutput("\n");
        return -1;
    }
}

int Xvr_private_pop(Xvr_Interpreter* interpreter, Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput("Incorrect number of arguments to _pop\n");
        return -1;
    }

    Xvr_Literal idn = arguments->literals[0];
    Xvr_Literal obj = arguments->literals[0];

    if (!XVR_IS_IDENTIFIER(idn)) {
        interpreter->errorOutput("Expected identifier in _pop\n");
        return -1;
    }

    Xvr_parseIdentifierToValue(interpreter, &obj);

    switch (obj.type) {
    case XVR_LITERAL_ARRAY: {
        Xvr_Literal lit = Xvr_popLiteralArray(XVR_AS_ARRAY(obj));
        Xvr_pushLiteralArray(&interpreter->stack, lit);
        Xvr_freeLiteral(lit);

        if (!Xvr_setScopeVariable(interpreter->scope, idn, obj, true)) {
            interpreter->errorOutput(
                "Incorrect type assigned to array in _pop: ");
            Xvr_printLiteralCustom(obj, interpreter->errorOutput);
            interpreter->errorOutput("\n");
            return -1;
        }
        Xvr_freeLiteral(obj);
        return 1;
    }

    default:
        interpreter->errorOutput("Incorrect compound type in _pop: ");
        Xvr_printLiteralCustom(obj, interpreter->errorOutput);
        interpreter->errorOutput("\n");
        return -1;
    }
}

int Xvr_private_length(Xvr_Interpreter* interpreter,
                       Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput("Incorrect number of arguments to _length\n");
        return 1;
    }

    Xvr_Literal obj = arguments->literals[0];
    bool freeObj = false;
    if (XVR_IS_IDENTIFIER(obj)) {
        Xvr_parseIdentifierToValue(interpreter, &obj);
        freeObj = true;
    }

    switch (obj.type) {
    case XVR_LITERAL_ARRAY: {
        Xvr_Literal lit = XVR_TO_INTEGER_LITERAL(XVR_AS_ARRAY(obj)->count);
        Xvr_pushLiteralArray(&interpreter->stack, lit);
        Xvr_freeLiteral(lit);
        break;
    }
    case XVR_LITERAL_DICTIONARY: {
        Xvr_Literal lit = XVR_TO_INTEGER_LITERAL(XVR_AS_DICTIONARY(obj)->count);
        Xvr_pushLiteralArray(&interpreter->stack, lit);
        Xvr_freeLiteral(lit);
        break;
    }

    case XVR_LITERAL_STRING: {
        Xvr_Literal lit = XVR_TO_INTEGER_LITERAL(XVR_AS_STRING(obj)->length);
        Xvr_pushLiteralArray(&interpreter->stack, lit);
        Xvr_freeLiteral(lit);
        break;
    }

    default:
        interpreter->errorOutput("Incorrect compound type in _length: ");
        Xvr_printLiteralCustom(obj, interpreter->errorOutput);
        interpreter->errorOutput("\n");
        return -1;
    }

    if (freeObj) {
        Xvr_freeLiteral(obj);
    }

    return 1;

    return 1;
}

int Xvr_private_clear(Xvr_Interpreter* interpreter,
                      Xvr_LiteralArray* arguments) {
    if (arguments->count != 1) {
        interpreter->errorOutput("Incorrect number of arguments to _clear\n");
        return -1;
    }

    Xvr_Literal idn = arguments->literals[0];
    Xvr_Literal obj = arguments->literals[0];

    if (!XVR_IS_IDENTIFIER(idn)) {
        interpreter->errorOutput("expected identifier in _clear\n");
        return -1;
    }

    Xvr_parseIdentifierToValue(interpreter, &obj);

    switch (obj.type) {
    case XVR_LITERAL_ARRAY: {
        Xvr_LiteralArray* array = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(array);

        Xvr_Literal obj = XVR_TO_ARRAY_LITERAL(array);

        if (!Xvr_setScopeVariable(interpreter->scope, idn, obj, true)) {
            interpreter->errorOutput(
                "Incorrect type assigned to array in _clear: ");
            Xvr_printLiteralCustom(obj, interpreter->errorOutput);
            interpreter->errorOutput("\n");
            return -1;
        }
        Xvr_freeLiteral(obj);
        break;
    }

    case XVR_LITERAL_DICTIONARY: {
        Xvr_LiteralDictionary* dictionary =
            XVR_ALLOCATE(Xvr_LiteralDictionary, 1);
        Xvr_initLiteralDictionary(dictionary);

        Xvr_Literal obj = XVR_TO_DICTIONARY_LITERAL(dictionary);

        if (!Xvr_setScopeVariable(interpreter->scope, idn, obj, true)) {
            interpreter->errorOutput(
                "Incorrect type assigned to dictionary in _clear: ");
            Xvr_printLiteralCustom(obj, interpreter->errorOutput);
            interpreter->errorOutput("\n");
            return -1;
        }

        Xvr_freeLiteral(obj);
        break;
    }

    default:
        interpreter->errorOutput("Incorrect compound type in _clear: ");
        Xvr_printLiteralCustom(obj, interpreter->errorOutput);
        interpreter->errorOutput("\n");
        return -1;
    }

    Xvr_freeLiteral(obj);
    return 1;
}
