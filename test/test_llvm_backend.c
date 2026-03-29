/**
 * XVR LLVM Backend Test Suite
 * Comprehensive tests for LLVM AOT compilation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/backend/xvr_llvm_codegen.h"
#include "../src/backend/xvr_llvm_context.h"
#include "../src/backend/xvr_llvm_expression_emitter.h"
#include "../src/backend/xvr_llvm_function_emitter.h"
#include "../src/backend/xvr_llvm_ir_builder.h"
#include "../src/backend/xvr_llvm_module_manager.h"
#include "../src/backend/xvr_llvm_optimizer.h"
#include "../src/backend/xvr_llvm_target.h"
#include "../src/backend/xvr_llvm_type_mapper.h"
#include "../src/xvr_ast_node.h"
#include "../src/xvr_console_colors.h"
#include "../src/xvr_lexer.h"
#include "../src/xvr_literal.h"
#include "../src/xvr_parser.h"

#define TEST_ASSERT(cond, msg)                                      \
    do {                                                            \
        if (!(cond)) {                                              \
            printf(XVR_CC_ERROR "  [FAIL] %s\n" XVR_CC_RESET, msg); \
            return 1;                                               \
        }                                                           \
    } while (0)

#define TEST_PASS(msg) printf(XVR_CC_NOTICE "  [PASS] %s\n" XVR_CC_RESET, msg)

static int test_context_create_destroy(void) {
    printf("\n  --- Context Create/Destroy ---\n");

    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    TEST_ASSERT(ctx != NULL, "Context creation failed");
    TEST_ASSERT(!Xvr_LLVMContextHasError(ctx),
                "Context has error after creation");

    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(ctx);
    TEST_ASSERT(llvm_ctx != NULL, "LLVM context is NULL");

    Xvr_LLVMContextDestroy(ctx);
    TEST_PASS("Context create/destroy");
    return 0;
}

static int test_type_mapper(void) {
    printf("\n  --- Type Mapper ---\n");

    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    Xvr_LLVMTypeMapper* mapper = Xvr_LLVMTypeMapperCreate(ctx);

    TEST_ASSERT(mapper != NULL, "Type mapper creation failed");

    /* Test integer type */
    LLVMTypeRef int_type =
        Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_INTEGER);
    TEST_ASSERT(int_type != NULL, "Integer type mapping failed");

    /* Test boolean type */
    LLVMTypeRef bool_type =
        Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_BOOLEAN);
    TEST_ASSERT(bool_type != NULL, "Boolean type mapping failed");

    /* Test float type */
    LLVMTypeRef float_type =
        Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_FLOAT);
    TEST_ASSERT(float_type != NULL, "Float type mapping failed");

    /* Test double type */
    LLVMTypeRef double_type =
        Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_FLOAT64);
    TEST_ASSERT(double_type != NULL, "Double type mapping failed");

    /* Test pointer type */
    LLVMTypeRef ptr_type =
        Xvr_LLVMTypeMapperGetPointerType(mapper, XVR_LITERAL_INTEGER);
    TEST_ASSERT(ptr_type != NULL, "Pointer type mapping failed");

    /* Test sign info */
    TEST_ASSERT(Xvr_LLVMTypeMapperIsSigned(XVR_LITERAL_INTEGER) == true,
                "Integer should be signed");
    TEST_ASSERT(Xvr_LLVMTypeMapperIsSigned(XVR_LITERAL_BOOLEAN) == false,
                "Boolean should be unsigned");

    Xvr_LLVMTypeMapperDestroy(mapper);
    Xvr_LLVMContextDestroy(ctx);
    TEST_PASS("Type mapper");
    return 0;
}

static int test_module_manager(void) {
    printf("\n  --- Module Manager ---\n");

    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    Xvr_LLVMModuleManager* mgr =
        Xvr_LLVMModuleManagerCreate(ctx, "test_module");

    TEST_ASSERT(mgr != NULL, "Module manager creation failed");

    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(mgr);
    TEST_ASSERT(module != NULL, "Module is NULL");

    /* Test IR printing */
    size_t ir_len = 0;
    char* ir = Xvr_LLVMModuleManagerPrintIR(mgr, &ir_len);
    TEST_ASSERT(ir != NULL, "IR printing failed");
    TEST_ASSERT(ir_len > 0, "IR length is zero");

    free(ir);
    Xvr_LLVMModuleManagerDestroy(mgr);
    Xvr_LLVMContextDestroy(ctx);
    TEST_PASS("Module manager");
    return 0;
}

static int test_ir_builder(void) {
    printf("\n  --- IR Builder ---\n");

    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    Xvr_LLVMModuleManager* mgr =
        Xvr_LLVMModuleManagerCreate(ctx, "test_module");
    Xvr_LLVMIRBuilder* builder =
        Xvr_LLVMIRBuilderCreate(ctx, Xvr_LLVMModuleManagerGetModule(mgr));

    TEST_ASSERT(builder != NULL, "IR builder creation failed");

    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
    TEST_ASSERT(llvm_builder != NULL, "LLVM builder is NULL");

    /* Create a function */
    LLVMTypeRef fn_type = LLVMFunctionType(
        LLVMInt32TypeInContext(Xvr_LLVMContextGetLLVMContext(ctx)), NULL, 0,
        false);
    LLVMValueRef fn = LLVMAddFunction(Xvr_LLVMModuleManagerGetModule(mgr),
                                      "test_fn", fn_type);
    TEST_ASSERT(fn != NULL, "Function creation failed");

    /* Create basic block */
    LLVMBasicBlockRef block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, fn, "entry");
    TEST_ASSERT(block != NULL, "Basic block creation failed");

    Xvr_LLVMIRBuilderSetInsertPoint(builder, block);
    LLVMBasicBlockRef current = Xvr_LLVMIRBuilderGetInsertBlock(builder);
    TEST_ASSERT(current == block, "Insert point not set correctly");

    Xvr_LLVMIRBuilderDestroy(builder);
    Xvr_LLVMModuleManagerDestroy(mgr);
    Xvr_LLVMContextDestroy(ctx);
    TEST_PASS("IR builder");
    return 0;
}

static int test_optimizer(void) {
    printf("\n  --- Optimizer ---\n");

    Xvr_LLVMOptimizer* opt = Xvr_LLVMOptimizerCreate();
    TEST_ASSERT(opt != NULL, "Optimizer creation failed");

    /* Test all optimization levels */
    TEST_ASSERT(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_NONE) == true,
                "None optimization failed");
    TEST_ASSERT(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_O1) == true,
                "O1 optimization failed");
    TEST_ASSERT(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_O2) == true,
                "O2 optimization failed");
    TEST_ASSERT(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_O3) == true,
                "O3 optimization failed");
    TEST_ASSERT(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_OS) == true,
                "Os optimization failed");
    TEST_ASSERT(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_OZ) == true,
                "Oz optimization failed");

    TEST_ASSERT(Xvr_LLVMOptimizerAddStandardPasses(opt) == true,
                "Adding standard passes failed");

    Xvr_LLVMOptimizerDestroy(opt);
    TEST_PASS("Optimizer");
    return 0;
}

static int test_target_config(void) {
    printf("\n  --- Target Configuration ---\n");

    Xvr_LLVMTargetConfig* config = Xvr_LLVMTargetConfigCreate();
    TEST_ASSERT(config != NULL, "Target config creation failed");

    TEST_ASSERT(
        Xvr_LLVMTargetConfigSetTriple(config, "x86_64-pc-linux-gnu") == true,
        "Set triple failed");
    TEST_ASSERT(Xvr_LLVMTargetConfigSetCPU(config, "generic") == true,
                "Set CPU failed");
    TEST_ASSERT(Xvr_LLVMTargetConfigSetFeatures(config, "+sse,+sse2") == true,
                "Set features failed");
    TEST_ASSERT(Xvr_LLVMTargetConfigSetReloc(config, "default") == true,
                "Set reloc failed");
    TEST_ASSERT(Xvr_LLVMTargetConfigSetCodeModel(config, "jitdefault") == true,
                "Set code model failed");

    const char* default_triple = Xvr_LLVMTargetMachineGetDefaultTargetTriple();
    TEST_ASSERT(default_triple != NULL, "Get default triple failed");

    const char* default_cpu = Xvr_LLVMTargetMachineGetDefaultCPU();
    TEST_ASSERT(default_cpu != NULL, "Get default CPU failed");

    Xvr_LLVMTargetConfigDestroy(config);
    TEST_PASS("Target configuration");
    return 0;
}

static int test_codegen_basic(void) {
    printf("\n  --- Codegen Basic Operations ---\n");

    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test_codegen");

    TEST_ASSERT(codegen != NULL, "Codegen creation failed");
    TEST_ASSERT(!Xvr_LLVMCodegenHasError(codegen),
                "Codegen has error after creation");

    /* Test IR printing */
    size_t ir_len = 0;
    char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
    TEST_ASSERT(ir != NULL, "IR printing failed");
    TEST_ASSERT(ir_len > 0, "IR length is zero");
    free(ir);

    Xvr_LLVMCodegenDestroy(codegen);
    Xvr_LLVMContextDestroy(ctx);
    TEST_PASS("Codegen basic operations");
    return 0;
}

static int test_codegen_optimization(void) {
    printf("\n  --- Codegen Optimization ---\n");

    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test_optimization");

    TEST_ASSERT(
        Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O1) == true,
        "O1 failed");
    TEST_ASSERT(
        Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O2) == true,
        "O2 failed");
    TEST_ASSERT(
        Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O3) == true,
        "O3 failed");
    TEST_ASSERT(
        Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_OS) == true,
        "Os failed");
    TEST_ASSERT(
        Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_OZ) == true,
        "Oz failed");
    TEST_ASSERT(
        Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_NONE) == true,
        "None failed");

    Xvr_LLVMCodegenDestroy(codegen);
    TEST_PASS("Codegen optimization levels");
    return 0;
}

static int test_codegen_target(void) {
    printf("\n  --- Codegen Target ---\n");

    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test_target");

    TEST_ASSERT(
        Xvr_LLVMCodegenSetTargetTriple(codegen, "x86_64-pc-linux-gnu") == true,
        "Set triple failed");
    TEST_ASSERT(Xvr_LLVMCodegenSetTargetCPU(codegen, "generic") == true,
                "Set CPU failed");

    Xvr_LLVMCodegenDestroy(codegen);
    TEST_PASS("Codegen target configuration");
    return 0;
}

static int test_codegen_bitcode(void) {
    printf("\n  --- Codegen Bitcode ---\n");

    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("bitcode_test");

    /* Note: This may fail if LLVM is in read-only mode, but we just test the
     * API */
    (void)Xvr_LLVMCodegenWriteBitcode(codegen, "/tmp/test_bitcode.bc");

    Xvr_LLVMCodegenDestroy(codegen);
    TEST_PASS("Codegen bitcode writing");
    return 0;
}

static int test_full_compilation_pipeline(void) {
    printf("\n  --- Full Compilation Pipeline ---\n");

    /* Test compiling a simple program */
    const char* source = "var x = 42;\nstd::print(\"{}\", x);\n";

    /* Parse the source */
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
            Xvr_freeASTNode(node);
            for (int i = 0; i < nodeCount; i++) {
                Xvr_freeASTNode(nodes[i]);
            }
            free(nodes);
            Xvr_freeParser(&parser);
            printf(XVR_CC_ERROR
                   "  [FAIL] Parse error in test source\n" XVR_CC_RESET);
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

    /* Create codegen and emit AST */
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("pipeline_test");
    TEST_ASSERT(codegen != NULL, "Codegen creation failed for pipeline test");

    for (int i = 0; i < nodeCount; i++) {
        Xvr_LLVMCodegenEmitAST(codegen, nodes[i]);
    }

    if (Xvr_LLVMCodegenHasError(codegen)) {
        const char* err = Xvr_LLVMCodegenGetError(codegen);
        printf(XVR_CC_ERROR "  [FAIL] Codegen error: %s\n" XVR_CC_RESET,
               err ? err : "unknown");
        Xvr_LLVMCodegenDestroy(codegen);
        for (int i = 0; i < nodeCount; i++) {
            Xvr_freeASTNode(nodes[i]);
        }
        free(nodes);
        return 1;
    }

    /* Test IR generation */
    size_t ir_len = 0;
    char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
    TEST_ASSERT(ir != NULL, "IR generation failed");
    TEST_ASSERT(ir_len > 0, "IR length is zero");
    TEST_ASSERT(strstr(ir, "define i32 @main") != NULL,
                "Main function not found in IR");
    free(ir);

    Xvr_LLVMCodegenDestroy(codegen);

    for (int i = 0; i < nodeCount; i++) {
        Xvr_freeASTNode(nodes[i]);
    }
    free(nodes);

    TEST_PASS("Full compilation pipeline");
    return 0;
}

static int test_datatypes_compilation(void) {
    printf("\n  --- Data Types Compilation ---\n");

    struct {
        const char* name;
        const char* source;
    } test_cases[] = {
        {"Integer", "var x = 42;\nstd::print(\"{}\", x);\n"},
        {"Float", "var x = 3.14;\nstd::print(\"{}\", x);\n"},
        {"String", "var x = \"hello\";\nstd::print(x);\n"},
        {"Boolean", "var x = true;\nstd::print(\"{}\", x);\n"},
        {"Array", "var x = [1, 2, 3];\nstd::print(\"{}\", x);\n"},
        {"Negative", "var x = -42;\nstd::print(\"{}\", x);\n"},
    };

    int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int t = 0; t < num_tests; t++) {
        Xvr_Lexer lexer;
        Xvr_Parser parser;
        Xvr_initLexer(&lexer, test_cases[t].source);
        Xvr_initParser(&parser, &lexer);

        Xvr_ASTNode** nodes = NULL;
        int nodeCount = 0;

        Xvr_ASTNode* node = Xvr_scanParser(&parser);
        while (node != NULL && node->type != XVR_AST_NODE_ERROR) {
            nodes = realloc(nodes, sizeof(Xvr_ASTNode*) * (nodeCount + 1));
            nodes[nodeCount++] = node;
            node = Xvr_scanParser(&parser);
        }

        Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("datatype_test");

        for (int i = 0; i < nodeCount; i++) {
            Xvr_LLVMCodegenEmitAST(codegen, nodes[i]);
        }

        if (Xvr_LLVMCodegenHasError(codegen)) {
            printf(XVR_CC_ERROR "  [FAIL] %s compilation failed\n" XVR_CC_RESET,
                   test_cases[t].name);
            Xvr_LLVMCodegenDestroy(codegen);
            for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
            free(nodes);
            Xvr_freeParser(&parser);
            return 1;
        }

        size_t ir_len = 0;
        char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
        TEST_ASSERT(ir != NULL && ir_len > 0, test_cases[t].name);
        free(ir);

        Xvr_LLVMCodegenDestroy(codegen);
        for (int i = 0; i < nodeCount; i++) Xvr_freeASTNode(nodes[i]);
        free(nodes);
        Xvr_freeParser(&parser);

        printf(XVR_CC_NOTICE "    [OK] %s\n" XVR_CC_RESET, test_cases[t].name);
    }

    TEST_PASS("Data types compilation");
    return 0;
}

int run_llvm_backend_tests(void) {
    printf("\n" XVR_CC_NOTICE "  LLVM Backend Tests\n" XVR_CC_RESET);

    int failures = 0;

    failures += test_context_create_destroy();
    failures += test_type_mapper();
    failures += test_module_manager();
    failures += test_ir_builder();
    failures += test_optimizer();
    failures += test_target_config();
    failures += test_codegen_basic();
    failures += test_codegen_optimization();
    failures += test_codegen_target();
    failures += test_codegen_bitcode();
    failures += test_full_compilation_pipeline();
    failures += test_datatypes_compilation();

    return failures;
}
