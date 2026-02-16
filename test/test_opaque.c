#include <stdio.h>
#include <stdlib.h>

#include "inter_tools.h"
#include "xvr_console_colors.h"
#include "xvr_interpreter.h"
#include "xvr_literal.h"
#include "xvr_literal_array.h"
#include "xvr_memory.h"

void error(char* message) {
    printf("%s", message);
    exit(-1);
}

typedef struct ArbitraryData {
    int value;
} ArbitraryData;

static int produce(Xvr_Interpreter* interpreter, Xvr_LiteralArray* arguments) {
    ArbitraryData* data = XVR_ALLOCATE(ArbitraryData, 1);
    data->value = 30;

    Xvr_Literal testData = XVR_TO_OPAQUE_LITERAL(data, 0);
    Xvr_pushLiteralArray(&interpreter->stack, testData);
    Xvr_freeLiteral(testData);
    return 1;
}

static int consume(Xvr_Interpreter* interpreter, Xvr_LiteralArray* arguments) {
    Xvr_Literal testData = Xvr_popLiteralArray(arguments);
    Xvr_Literal idn = testData;

    if (Xvr_parseIdentifierToValue(interpreter, &testData)) {
        Xvr_freeLiteral(idn);
    }

    if (XVR_IS_OPAQUE(testData) &&
        ((ArbitraryData*)(XVR_AS_OPAQUE(testData)))->value == 30) {
        ArbitraryData* data = (ArbitraryData*)XVR_AS_OPAQUE(testData);
        XVR_FREE(ArbitraryData, data);

        Xvr_freeLiteral(testData);

        return 0;
    }

    printf(XVR_CC_ERROR
           "TEST OPAQUE: test opaque failed cik: %d\n" XVR_CC_RESET,
           XVR_IS_OPAQUE(testData));
    exit(-1);
    return -1;
}

int main(void) {
    {
        size_t size = 0;
        const char* source =
            (const char*)Xvr_readFile("test/xvr_file/opaque_data.xvr", &size);
        const unsigned char* xb = Xvr_compileString(source, &size);
        free((void*)source);

        if (!xb) {
            return -1;
        }

        Xvr_Interpreter interpreter;
        Xvr_initInterpreter(&interpreter);

        Xvr_injectNativeFn(&interpreter, "produce", produce);
        Xvr_injectNativeFn(&interpreter, "consume", consume);

        Xvr_runInterpreter(&interpreter, xb, size);

        Xvr_freeInterpreter(&interpreter);
    }

    printf(XVR_CC_NOTICE "TEST OPAQUE: aman cik gass\n" XVR_CC_RESET);
    return 0;
}
