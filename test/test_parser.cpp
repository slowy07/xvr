#include <catch2/catch_test_macros.hpp>
#include "xvr_lexer.h"
#include "xvr_parser.h"
#include "xvr_ast_node.h"

TEST_CASE("Parser integer literal", "[parser][unit]") {
    const char* source = "42;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_LITERAL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser float literal", "[parser][unit]") {
    const char* source = "3.14;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_LITERAL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser string literal", "[parser][unit]") {
    const char* source = "\"hello\";";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_LITERAL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser boolean true", "[parser][unit]") {
    const char* source = "true;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_LITERAL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser boolean false", "[parser][unit]") {
    const char* source = "false;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_LITERAL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser null literal", "[parser][unit]") {
    const char* source = "null;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_LITERAL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser binary operators", "[parser][unit]") {
    const char* sources[] = {"1 + 2;", "1 - 2;", "1 * 2;", "1 / 2;", "1 % 2;",
                             "1 < 2;", "1 > 2;", "1 == 2;", "1 != 2;",
                             "1 <= 2;", "1 >= 2;"};
    for (const char* src : sources) {
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, src);
        Xvr_Parser parser;
        Xvr_initParser(&parser, &lexer);
        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        REQUIRE(node != nullptr);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
    }
}

TEST_CASE("Parser unary operators", "[parser][unit]") {
    const char* sources[] = {"!true;", "-5;"};
    for (const char* src : sources) {
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, src);
        Xvr_Parser parser;
        Xvr_initParser(&parser, &lexer);
        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        REQUIRE(node != nullptr);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
    }
}

TEST_CASE("Parser grouping", "[parser][unit]") {
    const char* source = "(1 + 2);";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_GROUPING);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser variable declaration", "[parser][unit]") {
    const char* source = "var x = 1;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_VAR_DECL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser variable declaration with type", "[parser][unit]") {
    const char* source = "var x: int = 1;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_VAR_DECL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser variable declaration const", "[parser][unit]") {
    const char* source = "var x: int const = 1;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_VAR_DECL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser if statement", "[parser][unit]") {
    const char* source = "if (true) { 1; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_IF);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser if-else statement", "[parser][unit]") {
    const char* source = "if (true) { 1; } else { 2; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_IF);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser while statement", "[parser][unit]") {
    const char* source = "while (true) { 1; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_WHILE);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser while with complex condition", "[parser][unit]") {
    const char* source = "while (x > 0 and y < 10) { 1; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_WHILE);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser for statement", "[parser][unit]") {
    const char* source = "for (var i = 0; i < 10; i++) { 1; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_FOR);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser for with empty clauses", "[parser][unit]") {
    const char* source = "for (;;) { 1; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_FOR);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser function declaration", "[parser][unit]") {
    const char* source = "proc foo() { 1; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_FN_DECL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser function declaration with params", "[parser][unit]") {
    const char* source = "proc foo(x: int, y: int) { x + y; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_FN_DECL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser function declaration with return type", "[parser][unit]") {
    const char* source = "proc foo(): int { return 1; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_FN_DECL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser function declaration void return", "[parser][unit]") {
    const char* source = "proc foo(): void { return; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_FN_DECL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser function call", "[parser][unit]") {
    const char* source = "foo();";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser function call with args", "[parser][unit]") {
    const char* source = "foo(1, 2);";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser return statement", "[parser][unit]") {
    const char* source = "return 1;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_FN_RETURN);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser return no value", "[parser][unit]") {
    const char* source = "return;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_FN_RETURN);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser break statement", "[parser][unit]") {
    const char* source = "break;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_BREAK);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser continue statement", "[parser][unit]") {
    const char* source = "continue;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_CONTINUE);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser prefix increment/decrement", "[parser][unit]") {
    const char* sources[] = {"++x;", "--x;"};
    for (const char* src : sources) {
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, src);
        Xvr_Parser parser;
        Xvr_initParser(&parser, &lexer);
        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        REQUIRE(node != nullptr);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
    }
}

TEST_CASE("Parser postfix increment/decrement", "[parser][unit]") {
    const char* sources[] = {"x++;", "x--;"};
    for (const char* src : sources) {
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, src);
        Xvr_Parser parser;
        Xvr_initParser(&parser, &lexer);
        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        REQUIRE(node != nullptr);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
    }
}

TEST_CASE("Parser array compound", "[parser][unit]") {
    const char* source = "[1, 2, 3];";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_COMPOUND);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser index access", "[parser][unit]") {
    const char* source = "arr[0];";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_INDEX);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser ternary operator", "[parser][unit]") {
    const char* source = "true ? 1 : 2;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_TERNARY);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser block statement", "[parser][unit]") {
    const char* source = "{ 1; 2; 3; }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_BLOCK);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser import statement", "[parser][unit]") {
    const char* source = "import foo;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_IMPORT);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser import with alias", "[parser][unit]") {
    const char* source = "import foo as bar;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_IMPORT);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser multiple statements", "[parser][unit]") {
    const char* source = "1; 2; 3;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    int count = 0;
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    while (node != nullptr) {
        count++;
        Xvr_freeASTNode(node);
        node = Xvr_scanParser(&parser);
    }
    REQUIRE(count == 3);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser multiple statements with vars", "[parser][unit]") {
    const char* source = "var x = 1; var y = 2; x + y;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    int count = 0;
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    while (node != nullptr) {
        count++;
        Xvr_freeASTNode(node);
        node = Xvr_scanParser(&parser);
    }
    REQUIRE(count == 3);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser compound assignment", "[parser][unit]") {
    const char* sources[] = {"x += 1;", "x -= 1;", "x *= 1;", "x /= 1;", "x %= 1;"};
    for (const char* src : sources) {
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, src);
        Xvr_Parser parser;
        Xvr_initParser(&parser, &lexer);
        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        REQUIRE(node != nullptr);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
    }
}

TEST_CASE("Parser statements without semicolons", "[parser][unit]") {
    const char* sources[] = {"var x = 1", "var x: int = 1", "return 1",
                             "break", "continue", "1 + 2"};
    for (const char* src : sources) {
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, src);
        Xvr_Parser parser;
        Xvr_initParser(&parser, &lexer);
        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        REQUIRE(node != nullptr);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
    }
}

TEST_CASE("Parser cast expressions", "[parser][unit]") {
    const char* sources[] = {"int32(x);", "int64(x);", "float32(x);", "float64(x);",
                             "int16(x);", "int8(x);", "uint8(x);", "uint16(x);",
                             "uint32(x);", "uint64(x);", "bool(x);", "string(x);"};
    for (const char* src : sources) {
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, src);
        Xvr_Parser parser;
        Xvr_initParser(&parser, &lexer);
        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        REQUIRE(node != nullptr);
        REQUIRE(node->type == XVR_AST_NODE_CAST);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
    }
}

TEST_CASE("Parser cast from literals", "[parser][unit]") {
    const char* sources[] = {"int32(42);", "float64(3.14);", "int32(1 + 2);"};
    for (const char* src : sources) {
        Xvr_Lexer lexer;
        Xvr_initLexer(&lexer, src);
        Xvr_Parser parser;
        Xvr_initParser(&parser, &lexer);
        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        REQUIRE(node != nullptr);
        REQUIRE(node->type == XVR_AST_NODE_CAST);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
    }
}

TEST_CASE("Parser nested cast", "[parser][unit]") {
    const char* source = "int32(float32(x));";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_CAST);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser if-else-if statement", "[parser][unit]") {
    const char* source = "if (true) { 1 } else if (false) { 2 }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_IF);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser if-else-if-else statement", "[parser][unit]") {
    const char* source = "if (true) { 1 } else if (false) { 2 } else { 3 }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_IF);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser expression-based if", "[parser][unit]") {
    const char* source = "var x = if (a) { 1 } else { 2 }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_VAR_DECL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser expression-based if with type", "[parser][unit]") {
    const char* source = "var x: int32 = if (a) { 1 } else { 2 }";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    REQUIRE(node->type == XVR_AST_NODE_VAR_DECL);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser chained method call", "[parser][unit]") {
    const char* source = "obj.method();";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

TEST_CASE("Parser property access", "[parser][unit]") {
    const char* source = "obj.prop;";
    Xvr_Lexer lexer;
    Xvr_initLexer(&lexer, source);
    Xvr_Parser parser;
    Xvr_initParser(&parser, &lexer);
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    REQUIRE(node != nullptr);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}
