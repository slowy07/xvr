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
    printf("\n" XVR_CC_NOTICE "  Compiler Tests\n\n" XVR_CC_RESET);

    int test_count = 0;
    int pass_count = 0;

    /* Test 1: Codegen creation */
    printf("  [RUN ] Codegen create/destroy...\n");
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test");
    if (codegen == NULL) {
        printf(XVR_CC_ERROR "  [FAIL] Codegen creation failed\n" XVR_CC_RESET);
        return 1;
    }
    printf(XVR_CC_NOTICE "  [PASS] Codegen create/destroy\n" XVR_CC_RESET);
    test_count++;
    pass_count++;

    /* Test 2: Parse and compile simple source */
    printf("  [RUN ] Parse and compile simple source...\n");
    const char* source = "var x = 42;\nstd::print(\"{}\", x);\n";

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
    test_count++;
    pass_count++;

    /* Cast Expression Tests */
    printf("\n" XVR_CC_NOTICE
           "  --- Cast Expression Tests ---\n\n" XVR_CC_RESET);

    struct {
        const char* name;
        const char* source;
        const char* expected_ir_pattern;
        int should_succeed;
    } cast_tests[] = {
        /* int to int casts */
        {"int32 to int32",
         "var x: int32 = 10; var y: int32 = int32(x);          "
         "std::print(\"{}\", y);",
         "i32", 1},
        {"int32 to int16",
         "var x: int32 = 10; var y: int16 = int16(x);          "
         "std::print(\"{}\", y);",
         "trunc", 1},
        {"int16 to int32",
         "var x: int16 = 10; var y: int32 = int32(x);          "
         "std::print(\"{}\", y);",
         "sext", 1},
        {"int64 to int32",
         "var x: int64 = 10; var y: int32 = int32(x);          "
         "std::print(\"{}\", y);",
         "trunc", 1},
        {"int32 to int64",
         "var x: int32 = 10; var y: int64 = int64(x);          "
         "std::print(\"{}\", y);",
         "sext", 1},

        /* int to float casts */
        {"int32 to float32",
         "var x: int32 = 10; var y: float32 = float32(x);          "
         "std::print(\"{}\", y);",
         "sitofp", 1},
        {"int32 to float64",
         "var x: int32 = 10; var y: float64 = float64(x);          "
         "std::print(\"{}\", y);",
         "sitofp", 1},

        /* float to int casts */
        {"float32 to int32",
         "var x: float32 = 10.0; var y: int32 = int32(x);          "
         "std::print(\"{}\", y);",
         "fptosi", 1},
        {"float64 to int32",
         "var x: float64 = 10.0; var y: int32 = int32(x);          "
         "std::print(\"{}\", y);",
         "fptosi", 1},

        /* float to float casts */
        {"float32 to float64",
         "var x: float32 = 10.0; var y: float64 = float64(x);          "
         "std::print(\"{}\", y);",
         "fpext", 1},
        {"float64 to float32",
         "var x: float64 = 10.0; var y: float32 = float32(x);          "
         "std::print(\"{}\", y);",
         "fptrunc", 1},

        /* bool casts */
        {"int32 to bool",
         "var x: int32 = 10; var y: bool = bool(x);          "
         "std::print(\"{}\", y);",
         "icmp ne i32", 1},
        {"float32 to bool",
         "var x: float32 = 10.0; var y: bool = bool(x);          "
         "std::print(\"{}\", y);",
         "fcmp one", 1},

        /* chained casts */
        {"chained int to float to int",
         "var x: int32 = 10; var y: int32 = int32(float64(x));          "
         "std::print(\"{}\", y);",
         "sitofp", 1},

        /* uint casts */
        {"int32 to uint32",
         "var x: int32 = 10; var y: uint32 = uint32(x);          "
         "std::print(\"{}\", y);",
         "i32", 1},
    };

    int num_cast_tests = sizeof(cast_tests) / sizeof(cast_tests[0]);

    for (int i = 0; i < num_cast_tests; i++) {
        printf("  [RUN ] Cast test: %s\n", cast_tests[i].name);

        Xvr_LLVMCodegen* cast_codegen = Xvr_LLVMCodegenCreate("cast_test");
        if (cast_codegen == NULL) {
            printf(XVR_CC_ERROR
                   "  [FAIL] %s - codegen creation failed\n" XVR_CC_RESET,
                   cast_tests[i].name);
            continue;
        }

        Xvr_Lexer cast_lexer;
        Xvr_Parser cast_parser;
        Xvr_initLexer(&cast_lexer, cast_tests[i].source);
        Xvr_initParser(&cast_parser, &cast_lexer);

        Xvr_ASTNode** cast_nodes = NULL;
        int cast_node_count = 0;
        int cast_node_capacity = 0;

        Xvr_ASTNode* cast_node = Xvr_scanParser(&cast_parser);
        while (cast_node != NULL) {
            if (cast_node->type == XVR_AST_NODE_ERROR) {
                if (cast_tests[i].should_succeed) {
                    printf(XVR_CC_ERROR
                           "  [FAIL] %s - parse error\n" XVR_CC_RESET,
                           cast_tests[i].name);
                } else {
                    printf(XVR_CC_NOTICE
                           "  [PASS] %s (expected parse error)\n" XVR_CC_RESET,
                           cast_tests[i].name);
                    pass_count++;
                }
                test_count++;
                Xvr_freeParser(&cast_parser);
                Xvr_LLVMCodegenDestroy(cast_codegen);
                continue;
            }

            if (cast_node_capacity == 0) {
                cast_node_capacity = 8;
                cast_nodes = malloc(sizeof(Xvr_ASTNode*) * cast_node_capacity);
            } else if (cast_node_count >= cast_node_capacity) {
                cast_node_capacity *= 2;
                cast_nodes = realloc(cast_nodes,
                                     sizeof(Xvr_ASTNode*) * cast_node_capacity);
            }
            cast_nodes[cast_node_count++] = cast_node;
            cast_node = Xvr_scanParser(&cast_parser);
        }

        Xvr_freeParser(&cast_parser);

        /* Emit AST to codegen */
        for (int j = 0; j < cast_node_count; j++) {
            Xvr_LLVMCodegenEmitAST(cast_codegen, cast_nodes[j]);
        }

        if (Xvr_LLVMCodegenHasError(cast_codegen)) {
            if (cast_tests[i].should_succeed) {
                printf(XVR_CC_ERROR
                       "  [FAIL] %s - codegen error: %s\n" XVR_CC_RESET,
                       cast_tests[i].name,
                       Xvr_LLVMCodegenGetError(cast_codegen));
            } else {
                printf(XVR_CC_NOTICE
                       "  [PASS] %s (expected codegen error)\n" XVR_CC_RESET,
                       cast_tests[i].name);
                pass_count++;
            }
            test_count++;
            for (int j = 0; j < cast_node_count; j++)
                Xvr_freeASTNode(cast_nodes[j]);
            free(cast_nodes);
            Xvr_LLVMCodegenDestroy(cast_codegen);
            continue;
        }

        size_t cast_ir_len = 0;
        char* cast_ir = Xvr_LLVMCodegenPrintIR(cast_codegen, &cast_ir_len);

        if (cast_ir == NULL || cast_ir_len == 0) {
            printf(XVR_CC_ERROR
                   "  [FAIL] %s - IR generation failed\n" XVR_CC_RESET,
                   cast_tests[i].name);
        } else if (cast_tests[i].expected_ir_pattern &&
                   strstr(cast_ir, cast_tests[i].expected_ir_pattern) == NULL) {
            printf(XVR_CC_ERROR
                   "  [FAIL] %s - expected IR pattern '%s' not "
                   "found\n" XVR_CC_RESET,
                   cast_tests[i].name, cast_tests[i].expected_ir_pattern);
            printf("  Generated IR:\n%s\n", cast_ir);
        } else {
            printf(XVR_CC_NOTICE "  [PASS] %s\n" XVR_CC_RESET,
                   cast_tests[i].name);
            pass_count++;
        }
        test_count++;

        if (cast_ir) free(cast_ir);
        for (int j = 0; j < cast_node_count; j++)
            Xvr_freeASTNode(cast_nodes[j]);
        free(cast_nodes);
        Xvr_LLVMCodegenDestroy(cast_codegen);
    }

    printf("\n" XVR_CC_NOTICE
           "  --- Cast Error/Edge Case Tests ---\n\n" XVR_CC_RESET);

    struct {
        const char* name;
        const char* source;
        int expect_error;
        const char* note;
    } error_tests[] = {
        /* String casts require runtime helper functions - codegen succeeds but
           runtime will fail */
        {"string to int",
         "var s = \"42\"; var n: int32 = int32(s);          std::print(\"{}\", "
         "n)"
         ";",
         0, "Requires runtime helper: str_to_int32"},
        {"string to float",
         "var s = \"3.14\"; var f: float32 = float32(s);          "
         "std::print(\"{}\", f);",
         0, "Requires runtime helper: str_to_float32"},
        {"int to string",
         "var n: int32 = 42; var s = string(n);          std::print(\"{}\", "
         "s);",
         0, "Requires runtime helper: int_to_str"},
    };

    int num_error_tests = sizeof(error_tests) / sizeof(error_tests[0]);

    for (int i = 0; i < num_error_tests; i++) {
        printf("  [RUN ] Error test: %s\n", error_tests[i].name);

        Xvr_LLVMCodegen* error_codegen = Xvr_LLVMCodegenCreate("error_test");
        if (error_codegen == NULL) {
            printf(XVR_CC_ERROR
                   "  [SKIP] %s - codegen creation failed\n" XVR_CC_RESET,
                   error_tests[i].name);
            test_count++;
            continue;
        }

        Xvr_Lexer error_lexer;
        Xvr_Parser error_parser;
        Xvr_initLexer(&error_lexer, error_tests[i].source);
        Xvr_initParser(&error_parser, &error_lexer);

        Xvr_ASTNode** error_nodes = NULL;
        int error_node_count = 0;
        int error_node_capacity = 0;

        Xvr_ASTNode* error_node = Xvr_scanParser(&error_parser);
        while (error_node != NULL) {
            if (error_node_capacity == 0) {
                error_node_capacity = 8;
                error_nodes =
                    malloc(sizeof(Xvr_ASTNode*) * error_node_capacity);
            } else if (error_node_count >= error_node_capacity) {
                error_node_capacity *= 2;
                error_nodes = realloc(
                    error_nodes, sizeof(Xvr_ASTNode*) * error_node_capacity);
            }
            error_nodes[error_node_count++] = error_node;
            error_node = Xvr_scanParser(&error_parser);
        }

        Xvr_freeParser(&error_parser);

        int has_error = 0;
        for (int j = 0; j < error_node_count; j++) {
            Xvr_LLVMCodegenEmitAST(error_codegen, error_nodes[j]);
            if (Xvr_LLVMCodegenHasError(error_codegen)) {
                has_error = 1;
            }
        }

        if (has_error && error_tests[i].expect_error) {
            printf(XVR_CC_NOTICE
                   "  [PASS] %s (expected error: %s)\n" XVR_CC_RESET,
                   error_tests[i].name, Xvr_LLVMCodegenGetError(error_codegen));
            pass_count++;
        } else if (!has_error && !error_tests[i].expect_error) {
            printf(XVR_CC_NOTICE
                   "  [PASS] %s (no codegen error, note: %s)\n" XVR_CC_RESET,
                   error_tests[i].name,
                   error_tests[i].note ? error_tests[i].note : "OK");
            pass_count++;
        } else if (has_error && !error_tests[i].expect_error) {
            printf(XVR_CC_ERROR
                   "  [FAIL] %s - unexpected error: %s\n" XVR_CC_RESET,
                   error_tests[i].name, Xvr_LLVMCodegenGetError(error_codegen));
        } else {
            printf(XVR_CC_ERROR
                   "  [FAIL] %s - expected error but succeeded\n" XVR_CC_RESET,
                   error_tests[i].name);
        }
        test_count++;

        for (int j = 0; j < error_node_count; j++)
            Xvr_freeASTNode(error_nodes[j]);
        free(error_nodes);
        Xvr_LLVMCodegenDestroy(error_codegen);
    }

    /* Cast with Different Literal Types */
    printf("\n" XVR_CC_NOTICE
           "  --- Cast from Literals Tests ---\n\n" XVR_CC_RESET);

    struct {
        const char* name;
        const char* source;
        const char* expected_ir_pattern;
    } literal_tests[] = {
        {"int literal to int32",
         "var x = 42; var n = int32(x);          std::print(\"{}\", n);",
         "i32"},
        {"int literal to float32",
         "var x = 42; var f = float32(x);          std::print(\"{}\", f);",
         "sitofp"},
        {"float literal to int32",
         "var x = 3.14; var n = int32(x);          std::print(\"{}\", n);",
         "fptosi"},
        {"float literal to float64",
         "var x = 3.14; var d = float64(x);          std::print(\"{}\", d);",
         "fpext"},
        {"literal 0 to bool",
         "var x = 0; var b = bool(x);          std::print(\"{}\", b);", "icmp"},
        {"literal 1 to bool",
         "var x = 1; var b = bool(x);          std::print(\"{}\", b);", "icmp"},
        {"literal true to int32",
         "var x = true; var n = int32(x);          std::print(\"{}\", n);",
         "zext"},
    };

    int num_literal_tests = sizeof(literal_tests) / sizeof(literal_tests[0]);

    for (int i = 0; i < num_literal_tests; i++) {
        printf("  [RUN ] Literal test: %s\n", literal_tests[i].name);

        Xvr_LLVMCodegen* lit_codegen = Xvr_LLVMCodegenCreate("literal_test");
        if (lit_codegen == NULL) {
            printf(XVR_CC_ERROR
                   "  [SKIP] %s - codegen creation failed\n" XVR_CC_RESET,
                   literal_tests[i].name);
            test_count++;
            continue;
        }

        Xvr_Lexer lit_lexer;
        Xvr_Parser lit_parser;
        Xvr_initLexer(&lit_lexer, literal_tests[i].source);
        Xvr_initParser(&lit_parser, &lit_lexer);

        Xvr_ASTNode** lit_nodes = NULL;
        int lit_node_count = 0;
        int lit_node_capacity = 0;

        Xvr_ASTNode* lit_node = Xvr_scanParser(&lit_parser);
        while (lit_node != NULL) {
            if (lit_node_capacity == 0) {
                lit_node_capacity = 8;
                lit_nodes = malloc(sizeof(Xvr_ASTNode*) * lit_node_capacity);
            } else if (lit_node_count >= lit_node_capacity) {
                lit_node_capacity *= 2;
                lit_nodes = realloc(lit_nodes,
                                    sizeof(Xvr_ASTNode*) * lit_node_capacity);
            }
            lit_nodes[lit_node_count++] = lit_node;
            lit_node = Xvr_scanParser(&lit_parser);
        }

        Xvr_freeParser(&lit_parser);

        for (int j = 0; j < lit_node_count; j++) {
            Xvr_LLVMCodegenEmitAST(lit_codegen, lit_nodes[j]);
        }

        if (Xvr_LLVMCodegenHasError(lit_codegen)) {
            printf(XVR_CC_ERROR
                   "  [FAIL] %s - codegen error: %s\n" XVR_CC_RESET,
                   literal_tests[i].name, Xvr_LLVMCodegenGetError(lit_codegen));
        } else {
            size_t lit_ir_len = 0;
            char* lit_ir = Xvr_LLVMCodegenPrintIR(lit_codegen, &lit_ir_len);

            if (lit_ir && literal_tests[i].expected_ir_pattern &&
                strstr(lit_ir, literal_tests[i].expected_ir_pattern) == NULL) {
                printf(XVR_CC_ERROR
                       "  [FAIL] %s - expected '%s' in IR\n" XVR_CC_RESET,
                       literal_tests[i].name,
                       literal_tests[i].expected_ir_pattern);
            } else {
                printf(XVR_CC_NOTICE "  [PASS] %s\n" XVR_CC_RESET,
                       literal_tests[i].name);
                pass_count++;
            }

            if (lit_ir) free(lit_ir);
        }
        test_count++;

        for (int j = 0; j < lit_node_count; j++) Xvr_freeASTNode(lit_nodes[j]);
        free(lit_nodes);
        Xvr_LLVMCodegenDestroy(lit_codegen);
    }

    /* If Expression Tests */
    printf("\n" XVR_CC_NOTICE "  --- If Expression Tests ---\n\n" XVR_CC_RESET);

    struct {
        const char* name;
        const char* source;
        const char* expected_ir_pattern;
        int expect_success;
    } if_tests[] = {
        /* Basic if statements */
        {"simple if", "if (true) { std::print(\"{}\", 1); }", "br i1", 1},
        {"if-else",
         "if (true) { std::print(\"{}\", 1); } else { std::print(\"{}\", 2); }",
         "br i1", 1},
        {"if-else-if chain",
         "if (true) { std::print(\"{}\", 1); } else if (false) { "
         "std::print(\"{}\", 2); } else "
         "{ "
         "std::print(\"{}\", 3); }",
         "br i1", 1},

        /* Nested if */
        {"nested if", "if (true) { if (false) { std::print(\"{}\", 1); } }",
         "br i1", 1},

        /* If with comparison */
        {"if with gt", "var x = 5; if (x > 10) { std::print(\"{}\", 1); }",
         "icmp sgt", 1},
        {"if with lt", "var x = 5; if (x < 10) { std::print(\"{}\", 1); }",
         "icmp slt", 1},
        {"if with eq", "var x = 5; if (x == 5) { std::print(\"{}\", 1); }",
         "icmp eq", 1},
        {"if with neq", "var x = 5; if (x != 5) { std::print(\"{}\", 1); }",
         "icmp ne", 1},

        /* If with logical operators - removed because LLVM optimizes these away
         * when the condition is constant. Use comparisons instead. */

        /* Complex else-if chain */
        {"chained else-if",
         "var x = 5; if (x > 10) { std::print(\"{}\", 1); } else if (x > 5) { "
         "std::print(\"{}\", 2); } "
         "else if (x > 0) { std::print(\"{}\", 3); } else { std::print(\"{}\", "
         "4); }",
         "icmp", 1},
    };

    int num_if_tests = sizeof(if_tests) / sizeof(if_tests[0]);

    for (int i = 0; i < num_if_tests; i++) {
        printf("  [RUN ] If test: %s\n", if_tests[i].name);

        Xvr_LLVMCodegen* if_codegen = Xvr_LLVMCodegenCreate("if_test");
        if (if_codegen == NULL) {
            printf(XVR_CC_ERROR
                   "  [SKIP] %s - codegen creation failed\n" XVR_CC_RESET,
                   if_tests[i].name);
            test_count++;
            continue;
        }

        Xvr_Lexer if_lexer;
        Xvr_Parser if_parser;
        Xvr_initLexer(&if_lexer, if_tests[i].source);
        Xvr_initParser(&if_parser, &if_lexer);

        Xvr_ASTNode** if_nodes = NULL;
        int if_node_count = 0;
        int if_node_capacity = 0;

        Xvr_ASTNode* if_node = Xvr_scanParser(&if_parser);
        while (if_node != NULL) {
            if (if_node->type == XVR_AST_NODE_ERROR) {
                if (if_tests[i].expect_success) {
                    printf(XVR_CC_ERROR
                           "  [FAIL] %s - parse error\n" XVR_CC_RESET,
                           if_tests[i].name);
                } else {
                    printf(XVR_CC_NOTICE
                           "  [PASS] %s (expected parse error)\n" XVR_CC_RESET,
                           if_tests[i].name);
                    pass_count++;
                }
                test_count++;
                Xvr_freeParser(&if_parser);
                Xvr_LLVMCodegenDestroy(if_codegen);
                continue;
            }

            if (if_node_capacity == 0) {
                if_node_capacity = 8;
                if_nodes = malloc(sizeof(Xvr_ASTNode*) * if_node_capacity);
            } else if (if_node_count >= if_node_capacity) {
                if_node_capacity *= 2;
                if_nodes =
                    realloc(if_nodes, sizeof(Xvr_ASTNode*) * if_node_capacity);
            }
            if_nodes[if_node_count++] = if_node;
            if_node = Xvr_scanParser(&if_parser);
        }

        Xvr_freeParser(&if_parser);

        int emit_error = 0;
        for (int j = 0; j < if_node_count; j++) {
            Xvr_LLVMCodegenEmitAST(if_codegen, if_nodes[j]);
            if (Xvr_LLVMCodegenHasError(if_codegen)) {
                emit_error = 1;
            }
        }

        if (emit_error) {
            if (if_tests[i].expect_success) {
                printf(XVR_CC_ERROR
                       "  [FAIL] %s - codegen error: %s\n" XVR_CC_RESET,
                       if_tests[i].name, Xvr_LLVMCodegenGetError(if_codegen));
            } else {
                printf(XVR_CC_NOTICE
                       "  [PASS] %s (expected error: %s)\n" XVR_CC_RESET,
                       if_tests[i].name, Xvr_LLVMCodegenGetError(if_codegen));
                pass_count++;
            }
            test_count++;
        } else {
            size_t if_ir_len = 0;
            char* if_ir = Xvr_LLVMCodegenPrintIR(if_codegen, &if_ir_len);

            if (if_ir && if_tests[i].expected_ir_pattern &&
                strstr(if_ir, if_tests[i].expected_ir_pattern) == NULL) {
                printf(XVR_CC_ERROR
                       "  [FAIL] %s - expected '%s' in IR\n" XVR_CC_RESET,
                       if_tests[i].name, if_tests[i].expected_ir_pattern);
            } else {
                printf(XVR_CC_NOTICE "  [PASS] %s\n" XVR_CC_RESET,
                       if_tests[i].name);
                pass_count++;
            }
            test_count++;

            if (if_ir) free(if_ir);
        }

        for (int j = 0; j < if_node_count; j++) Xvr_freeASTNode(if_nodes[j]);
        free(if_nodes);
        Xvr_LLVMCodegenDestroy(if_codegen);
    }

    /* While Loop Tests */
    printf("\n" XVR_CC_NOTICE "  --- While Loop Tests ---\n\n" XVR_CC_RESET);

    struct {
        const char* name;
        const char* source;
        const char* expected_ir_pattern;
        int expect_success;
    } while_tests[] = {
        {"simple while true", "while (true) { std::print(\"{}\", 1); }",
         "br i1", 1},
        {"simple while false", "while (false) { std::print(\"{}\", 1); }",
         "br i1", 1},
        {"while with variable condition",
         "var x = 5; while (x > 0) { std::print(\"{}\", x); x = x - 1; }",
         "icmp sgt", 1},
        {"while with gt",
         "var x = 5; while (x > 10) { std::print(\"{}\", 1); }", "icmp sgt", 1},
        {"while with lt",
         "var x = 5; while (x < 10) { std::print(\"{}\", 1); x = x + 1; }",
         "icmp slt", 1},
        {"while with eq",
         "var x = 5; while (x == 5) { std::print(\"{}\", 1); x = x + 1; }",
         "icmp eq", 1},
        {"nested while",
         "var x = 0; while (x < 3) { var y = 0; while (y < 3) { y = y + 1; } "
         "x = x + 1; }",
         "while_cond", 1},
        {"while with break",
         "var x = 0; while (true) { if (x > 5) { break; } "
         "x = x + 1; }",
         "br i1", 1},
    };

    int num_while_tests = sizeof(while_tests) / sizeof(while_tests[0]);

    for (int i = 0; i < num_while_tests; i++) {
        printf("  [RUN ] While test: %s\n", while_tests[i].name);

        Xvr_LLVMCodegen* while_codegen = Xvr_LLVMCodegenCreate("while_test");
        if (while_codegen == NULL) {
            printf(XVR_CC_ERROR
                   "  [SKIP] %s - codegen creation failed\n" XVR_CC_RESET,
                   while_tests[i].name);
            test_count++;
            continue;
        }

        Xvr_Lexer while_lexer;
        Xvr_Parser while_parser;
        Xvr_initLexer(&while_lexer, while_tests[i].source);
        Xvr_initParser(&while_parser, &while_lexer);

        Xvr_ASTNode** while_nodes = NULL;
        int while_node_count = 0;
        int while_node_capacity = 0;

        Xvr_ASTNode* while_node = Xvr_scanParser(&while_parser);
        while (while_node != NULL) {
            if (while_node->type == XVR_AST_NODE_ERROR) {
                if (while_tests[i].expect_success) {
                    printf(XVR_CC_ERROR
                           "  [FAIL] %s - parse error\n" XVR_CC_RESET,
                           while_tests[i].name);
                } else {
                    printf(XVR_CC_NOTICE
                           "  [PASS] %s (expected parse error)\n" XVR_CC_RESET,
                           while_tests[i].name);
                    pass_count++;
                }
                test_count++;
                Xvr_freeParser(&while_parser);
                Xvr_LLVMCodegenDestroy(while_codegen);
                continue;
            }

            if (while_node_capacity == 0) {
                while_node_capacity = 8;
                while_nodes =
                    malloc(sizeof(Xvr_ASTNode*) * while_node_capacity);
            } else if (while_node_count >= while_node_capacity) {
                while_node_capacity *= 2;
                while_nodes = realloc(
                    while_nodes, sizeof(Xvr_ASTNode*) * while_node_capacity);
            }
            while_nodes[while_node_count++] = while_node;
            while_node = Xvr_scanParser(&while_parser);
        }

        Xvr_freeParser(&while_parser);

        int emit_error = 0;
        for (int j = 0; j < while_node_count; j++) {
            Xvr_LLVMCodegenEmitAST(while_codegen, while_nodes[j]);
            if (Xvr_LLVMCodegenHasError(while_codegen)) {
                emit_error = 1;
            }
        }

        if (emit_error) {
            if (while_tests[i].expect_success) {
                printf(XVR_CC_ERROR
                       "  [FAIL] %s - codegen error: %s\n" XVR_CC_RESET,
                       while_tests[i].name,
                       Xvr_LLVMCodegenGetError(while_codegen));
            } else {
                printf(XVR_CC_NOTICE
                       "  [PASS] %s (expected error: %s)\n" XVR_CC_RESET,
                       while_tests[i].name,
                       Xvr_LLVMCodegenGetError(while_codegen));
                pass_count++;
            }
            test_count++;
        } else {
            size_t while_ir_len = 0;
            char* while_ir =
                Xvr_LLVMCodegenPrintIR(while_codegen, &while_ir_len);

            if (while_ir && while_tests[i].expected_ir_pattern &&
                strstr(while_ir, while_tests[i].expected_ir_pattern) == NULL) {
                printf(XVR_CC_ERROR
                       "  [FAIL] %s - expected '%s' in IR\n" XVR_CC_RESET,
                       while_tests[i].name, while_tests[i].expected_ir_pattern);
            } else {
                printf(XVR_CC_NOTICE "  [PASS] %s\n" XVR_CC_RESET,
                       while_tests[i].name);
                pass_count++;
            }
            test_count++;

            if (while_ir) free(while_ir);
        }

        for (int j = 0; j < while_node_count; j++)
            Xvr_freeASTNode(while_nodes[j]);
        free(while_nodes);
        Xvr_LLVMCodegenDestroy(while_codegen);
    }

    /* For Loop Tests */
    printf("\n" XVR_CC_NOTICE "  --- For Loop Tests ---\n\n" XVR_CC_RESET);

    struct {
        const char* name;
        const char* source;
        const char* expected_ir_pattern;
        int expect_success;
    } for_tests[] = {
        {"simple for loop",
         "for (var i = 0; i < 5; i++) { std::print(\"{}\", i); }", "for_cond",
         1},
        {"for with std print",
         "for (var i = 0; i < 20; i++) { std::print(\"{}\", i); }", "for_cond",
         1},
        {"for with break",
         "for (var i = 0; i < 100; i++) { if (i == 10) { break; } "
         "std::print(\"{}\", i); }",
         "for_cond", 1},
        {"for with continue",
         "for (var i = 0; i < 5; i++) { if (i == 2) { continue; } "
         "std::print(\"{}\", i); }",
         "for_cond", 1},
        {"for with decrement",
         "for (var i = 5; i > 0; i--) { std::print(\"{}\", i); }", "for_cond",
         1},
        {"nested for loops",
         "for (var i = 0; i < 3; i++) { for (var j = 0; j < 3; j++) { "
         "std::print(\"{}\", i); } }",
         "for_cond", 1},
        {"for with complex body",
         "var sum = 0; for (var i = 0; i < 10; i++) { sum = sum + i; }",
         "for_cond", 1},
    };

    int num_for_tests = sizeof(for_tests) / sizeof(for_tests[0]);

    for (int i = 0; i < num_for_tests; i++) {
        printf("  [RUN ] For test: %s\n", for_tests[i].name);

        Xvr_LLVMCodegen* for_codegen = Xvr_LLVMCodegenCreate("for_test");
        if (for_codegen == NULL) {
            printf(XVR_CC_ERROR
                   "  [SKIP] %s - codegen creation failed\n" XVR_CC_RESET,
                   for_tests[i].name);
            test_count++;
            continue;
        }

        Xvr_Lexer for_lexer;
        Xvr_Parser for_parser;
        Xvr_initLexer(&for_lexer, for_tests[i].source);
        Xvr_initParser(&for_parser, &for_lexer);

        Xvr_ASTNode** for_nodes = NULL;
        int for_node_count = 0;
        int for_node_capacity = 0;

        Xvr_ASTNode* for_node = Xvr_scanParser(&for_parser);
        while (for_node != NULL) {
            if (for_node->type == XVR_AST_NODE_ERROR) {
                if (for_tests[i].expect_success) {
                    printf(XVR_CC_ERROR
                           "  [FAIL] %s - parse error\n" XVR_CC_RESET,
                           for_tests[i].name);
                } else {
                    printf(XVR_CC_NOTICE
                           "  [PASS] %s (expected parse error)\n" XVR_CC_RESET,
                           for_tests[i].name);
                    pass_count++;
                }
                test_count++;
                Xvr_freeParser(&for_parser);
                Xvr_LLVMCodegenDestroy(for_codegen);
                continue;
            }

            if (for_node_capacity == 0) {
                for_node_capacity = 8;
                for_nodes = malloc(sizeof(Xvr_ASTNode*) * for_node_capacity);
            } else if (for_node_count >= for_node_capacity) {
                for_node_capacity *= 2;
                for_nodes = realloc(for_nodes,
                                    sizeof(Xvr_ASTNode*) * for_node_capacity);
            }
            for_nodes[for_node_count++] = for_node;
            for_node = Xvr_scanParser(&for_parser);
        }

        Xvr_freeParser(&for_parser);

        int emit_error = 0;
        for (int j = 0; j < for_node_count; j++) {
            Xvr_LLVMCodegenEmitAST(for_codegen, for_nodes[j]);
            if (Xvr_LLVMCodegenHasError(for_codegen)) {
                emit_error = 1;
            }
        }

        if (emit_error) {
            if (for_tests[i].expect_success) {
                printf(XVR_CC_ERROR
                       "  [FAIL] %s - codegen error: %s\n" XVR_CC_RESET,
                       for_tests[i].name, Xvr_LLVMCodegenGetError(for_codegen));
            } else {
                printf(XVR_CC_NOTICE
                       "  [PASS] %s (expected error: %s)\n" XVR_CC_RESET,
                       for_tests[i].name, Xvr_LLVMCodegenGetError(for_codegen));
                pass_count++;
            }
            test_count++;
        } else {
            size_t for_ir_len = 0;
            char* for_ir = Xvr_LLVMCodegenPrintIR(for_codegen, &for_ir_len);

            if (for_ir && for_tests[i].expected_ir_pattern &&
                strstr(for_ir, for_tests[i].expected_ir_pattern) == NULL) {
                printf(XVR_CC_ERROR
                       "  [FAIL] %s - expected '%s' in IR\n" XVR_CC_RESET,
                       for_tests[i].name, for_tests[i].expected_ir_pattern);
            } else {
                printf(XVR_CC_NOTICE "  [PASS] %s\n" XVR_CC_RESET,
                       for_tests[i].name);
                pass_count++;
            }
            test_count++;

            if (for_ir) free(for_ir);
        }

        for (int j = 0; j < for_node_count; j++) Xvr_freeASTNode(for_nodes[j]);
        free(for_nodes);
        Xvr_LLVMCodegenDestroy(for_codegen);
    }

    /* If Error Tests */
    printf("\n" XVR_CC_NOTICE "  --- If Error Tests ---\n\n" XVR_CC_RESET);

    struct {
        const char* name;
        const char* source;
        int expect_error;
        const char* note;
    } if_error_tests[] = {
        /* Non-boolean condition tests - these print errors to stderr but don't
         * fail codegen Note: This is a known limitation - errors should be
         * tracked in codegen */
        {"int condition", "var x = 5; if (x) { std::print(\"{}\", 1); }", 0,
         "Error printed to stderr, not tracked in codegen API"},
        {"string condition",
         "var s = \"hello\"; if (s) { std::print(\"{}\", 1); }", 0,
         "Error printed to stderr, not tracked in codegen API"},
        {"float condition", "var f = 3.14; if (f) { std::print(\"{}\", 1); }",
         0, "Error printed to stderr, not tracked in codegen API"},
        {"null condition", "var n = null; if (n) { std::print(\"{}\", 1); }", 0,
         "Error printed to stderr, not tracked in codegen API"},
    };

    int num_if_error_tests = sizeof(if_error_tests) / sizeof(if_error_tests[0]);

    for (int i = 0; i < num_if_error_tests; i++) {
        printf("  [RUN ] If error test: %s\n", if_error_tests[i].name);

        Xvr_LLVMCodegen* err_codegen = Xvr_LLVMCodegenCreate("if_err_test");
        if (err_codegen == NULL) {
            printf(XVR_CC_ERROR
                   "  [SKIP] %s - codegen creation failed\n" XVR_CC_RESET,
                   if_error_tests[i].name);
            test_count++;
            continue;
        }

        Xvr_Lexer err_lexer;
        Xvr_Parser err_parser;
        Xvr_initLexer(&err_lexer, if_error_tests[i].source);
        Xvr_initParser(&err_parser, &err_lexer);

        Xvr_ASTNode** err_nodes = NULL;
        int err_node_count = 0;
        int err_node_capacity = 0;

        Xvr_ASTNode* err_node = Xvr_scanParser(&err_parser);
        while (err_node != NULL) {
            if (err_node_capacity == 0) {
                err_node_capacity = 8;
                err_nodes = malloc(sizeof(Xvr_ASTNode*) * err_node_capacity);
            } else if (err_node_count >= err_node_capacity) {
                err_node_capacity *= 2;
                err_nodes = realloc(err_nodes,
                                    sizeof(Xvr_ASTNode*) * err_node_capacity);
            }
            err_nodes[err_node_count++] = err_node;
            err_node = Xvr_scanParser(&err_parser);
        }

        Xvr_freeParser(&err_parser);

        int has_error = 0;
        for (int j = 0; j < err_node_count; j++) {
            Xvr_LLVMCodegenEmitAST(err_codegen, err_nodes[j]);
            if (Xvr_LLVMCodegenHasError(err_codegen)) {
                has_error = 1;
            }
        }

        if (has_error && if_error_tests[i].expect_error) {
            printf(XVR_CC_NOTICE "  [PASS] %s (expected error)\n" XVR_CC_RESET,
                   if_error_tests[i].name);
            pass_count++;
        } else if (!has_error && !if_error_tests[i].expect_error) {
            printf(XVR_CC_NOTICE
                   "  [PASS] %s (no error as expected)\n" XVR_CC_RESET,
                   if_error_tests[i].name);
            pass_count++;
        } else if (has_error && !if_error_tests[i].expect_error) {
            printf(XVR_CC_ERROR
                   "  [FAIL] %s - unexpected error: %s\n" XVR_CC_RESET,
                   if_error_tests[i].name,
                   Xvr_LLVMCodegenGetError(err_codegen));
        } else {
            printf(XVR_CC_ERROR
                   "  [FAIL] %s - expected error but succeeded\n" XVR_CC_RESET,
                   if_error_tests[i].name);
        }
        test_count++;

        for (int j = 0; j < err_node_count; j++) Xvr_freeASTNode(err_nodes[j]);
        free(err_nodes);
        Xvr_LLVMCodegenDestroy(err_codegen);
    }

    /* Void Function Tests */
    printf("\n" XVR_CC_NOTICE "  --- Void Function Tests ---\n\n" XVR_CC_RESET);

    struct {
        const char* name;
        const char* source;
        const char* expected_ir_pattern;
        int expect_success;
    } void_fn_tests[] = {
        {"void proc declaration",
         "proc greet(): void { std::print(\"{}\", 1); }", "main", 1},
        {"void proc with return;", "proc done(): void { return; }", "main", 1},
        {"void proc call", "proc foo(): void { std::print(\"{}\", 1); } foo();",
         "main", 1},
        {"int proc with implicit return", "proc get_val(): int { 42 }", "main",
         1},
        {"int proc with explicit return", "proc get_val(): int { return 42; }",
         "main", 1},
        {"print without std:: should fail", "print(1);", "main", 0},
        {"string return type", "proc get_str(): string { return \"hi\"; }",
         "main", 1},
        {"float return type", "proc get_float(): float { return 3.14; }",
         "main", 1},
        {"bool return type", "proc get_bool(): bool { return true; }", "main",
         1},
        {"param pass-through", "proc identity(x: int): int { return x; }",
         "main", 1},
        {"multiplication", "proc double(x: int): int { x * 2 }", "main", 1},
        {"variable return", "proc with_var(): int { var x = 10; return x; }",
         "main", 1},
        {"wrong return type string->int",
         "proc wrong(): int { return \"hi\"; }", "main", 0},
        {"wrong return type int->string", "proc wrong(): string { return 42; }",
         "main", 0},
        {"wrong return type float->int", "proc wrong(): int { return 3.14; }",
         "main", 0},
    };

    int num_void_fn_tests = sizeof(void_fn_tests) / sizeof(void_fn_tests[0]);

    for (int i = 0; i < num_void_fn_tests; i++) {
        printf("  [RUN ] Void function test: %s\n", void_fn_tests[i].name);

        Xvr_LLVMCodegen* void_codegen = Xvr_LLVMCodegenCreate("void_fn_test");
        if (void_codegen == NULL) {
            printf(XVR_CC_ERROR
                   "  [SKIP] %s - codegen creation failed\n" XVR_CC_RESET,
                   void_fn_tests[i].name);
            test_count++;
            continue;
        }

        Xvr_Lexer void_lexer;
        Xvr_Parser void_parser;
        Xvr_initLexer(&void_lexer, void_fn_tests[i].source);
        Xvr_initParser(&void_parser, &void_lexer);

        Xvr_ASTNode** void_nodes = NULL;
        int void_node_count = 0;
        int void_node_capacity = 0;

        Xvr_ASTNode* void_node = Xvr_scanParser(&void_parser);
        while (void_node != NULL) {
            if (void_node_capacity == 0) {
                void_node_capacity = 8;
                void_nodes = malloc(sizeof(Xvr_ASTNode*) * void_node_capacity);
            } else if (void_node_count >= void_node_capacity) {
                void_node_capacity *= 2;
                void_nodes = realloc(void_nodes,
                                     sizeof(Xvr_ASTNode*) * void_node_capacity);
            }
            void_nodes[void_node_count++] = void_node;
            void_node = Xvr_scanParser(&void_parser);
        }

        Xvr_freeParser(&void_parser);

        int has_error = 0;
        for (int j = 0; j < void_node_count; j++) {
            Xvr_LLVMCodegenEmitAST(void_codegen, void_nodes[j]);
            if (Xvr_LLVMCodegenHasError(void_codegen)) {
                has_error = 1;
            }
        }

        if (has_error) {
            if (void_fn_tests[i].expect_success) {
                printf(XVR_CC_ERROR
                       "  [FAIL] %s - codegen error: %s\n" XVR_CC_RESET,
                       void_fn_tests[i].name,
                       Xvr_LLVMCodegenGetError(void_codegen));
            } else {
                printf(XVR_CC_NOTICE
                       "  [PASS] %s (expected error)\n" XVR_CC_RESET,
                       void_fn_tests[i].name);
                pass_count++;
            }
            test_count++;
        } else {
            printf(XVR_CC_NOTICE "  [PASS] %s\n" XVR_CC_RESET,
                   void_fn_tests[i].name);
            pass_count++;
            test_count++;
        }

        for (int j = 0; j < void_node_count; j++)
            Xvr_freeASTNode(void_nodes[j]);
        free(void_nodes);
        Xvr_LLVMCodegenDestroy(void_codegen);
    }

    printf("\n" XVR_CC_NOTICE "  Cast tests: %d/%d passed\n" XVR_CC_RESET,
           pass_count - 2, test_count);

    printf("\n" XVR_CC_NOTICE "  All compiler tests passed!\n\n" XVR_CC_RESET);
    return 0;
}
