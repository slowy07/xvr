#include "lib_standard.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "xvr_memory.h"

static int nativeClock(Xvr_Interpreter* interpreter,
                       Xvr_LiteralArray* arguments) {
    // no arguments
    if (arguments->count != 0) {
        interpreter->errorOutput("Incorrect number of arguments to clock\n");
        return -1;
    }

    // get the time from C (what a pain)
    time_t rawtime = time(NULL);
    struct tm* timeinfo = localtime(&rawtime);
    char* timestr = asctime(timeinfo);

    // push to the stack
    int len = strlen(timestr) - 1;
    Xvr_Literal timeLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefStringLength(timestr, len));

    Xvr_pushLiteralArray(&interpreter->stack, timeLiteral);

    // cleanup
    Xvr_freeLiteral(timeLiteral);

    return 1;
}

typedef struct Natives {
    char* name;
    Xvr_NativeFn fn;
} Natives;

int Xvr_hookStandard(Xvr_Interpreter* interpreter, Xvr_Literal identifier,
                     Xvr_Literal alias) {
    Natives natives[] = {{"clock", nativeClock}, {NULL, NULL}};

    if (!XVR_IS_NULL(alias)) {
        if (Xvr_isDeclaredScopeVariable(interpreter->scope, alias)) {
            interpreter->errorOutput("Can't override an existing variable\n");
            Xvr_freeLiteral(alias);
            return -1;
        }

        // create the dictionary to load up with functions
        Xvr_LiteralDictionary* dictionary =
            XVR_ALLOCATE(Xvr_LiteralDictionary, 1);
        Xvr_initLiteralDictionary(dictionary);

        // load the dict with functions
        for (int i = 0; natives[i].name; i++) {
            Xvr_Literal name =
                XVR_TO_STRING_LITERAL(Xvr_createRefString(natives[i].name));
            Xvr_Literal func = XVR_TO_FUNCTION_NATIVE_LITERAL(natives[i].fn);

            Xvr_setLiteralDictionary(dictionary, name, func);

            Xvr_freeLiteral(name);
            Xvr_freeLiteral(func);
        }

        // build the type
        Xvr_Literal type = XVR_TO_TYPE_LITERAL(XVR_LITERAL_DICTIONARY, true);
        Xvr_Literal strType = XVR_TO_TYPE_LITERAL(XVR_LITERAL_STRING, true);
        Xvr_Literal fnType =
            XVR_TO_TYPE_LITERAL(XVR_LITERAL_FUNCTION_NATIVE, true);
        XVR_TYPE_PUSH_SUBTYPE(&type, strType);
        XVR_TYPE_PUSH_SUBTYPE(&type, fnType);

        Xvr_Literal dict = XVR_TO_DICTIONARY_LITERAL(dictionary);
        Xvr_declareScopeVariable(interpreter->scope, alias, type);
        Xvr_setScopeVariable(interpreter->scope, alias, dict, false);

        Xvr_freeLiteral(dict);
        Xvr_freeLiteral(type);
        return 0;
    }

    for (int i = 0; natives[i].name; i++) {
        Xvr_injectNativeFn(interpreter, natives[i].name, natives[i].fn);
    }

    return 0;
}
