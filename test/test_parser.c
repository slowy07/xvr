#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter_tools.h"
#include "xvr_ast_node.h"
#include "xvr_console_colors.h"
#include "xvr_parser.h"

static int testCount = 0;
static int passCount = 0;

static void testParse(const char* name, const char* source,
                      Xvr_ASTNodeType expectedType) {
    testCount++;
    Xvr_Lexer lexer;
    Xvr_Parser parser;
    Xvr_initLexer(&lexer, source);
    Xvr_initParser(&parser, &lexer);

    Xvr_ASTNode* node = Xvr_scanParser(&parser);

    if (node == NULL) {
        fprintf(stderr, XVR_CC_ERROR "FAIL: %s - got NULL node\n" XVR_CC_RESET,
                name);
        return;
    }

    if (node->type != expectedType) {
        fprintf(stderr,
                XVR_CC_ERROR "FAIL: %s - expected %d, got %d\n" XVR_CC_RESET,
                name, expectedType, node->type);
        Xvr_freeASTNode(node);
        Xvr_freeParser(&parser);
        return;
    }

    passCount++;
    printf(XVR_CC_NOTICE "PASS CIK: %s\n" XVR_CC_RESET, name);
    Xvr_freeASTNode(node);
    Xvr_freeParser(&parser);
}

static void testParseMultiple(const char* name, const char* source,
                              int expectedCount) {
    testCount++;
    Xvr_Lexer lexer;
    Xvr_Parser parser;
    Xvr_initLexer(&lexer, source);
    Xvr_initParser(&parser, &lexer);

    int count = 0;
    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    while (node != NULL) {
        count++;
        Xvr_freeASTNode(node);
        node = Xvr_scanParser(&parser);
    }

    if (count != expectedCount) {
        fprintf(stderr,
                XVR_CC_ERROR
                "FAIL: %s - expected %d nodes, got %d\n" XVR_CC_RESET,
                name, expectedCount, count);
        Xvr_freeParser(&parser);
        return;
    }

    passCount++;
    printf(XVR_CC_NOTICE "PASS CIK: %s (%d statements)\n" XVR_CC_RESET, name,
           count);
    Xvr_freeParser(&parser);
}

int main(void) {
    printf("INFO: TEST PARSERNY CIK\n");
    testParse("integer literal", "42;", XVR_AST_NODE_LITERAL);
    testParse("float literal", "3.14;", XVR_AST_NODE_LITERAL);
    testParse("string literal", "\"hello\";", XVR_AST_NODE_LITERAL);
    testParse("boolean true", "true;", XVR_AST_NODE_LITERAL);
    testParse("boolean false", "false;", XVR_AST_NODE_LITERAL);
    testParse("null literal", "null;", XVR_AST_NODE_LITERAL);

    testParse("binary add", "1 + 2;", XVR_AST_NODE_LITERAL);
    testParse("binary subtract", "1 - 2;", XVR_AST_NODE_LITERAL);
    testParse("binary multiply", "1 * 2;", XVR_AST_NODE_LITERAL);
    testParse("binary divide", "1 / 2;", XVR_AST_NODE_LITERAL);
    testParse("binary modulo", "1 % 2;", XVR_AST_NODE_LITERAL);
    testParse("binary less than", "1 < 2;", XVR_AST_NODE_LITERAL);
    testParse("binary greater than", "1 > 2;", XVR_AST_NODE_LITERAL);
    testParse("binary equal", "1 == 2;", XVR_AST_NODE_LITERAL);
    testParse("binary not equal", "1 != 2;", XVR_AST_NODE_LITERAL);
    testParse("binary less equal", "1 <= 2;", XVR_AST_NODE_LITERAL);
    testParse("binary greater equal", "1 >= 2;", XVR_AST_NODE_LITERAL);
    testParse("binary and", "true and false;", XVR_AST_NODE_BINARY);
    testParse("binary or", "true or false;", XVR_AST_NODE_BINARY);

    testParse("unary not", "!true;", XVR_AST_NODE_LITERAL);
    testParse("unary negative", "-5;", XVR_AST_NODE_LITERAL);

    testParse("grouping", "(1 + 2);", XVR_AST_NODE_GROUPING);

    testParse("variable declaration", "var x = 1;", XVR_AST_NODE_VAR_DECL);
    testParse("variable declaration with type", "var x: int = 1;",
              XVR_AST_NODE_VAR_DECL);
    testParse("variable declaration const", "var x: int const = 1;",
              XVR_AST_NODE_VAR_DECL);

    testParse("if statement", "if (true) { 1; }", XVR_AST_NODE_IF);
    testParse("if-else statement", "if (true) { 1; } else { 2; }",
              XVR_AST_NODE_IF);

    testParse("while statement", "while (true) { 1; }", XVR_AST_NODE_WHILE);

    testParse("for statement", "for (var i = 0; i < 10; i++) { 1; }",
              XVR_AST_NODE_FOR);

    testParse("function declaration", "proc foo() { 1; }",
              XVR_AST_NODE_FN_DECL);
    testParse("function declaration with params",
              "proc foo(x: int, y: int) { x + y; }", XVR_AST_NODE_FN_DECL);
    testParse("function declaration with return type",
              "proc foo(): int { return 1; }", XVR_AST_NODE_FN_DECL);

    testParse("function call", "foo();", XVR_AST_NODE_BINARY);
    testParse("function call with args", "foo(1, 2);", XVR_AST_NODE_BINARY);

    testParse("return statement", "return 1;", XVR_AST_NODE_FN_RETURN);
    testParse("return no value", "return;", XVR_AST_NODE_FN_RETURN);

    testParse("break statement", "break;", XVR_AST_NODE_BREAK);
    testParse("continue statement", "continue;", XVR_AST_NODE_CONTINUE);

    testParse("prefix increment", "++x;", XVR_AST_NODE_PREFIX_INCREMENT);
    testParse("prefix decrement", "--x;", XVR_AST_NODE_PREFIX_DECREMENT);
    testParse("postfix increment", "x++;", XVR_AST_NODE_POSTFIX_INCREMENT);
    testParse("postfix decrement", "x--;", XVR_AST_NODE_POSTFIX_DECREMENT);

    testParse("array compound", "[1, 2, 3];", XVR_AST_NODE_COMPOUND);

    testParse("index access", "arr[0];", XVR_AST_NODE_BINARY);
    testParse("index assignment", "arr[0] = 1;", XVR_AST_NODE_BINARY);

    testParse("ternary operator", "true ? 1 : 2;", XVR_AST_NODE_TERNARY);

    testParse("block statement", "{ 1; 2; 3; }", XVR_AST_NODE_BLOCK);

    testParse("import statement", "import foo;", XVR_AST_NODE_IMPORT);
    testParse("import with alias", "import foo as bar;", XVR_AST_NODE_IMPORT);

    testParseMultiple("multiple statements", "1; 2; 3;", 3);
    testParseMultiple("multiple statements with vars",
                      "var x = 1; var y = 2; x + y;", 3);

    testParse("chained method call", "obj.method();", XVR_AST_NODE_BINARY);
    testParse("chained property access", "obj.prop;", XVR_AST_NODE_BINARY);

    testParse("compound assignment +=", "x += 1;", XVR_AST_NODE_BINARY);
    testParse("compound assignment -=", "x -= 1;", XVR_AST_NODE_BINARY);
    testParse("compound assignment *=", "x *= 1;", XVR_AST_NODE_BINARY);
    testParse("compound assignment /=", "x /= 1;", XVR_AST_NODE_BINARY);
    testParse("compound assignment %=", "x %= 1;", XVR_AST_NODE_BINARY);

    printf("Pass CIK (parser): %d/%d\n", passCount, testCount);

    if (passCount != testCount) {
        fprintf(stderr, XVR_CC_ERROR "SOME TESTS FAILED!\n" XVR_CC_RESET);
        return 1;
    }

    return 0;
}
