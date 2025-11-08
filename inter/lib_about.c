#include "lib_about.h"

#include "xvr_memory.h"

int Xvr_hookAbout(Xvr_Interpreter* interpreter, Xvr_Literal identifier,
                  Xvr_Literal alias) {
    Xvr_Literal majorKeyLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefString("major"));
    Xvr_Literal minorKeyLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefString("minor"));
    Xvr_Literal patchKeyLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefString("patch"));
    Xvr_Literal buildKeyLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefString("build"));
    Xvr_Literal authorKeyLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefString("author"));

    Xvr_Literal majorIdentifierLiteral =
        XVR_TO_IDENTIFIER_LITERAL(Xvr_createRefString("major"));
    Xvr_Literal minorIdentifierLiteral =
        XVR_TO_IDENTIFIER_LITERAL(Xvr_createRefString("minor"));
    Xvr_Literal patchIdentifierLiteral =
        XVR_TO_IDENTIFIER_LITERAL(Xvr_createRefString("patch"));
    Xvr_Literal buildIdentifierLiteral =
        XVR_TO_IDENTIFIER_LITERAL(Xvr_createRefString("build"));
    Xvr_Literal authorIdentifierLiteral =
        XVR_TO_IDENTIFIER_LITERAL(Xvr_createRefString("author"));

    Xvr_Literal majorLiteral = XVR_TO_INTEGER_LITERAL(XVR_VERSION_MAJOR);
    Xvr_Literal minorLiteral = XVR_TO_INTEGER_LITERAL(XVR_VERSION_MINOR);
    Xvr_Literal patchLiteral = XVR_TO_INTEGER_LITERAL(XVR_VERSION_PATCH);
    Xvr_Literal buildLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefString(XVR_VERSION_BUILD));
    Xvr_Literal authorLiteral =
        XVR_TO_STRING_LITERAL(Xvr_createRefString("Arfy Slowy"));

    if (!XVR_IS_NULL(alias)) {
        if (Xvr_isDeclaredScopeVariable(interpreter->scope, alias)) {
            interpreter->errorOutput("Can't override an existing variable\n");
            Xvr_freeLiteral(alias);

            Xvr_freeLiteral(majorKeyLiteral);
            Xvr_freeLiteral(minorKeyLiteral);
            Xvr_freeLiteral(patchKeyLiteral);
            Xvr_freeLiteral(buildKeyLiteral);
            Xvr_freeLiteral(authorKeyLiteral);

            Xvr_freeLiteral(majorIdentifierLiteral);
            Xvr_freeLiteral(minorIdentifierLiteral);
            Xvr_freeLiteral(patchIdentifierLiteral);
            Xvr_freeLiteral(buildIdentifierLiteral);
            Xvr_freeLiteral(authorIdentifierLiteral);

            Xvr_freeLiteral(majorLiteral);
            Xvr_freeLiteral(minorLiteral);
            Xvr_freeLiteral(patchLiteral);
            Xvr_freeLiteral(buildLiteral);
            Xvr_freeLiteral(authorLiteral);

            return -1;
        }

        Xvr_LiteralDictionary* dictionary =
            XVR_ALLOCATE(Xvr_LiteralDictionary, 1);
        Xvr_initLiteralDictionary(dictionary);

        Xvr_setLiteralDictionary(dictionary, majorKeyLiteral, majorLiteral);
        Xvr_setLiteralDictionary(dictionary, minorKeyLiteral, minorLiteral);
        Xvr_setLiteralDictionary(dictionary, patchKeyLiteral, patchLiteral);
        Xvr_setLiteralDictionary(dictionary, buildKeyLiteral, buildLiteral);
        Xvr_setLiteralDictionary(dictionary, authorKeyLiteral, authorLiteral);

        Xvr_Literal type = XVR_TO_TYPE_LITERAL(XVR_LITERAL_DICTIONARY, true);
        Xvr_Literal strType = XVR_TO_TYPE_LITERAL(XVR_LITERAL_STRING, true);
        Xvr_Literal anyType = XVR_TO_TYPE_LITERAL(XVR_LITERAL_ANY, true);
        XVR_TYPE_PUSH_SUBTYPE(&type, strType);
        XVR_TYPE_PUSH_SUBTYPE(&type, anyType);

        Xvr_Literal dict = XVR_TO_DICTIONARY_LITERAL(dictionary);
        Xvr_declareScopeVariable(interpreter->scope, alias, type);
        Xvr_setScopeVariable(interpreter->scope, alias, dict, false);

        Xvr_freeLiteral(dict);
        Xvr_freeLiteral(type);
    } else {
        if (Xvr_isDeclaredScopeVariable(interpreter->scope, majorKeyLiteral) ||
            Xvr_isDeclaredScopeVariable(interpreter->scope, minorKeyLiteral) ||
            Xvr_isDeclaredScopeVariable(interpreter->scope, patchKeyLiteral) ||
            Xvr_isDeclaredScopeVariable(interpreter->scope, buildKeyLiteral) ||
            Xvr_isDeclaredScopeVariable(interpreter->scope, authorKeyLiteral)) {
            interpreter->errorOutput("Can't override an existing variable\n");
            Xvr_freeLiteral(alias);

            Xvr_freeLiteral(majorKeyLiteral);
            Xvr_freeLiteral(minorKeyLiteral);
            Xvr_freeLiteral(patchKeyLiteral);
            Xvr_freeLiteral(buildKeyLiteral);
            Xvr_freeLiteral(authorKeyLiteral);

            Xvr_freeLiteral(majorIdentifierLiteral);
            Xvr_freeLiteral(minorIdentifierLiteral);
            Xvr_freeLiteral(patchIdentifierLiteral);
            Xvr_freeLiteral(buildIdentifierLiteral);
            Xvr_freeLiteral(authorIdentifierLiteral);

            Xvr_freeLiteral(majorLiteral);
            Xvr_freeLiteral(minorLiteral);
            Xvr_freeLiteral(patchLiteral);
            Xvr_freeLiteral(buildLiteral);
            Xvr_freeLiteral(authorLiteral);

            return -1;
        }

        Xvr_Literal intType = XVR_TO_TYPE_LITERAL(XVR_LITERAL_INTEGER, true);
        Xvr_Literal strType = XVR_TO_TYPE_LITERAL(XVR_LITERAL_STRING, true);

        // major
        Xvr_declareScopeVariable(interpreter->scope, majorIdentifierLiteral,
                                 intType);
        Xvr_setScopeVariable(interpreter->scope, majorIdentifierLiteral,
                             majorLiteral, false);

        // minor
        Xvr_declareScopeVariable(interpreter->scope, minorIdentifierLiteral,
                                 intType);
        Xvr_setScopeVariable(interpreter->scope, minorIdentifierLiteral,
                             minorLiteral, false);

        // patch
        Xvr_declareScopeVariable(interpreter->scope, patchIdentifierLiteral,
                                 intType);
        Xvr_setScopeVariable(interpreter->scope, patchIdentifierLiteral,
                             patchLiteral, false);

        // build
        Xvr_declareScopeVariable(interpreter->scope, buildIdentifierLiteral,
                                 strType);
        Xvr_setScopeVariable(interpreter->scope, buildIdentifierLiteral,
                             buildLiteral, false);

        // author
        Xvr_declareScopeVariable(interpreter->scope, authorIdentifierLiteral,
                                 strType);
        Xvr_setScopeVariable(interpreter->scope, authorIdentifierLiteral,
                             authorLiteral, false);

        Xvr_freeLiteral(intType);
        Xvr_freeLiteral(strType);
    }

    Xvr_freeLiteral(majorKeyLiteral);
    Xvr_freeLiteral(minorKeyLiteral);
    Xvr_freeLiteral(patchKeyLiteral);
    Xvr_freeLiteral(buildKeyLiteral);
    Xvr_freeLiteral(authorKeyLiteral);

    Xvr_freeLiteral(majorIdentifierLiteral);
    Xvr_freeLiteral(minorIdentifierLiteral);
    Xvr_freeLiteral(patchIdentifierLiteral);
    Xvr_freeLiteral(buildIdentifierLiteral);
    Xvr_freeLiteral(authorIdentifierLiteral);

    Xvr_freeLiteral(majorLiteral);
    Xvr_freeLiteral(minorLiteral);
    Xvr_freeLiteral(patchLiteral);
    Xvr_freeLiteral(buildLiteral);
    Xvr_freeLiteral(authorLiteral);

    return 0;
}
