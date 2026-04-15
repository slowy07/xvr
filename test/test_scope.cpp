#include <catch2/catch_test_macros.hpp>
#include "xvr_scope.h"
#include "xvr_literal.h"
#include "xvr_memory.h"

TEST_CASE("Scope push/pop basic", "[scope][unit]") {
    Xvr_Scope* scope = Xvr_pushScope(nullptr);
    REQUIRE(scope != nullptr);
    
    scope = Xvr_popScope(scope);
    REQUIRE(scope == nullptr);
}

TEST_CASE("Scope variable declaration", "[scope][unit]") {
    const char* idn_raw = "xvr data";
    Xvr_Literal identifier = XVR_TO_IDENTIFIER_LITERAL(Xvr_createRefString(idn_raw));
    Xvr_Literal value = XVR_TO_INTEGER_LITERAL(42);
    Xvr_Literal type = XVR_TO_TYPE_LITERAL(value.type, false);

    Xvr_Scope* scope = Xvr_pushScope(nullptr);

    REQUIRE(Xvr_declareScopeVariable(scope, identifier, type));
    REQUIRE(Xvr_setScopeVariable(scope, identifier, value, true));

    scope = Xvr_popScope(scope);
    Xvr_freeLiteral(identifier);
    Xvr_freeLiteral(value);
    Xvr_freeLiteral(type);
}
