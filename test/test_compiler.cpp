#include <catch2/catch_test_macros.hpp>
#include "xvr_lexer.h"
#include "xvr_parser.h"
#include "xvr_ast_node.h"
#include "adapters/llvm/xvr_llvm_codegen.h"

TEST_CASE("Codegen create/destroy", "[compiler][llvm]") {
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test");
    REQUIRE(codegen != nullptr);
    Xvr_LLVMCodegenDestroy(codegen);
}

TEST_CASE("Parse and compile simple source", "[compiler][llvm]") {
    const char* source = "var x = 42;\n";

    Xvr_Lexer lexer;
    Xvr_Parser parser;
    Xvr_initLexer(&lexer, source);
    Xvr_initParser(&parser, &lexer);

    Xvr_ASTNode** nodes = nullptr;
    int nodeCount = 0;
    int nodeCapacity = 0;

    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    while (node != nullptr) {
        if (node->type == XVR_AST_NODE_ERROR) {
            Xvr_freeParser(&parser);
            FAIL("Parse error occurred");
        }

        if (nodeCount >= nodeCapacity) {
            nodeCapacity = nodeCapacity < 8 ? 8 : nodeCapacity * 2;
            nodes = reinterpret_cast<Xvr_ASTNode**>(realloc(nodes, sizeof(Xvr_ASTNode*) * nodeCapacity));
        }
        nodes[nodeCount++] = node;
        node = Xvr_scanParser(&parser);
    }

    Xvr_freeParser(&parser);

    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test");
    REQUIRE(codegen != nullptr);

    for (int i = 0; i < nodeCount; i++) {
        Xvr_LLVMCodegenEmitAST(codegen, nodes[i]);
    }

    REQUIRE_FALSE(Xvr_LLVMCodegenHasError(codegen));

    size_t ir_len = 0;
    char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
    REQUIRE(ir != nullptr);
    REQUIRE(ir_len > 0);
    REQUIRE(strstr(ir, "main") != nullptr);

    free(ir);
    for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
    free(nodes);
    Xvr_LLVMCodegenDestroy(codegen);
}
