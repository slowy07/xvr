#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter_tools.h"
#include "xvr_compiler.h"
#include "xvr_console_colors.h"
#include "xvr_interpreter.h"
#include "xvr_lexer.h"
#include "xvr_literal_array.h"
#include "xvr_memory.h"
#include "xvr_parser.h"

static void noPrintFn(const char* output) {}

void error(char* msg) {
    printf("%s", msg);
    exit(-1);
}

int main(void) {
    {
        size_t size = 0;
        char* source = Xvr_readFile("scripts/call_host.xvr", &size);
        unsigned char* tb = Xvr_compileString(source, &size);
        free((void*)source);

        if (!tb) {
            return -1;
        }

        Xvr_Interpreter interpreter;
        Xvr_initInterpreter(&interpreter);
        Xvr_runInterpreter(&interpreter, tb, size);

        {
            interpreter.printOutput("Testing asw");
            Xvr_LiteralArray arguments;
            Xvr_initLiteralArray(&arguments);
            Xvr_LiteralArray returns;
            Xvr_initLiteralArray(&returns);

            Xvr_callFn(&interpreter, "asw", &arguments, &returns);

            if (arguments.count != 0) {
                error("Woilah cik arguments has the wrong number of members\n");
            }

            if (returns.count != 1) {
                error("Woilah cik Returns has the wrong number of members\n");
            }

            if (!XVR_IS_INTEGER(returns.literals[0]) ||
                XVR_AS_INTEGER(returns.literals[0]) != 42) {
                error("Returned value is incorrect\n");
            }

            Xvr_freeLiteralArray(&arguments);
            Xvr_freeLiteralArray(&returns);
        }

        Xvr_freeInterpreter(&interpreter);
    }

    printf(XVR_CC_NOTICE "FROM HOST: jalan cik aman loh ya\n" XVR_CC_RESET);
    return 0;
}
