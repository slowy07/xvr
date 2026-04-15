#include <catch2/catch_test_macros.hpp>
#include "xvr_literal.h"
#include "xvr_refstring.h"

TEST_CASE("Literal null handling", "[literal][unit]") {
    Xvr_Literal literal = XVR_TO_NULL_LITERAL;
    REQUIRE(XVR_IS_NULL(literal));
}

TEST_CASE("Literal boolean handling", "[literal][unit]") {
    Xvr_Literal t = XVR_TO_BOOLEAN_LITERAL(true);
    Xvr_Literal f = XVR_TO_BOOLEAN_LITERAL(false);
    
    REQUIRE(XVR_IS_TRUTHY(t));
    REQUIRE_FALSE(XVR_IS_TRUTHY(f));
}

TEST_CASE("Literal string handling", "[literal][unit]") {
    const char* buffer = "wello cik";
    Xvr_Literal literal = XVR_TO_STRING_LITERAL(Xvr_createRefString(buffer));
    
    REQUIRE_NOTHROW(Xvr_freeLiteral(literal));
}

TEST_CASE("Literal integer equality", "[literal][unit]") {
    Xvr_Literal a = XVR_TO_INTEGER_LITERAL(42);
    Xvr_Literal b = XVR_TO_INTEGER_LITERAL(42);
    
    REQUIRE(Xvr_literalsAreEqual(a, b));
    
    Xvr_freeLiteral(a);
    Xvr_freeLiteral(b);
}

TEST_CASE("Literal copy operations", "[literal][unit]") {
    Xvr_Literal original = XVR_TO_INTEGER_LITERAL(100);
    Xvr_Literal copy = Xvr_copyLiteral(original);
    
    REQUIRE(Xvr_literalsAreEqual(original, copy));
    
    Xvr_freeLiteral(original);
    Xvr_freeLiteral(copy);
}
