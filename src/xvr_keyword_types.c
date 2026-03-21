#include "xvr_keyword_types.h"

#include <string.h>

#include "xvr_common.h"
#include "xvr_token_types.h"

Xvr_KeywordType Xvr_keywordTypes[] = {
    // type keywords
    {XVR_TOKEN_NULL, "null"},
    {XVR_TOKEN_VOID, "void"},
    {XVR_TOKEN_BOOLEAN, "bool"},
    {XVR_TOKEN_INTEGER, "int"},
    {XVR_TOKEN_FLOAT, "float"},
    {XVR_TOKEN_STRING, "string"},
    {XVR_TOKEN_FUNCTION, "proc"},
    {XVR_TOKEN_OPAQUE, "opaque"},
    {XVR_TOKEN_ANY, "any"},

    // fixed-size integer types
    {XVR_TOKEN_INT8, "int8"},
    {XVR_TOKEN_INT16, "int16"},
    {XVR_TOKEN_INT32, "int32"},
    {XVR_TOKEN_INT64, "int64"},
    {XVR_TOKEN_UINT8, "uint8"},
    {XVR_TOKEN_UINT16, "uint16"},
    {XVR_TOKEN_UINT32, "uint32"},
    {XVR_TOKEN_UINT64, "uint64"},
    {XVR_TOKEN_FLOAT16, "float16"},
    {XVR_TOKEN_FLOAT32, "float32"},
    {XVR_TOKEN_FLOAT64, "float64"},

    // other keywords
    {XVR_TOKEN_AS, "as"},
    {XVR_TOKEN_ASSERT, "assert"},
    {XVR_TOKEN_BREAK, "break"},
    {XVR_TOKEN_CLASS, "class"},
    {XVR_TOKEN_CONST, "const"},
    {XVR_TOKEN_CONTINUE, "continue"},
    {XVR_TOKEN_DO, "do"},
    {XVR_TOKEN_ELSE, "else"},
    {XVR_TOKEN_EXPORT, "export"},
    {XVR_TOKEN_FOR, "for"},
    {XVR_TOKEN_FOREACH, "foreach"},
    {XVR_TOKEN_IF, "if"},
    {XVR_TOKEN_IMPORT, "import"},
    {XVR_TOKEN_IN, "in"},
    {XVR_TOKEN_OF, "of"},
    {XVR_TOKEN_PRINT, "print"},
    {XVR_TOKEN_RETURN, "return"},
    {XVR_TOKEN_TYPE, "type"},
    {XVR_TOKEN_ASTYPE, "astype"},
    {XVR_TOKEN_TYPEOF, "typeof"},
    {XVR_TOKEN_VAR, "var"},
    {XVR_TOKEN_WHILE, "while"},

    // literal values
    {XVR_TOKEN_LITERAL_TRUE, "true"},
    {XVR_TOKEN_LITERAL_FALSE, "false"},

    // meta tokens
    {XVR_TOKEN_PASS, NULL},
    {XVR_TOKEN_ERROR, NULL},

    {XVR_TOKEN_EOF, NULL},
};

char* Xvr_findKeywordByType(Xvr_TokenType type) {
    if (type == XVR_TOKEN_EOF) {
        return "EOF";
    }

    for (int i = 0; Xvr_keywordTypes[i].keyword; i++) {
        if (Xvr_keywordTypes[i].type == type) {
            return Xvr_keywordTypes[i].keyword;
        }
    }

    return NULL;
}

Xvr_TokenType Xvr_findTypeByKeyword(const char* keyword) {
    const int length = strlen(keyword);

    for (int i = 0; Xvr_keywordTypes[i].keyword; i++) {
        if (!strncmp(keyword, Xvr_keywordTypes[i].keyword, length)) {
            return Xvr_keywordTypes[i].type;
        }
    }

    return XVR_TOKEN_EOF;
}
