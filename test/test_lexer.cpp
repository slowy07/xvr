#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include "xvr_lexer.h"

TEST_CASE("Lexer basic semicolon tokenization", "[lexer][unit]") {
    const char* source = "var null;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);

    Xvr_Token print = Xvr_private_scanLexer(&lexer);
    Xvr_Token null = Xvr_private_scanLexer(&lexer);
    Xvr_Token semi = Xvr_private_scanLexer(&lexer);
    Xvr_Token eof = Xvr_private_scanLexer(&lexer);

    REQUIRE(strncmp(print.lexeme, "var", print.length) == 0);
    REQUIRE(strncmp(null.lexeme, "null", null.length) == 0);
    REQUIRE(strncmp(semi.lexeme, ";", semi.length) == 0);
    REQUIRE(eof.type == XVR_TOKEN_EOF);
}

TEST_CASE("Lexer multiple semicolons", "[lexer][unit]") {
    const char* source = ";;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);

    Xvr_Token semi1 = Xvr_private_scanLexer(&lexer);
    Xvr_Token semi2 = Xvr_private_scanLexer(&lexer);
    Xvr_Token eof = Xvr_private_scanLexer(&lexer);

    REQUIRE(semi1.type == XVR_TOKEN_SEMICOLON);
    REQUIRE(semi2.type == XVR_TOKEN_SEMICOLON);
    REQUIRE(eof.type == XVR_TOKEN_EOF);
}

TEST_CASE("Lexer semicolon token type verification", "[lexer][unit]") {
    const char* source = ";";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);

    Xvr_Token semi = Xvr_private_scanLexer(&lexer);

    REQUIRE(semi.type == XVR_TOKEN_SEMICOLON);
    REQUIRE(semi.length == 1);
}

TEST_CASE("Lexer semicolons in for loop context", "[lexer][unit]") {
    const char* source = "for (;;)";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);

    Xvr_Token for_kw = Xvr_private_scanLexer(&lexer);
    Xvr_Token paren_l = Xvr_private_scanLexer(&lexer);
    Xvr_Token semi1 = Xvr_private_scanLexer(&lexer);
    Xvr_Token semi2 = Xvr_private_scanLexer(&lexer);
    Xvr_Token paren_r = Xvr_private_scanLexer(&lexer);
    Xvr_Token eof = Xvr_private_scanLexer(&lexer);

    REQUIRE(for_kw.type == XVR_TOKEN_FOR);
    REQUIRE(paren_l.type == XVR_TOKEN_PAREN_LEFT);
    REQUIRE(semi1.type == XVR_TOKEN_SEMICOLON);
    REQUIRE(semi2.type == XVR_TOKEN_SEMICOLON);
    REQUIRE(paren_r.type == XVR_TOKEN_PAREN_RIGHT);
    REQUIRE(eof.type == XVR_TOKEN_EOF);
}

TEST_CASE("Lexer mixed statements with semicolons", "[lexer][unit]") {
    const char* source = "var x = 1; var y = 2";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);

    int semi_count = 0;
    Xvr_Token tok;
    while ((tok = Xvr_private_scanLexer(&lexer)).type != XVR_TOKEN_EOF) {
        if (tok.type == XVR_TOKEN_SEMICOLON) {
            semi_count++;
        }
    }

    REQUIRE(semi_count == 1);
}
