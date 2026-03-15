/**
 * XVR Compiler Test Suite
 * Tests for LLVM AOT compilation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/backend/xvr_llvm_codegen.h"
#include "../src/xvr_ast_node.h"
#include "../src/xvr_console_colors.h"
#include "../src/xvr_lexer.h"
#include "../src/xvr_memory.h"
#include "../src/xvr_parser.h"

int run_compiler_tests(void) {
    printf("\n" XVR_CC_NOTICE "========================================\n");
    printf("  Compiler Tests\n");
    printf("========================================\n\n" XVR_CC_RESET);

    /* Test 1: Codegen creation */
    printf("  [RUN ] Codegen create/destroy...\n");
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test");
    if (codegen == NULL) {
        printf(XVR_CC_ERROR "  [FAIL] Codegen creation failed\n" XVR_CC_RESET);
        return 1;
    }
    printf(XVR_CC_NOTICE "  [PASS] Codegen create/destroy\n" XVR_CC_RESET);

    /* Test 2: Parse and compile simple source */
    printf("  [RUN ] Parse and compile simple source...\n");
    const char* source = "var x = 42;\nprint(x);\n";

    Xvr_Lexer lexer;
    Xvr_Parser parser;
    Xvr_initLexer(&lexer, source);
    Xvr_initParser(&parser, &lexer);

    Xvr_ASTNode** nodes = NULL;
    int nodeCount = 0;
    int nodeCapacity = 0;

    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    while (node != NULL) {
        if (node->type == XVR_AST_NODE_ERROR) {
            printf(XVR_CC_ERROR "  [FAIL] Parse error\n" XVR_CC_RESET);
            Xvr_freeParser(&parser);
            Xvr_LLVMCodegenDestroy(codegen);
            return 1;
        }

        if (nodeCount >= nodeCapacity) {
            nodeCapacity = nodeCapacity < 8 ? 8 : nodeCapacity * 2;
            nodes = realloc(nodes, sizeof(Xvr_ASTNode*) * nodeCapacity);
        }
        nodes[nodeCount++] = node;
        node = Xvr_scanParser(&parser);
    }

    Xvr_freeParser(&parser);

    /* Emit AST to codegen */
    for (int i = 0; i < nodeCount; i++) {
        Xvr_LLVMCodegenEmitAST(codegen, nodes[i]);
    }

    if (Xvr_LLVMCodegenHasError(codegen)) {
        printf(XVR_CC_ERROR "  [FAIL] Codegen error: %s\n" XVR_CC_RESET,
               Xvr_LLVMCodegenGetError(codegen));
        for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
        free(nodes);
        Xvr_LLVMCodegenDestroy(codegen);
        return 1;
    }

    /* Test IR generation */
    size_t ir_len = 0;
    char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
    if (ir == NULL || ir_len == 0) {
        printf(XVR_CC_ERROR "  [FAIL] IR generation failed\n" XVR_CC_RESET);
        for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
        free(nodes);
        Xvr_LLVMCodegenDestroy(codegen);
        return 1;
    }

    if (strstr(ir, "main") == NULL) {
        printf(XVR_CC_ERROR
               "  [FAIL] main function not found in IR\n" XVR_CC_RESET);
        free(ir);
        for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
        free(nodes);
        Xvr_LLVMCodegenDestroy(codegen);
        return 1;
    }

    printf(XVR_CC_NOTICE
           "  [PASS] Parse and compile simple source\n" XVR_CC_RESET);

    free(ir);
    for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
    free(nodes);
    Xvr_LLVMCodegenDestroy(codegen);

    printf("\n" XVR_CC_NOTICE "  All compiler tests passed!\n\n" XVR_CC_RESET);
    return 0;
}
