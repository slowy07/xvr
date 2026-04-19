#include <catch2/catch_test_macros.hpp>
#include "xvr_lexer.h"
#include "xvr_parser.h"
#include "xvr_ast_node.h"
#include "adapters/llvm/xvr_llvm_codegen.h"
#include "adapters/llvm/xvr_llvm_context.h"
#include "adapters/llvm/xvr_llvm_expression_emitter.h"
#include "adapters/llvm/xvr_llvm_function_emitter.h"
#include "adapters/llvm/xvr_llvm_ir_builder.h"
#include "adapters/llvm/xvr_llvm_module_manager.h"
#include "adapters/llvm/xvr_llvm_optimizer.h"
#include "adapters/llvm/xvr_llvm_target.h"
#include "adapters/llvm/xvr_llvm_type_mapper.h"

static void compileAndVerify(const char* source) {
    Xvr_Lexer lexer;
    Xvr_Parser parser;
    Xvr_initLexer(&lexer, source);
    Xvr_initParser(&parser, &lexer);

    Xvr_ASTNode** nodes = nullptr;
    int nodeCount = 0;

    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    while (node != nullptr && node->type != XVR_AST_NODE_ERROR) {
        nodes = reinterpret_cast<Xvr_ASTNode**>(realloc(nodes, sizeof(Xvr_ASTNode*) * (nodeCount + 1)));
        nodes[nodeCount++] = node;
        node = Xvr_scanParser(&parser);
    }

    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("datatype_test");

    for (int i = 0; i < nodeCount; i++) {
        Xvr_LLVMCodegenEmitAST(codegen, nodes[i]);
    }

    REQUIRE_FALSE(Xvr_LLVMCodegenHasError(codegen));

    size_t ir_len = 0;
    char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
    REQUIRE(ir != nullptr);
    REQUIRE(ir_len > 0);

    free(ir);
    Xvr_LLVMCodegenDestroy(codegen);

    for (int i = 0; i < nodeCount; i++) {
        Xvr_freeASTNode(nodes[i]);
    }
    free(nodes);
    Xvr_freeParser(&parser);
}

TEST_CASE("Context create/destroy", "[llvm_backend][llvm]") {
    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    REQUIRE(ctx != nullptr);
    REQUIRE_FALSE(Xvr_LLVMContextHasError(ctx));

    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(ctx);
    REQUIRE(llvm_ctx != nullptr);

    Xvr_LLVMContextDestroy(ctx);
}

TEST_CASE("Type mapper", "[llvm_backend][llvm]") {
    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    Xvr_LLVMTypeMapper* mapper = Xvr_LLVMTypeMapperCreate(ctx);

    REQUIRE(mapper != nullptr);

    LLVMTypeRef int_type = Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_INTEGER);
    REQUIRE(int_type != nullptr);

    LLVMTypeRef bool_type = Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_BOOLEAN);
    REQUIRE(bool_type != nullptr);

    LLVMTypeRef float_type = Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_FLOAT);
    REQUIRE(float_type != nullptr);

    LLVMTypeRef double_type = Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_FLOAT64);
    REQUIRE(double_type != nullptr);

    LLVMTypeRef ptr_type = Xvr_LLVMTypeMapperGetPointerType(mapper, XVR_LITERAL_INTEGER);
    REQUIRE(ptr_type != nullptr);

    REQUIRE(Xvr_LLVMTypeMapperIsSigned(XVR_LITERAL_INTEGER) == true);
    REQUIRE(Xvr_LLVMTypeMapperIsSigned(XVR_LITERAL_BOOLEAN) == false);

    Xvr_LLVMTypeMapperDestroy(mapper);
    Xvr_LLVMContextDestroy(ctx);
}

TEST_CASE("Module manager", "[llvm_backend][llvm]") {
    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    Xvr_LLVMModuleManager* mgr = Xvr_LLVMModuleManagerCreate(ctx, "test_module");

    REQUIRE(mgr != nullptr);

    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(mgr);
    REQUIRE(module != nullptr);

    size_t ir_len = 0;
    char* ir = Xvr_LLVMModuleManagerPrintIR(mgr, &ir_len);
    REQUIRE(ir != nullptr);
    REQUIRE(ir_len > 0);

    free(ir);
    Xvr_LLVMModuleManagerDestroy(mgr);
    Xvr_LLVMContextDestroy(ctx);
}

TEST_CASE("IR builder", "[llvm_backend][llvm]") {
    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    Xvr_LLVMModuleManager* mgr = Xvr_LLVMModuleManagerCreate(ctx, "test_module");
    Xvr_LLVMIRBuilder* builder = Xvr_LLVMIRBuilderCreate(ctx, Xvr_LLVMModuleManagerGetModule(mgr));

    REQUIRE(builder != nullptr);

    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
    REQUIRE(llvm_builder != nullptr);

    LLVMTypeRef fn_type = LLVMFunctionType(
        LLVMInt32TypeInContext(Xvr_LLVMContextGetLLVMContext(ctx)), nullptr, 0, false);
    LLVMValueRef fn = LLVMAddFunction(Xvr_LLVMModuleManagerGetModule(mgr), "test_fn", fn_type);
    REQUIRE(fn != nullptr);

    LLVMBasicBlockRef block = Xvr_LLVMIRBuilderCreateBlockInFunction(builder, fn, "entry");
    REQUIRE(block != nullptr);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, block);
    LLVMBasicBlockRef current = Xvr_LLVMIRBuilderGetInsertBlock(builder);
    REQUIRE(current == block);

    Xvr_LLVMIRBuilderDestroy(builder);
    Xvr_LLVMModuleManagerDestroy(mgr);
    Xvr_LLVMContextDestroy(ctx);
}

TEST_CASE("Optimizer", "[llvm_backend][llvm]") {
    Xvr_LLVMOptimizer* opt = Xvr_LLVMOptimizerCreate();
    REQUIRE(opt != nullptr);

    REQUIRE(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_NONE) == true);
    REQUIRE(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_O1) == true);
    REQUIRE(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_O2) == true);
    REQUIRE(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_O3) == true);
    REQUIRE(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_OS) == true);
    REQUIRE(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_OZ) == true);
    REQUIRE(Xvr_LLVMOptimizerAddStandardPasses(opt) == true);

    Xvr_LLVMOptimizerDestroy(opt);
}

TEST_CASE("Target configuration", "[llvm_backend][llvm]") {
    Xvr_LLVMTargetConfig* config = Xvr_LLVMTargetConfigCreate();
    REQUIRE(config != nullptr);

    REQUIRE(Xvr_LLVMTargetConfigSetTriple(config, "x86_64-pc-linux-gnu") == true);
    REQUIRE(Xvr_LLVMTargetConfigSetCPU(config, "generic") == true);
    REQUIRE(Xvr_LLVMTargetConfigSetFeatures(config, "+sse,+sse2") == true);
    REQUIRE(Xvr_LLVMTargetConfigSetReloc(config, "default") == true);
    REQUIRE(Xvr_LLVMTargetConfigSetCodeModel(config, "jitdefault") == true);

    const char* default_triple = Xvr_LLVMTargetMachineGetDefaultTargetTriple();
    REQUIRE(default_triple != nullptr);

    const char* default_cpu = Xvr_LLVMTargetMachineGetDefaultCPU();
    REQUIRE(default_cpu != nullptr);

    Xvr_LLVMTargetConfigDestroy(config);
}

TEST_CASE("Codegen basic operations", "[llvm_backend][llvm]") {
    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test_codegen");

    REQUIRE(codegen != nullptr);
    REQUIRE_FALSE(Xvr_LLVMCodegenHasError(codegen));

    size_t ir_len = 0;
    char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
    REQUIRE(ir != nullptr);
    REQUIRE(ir_len > 0);
    free(ir);

    Xvr_LLVMCodegenDestroy(codegen);
    Xvr_LLVMContextDestroy(ctx);
}

TEST_CASE("Codegen optimization levels", "[llvm_backend][llvm]") {
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test_optimization");

    REQUIRE(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O1) == true);
    REQUIRE(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O2) == true);
    REQUIRE(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O3) == true);
    REQUIRE(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_OS) == true);
    REQUIRE(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_OZ) == true);
    REQUIRE(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_NONE) == true);

    Xvr_LLVMCodegenDestroy(codegen);
}

TEST_CASE("Codegen target configuration", "[llvm_backend][llvm]") {
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test_target");

    REQUIRE(Xvr_LLVMCodegenSetTargetTriple(codegen, "x86_64-pc-linux-gnu") == true);
    REQUIRE(Xvr_LLVMCodegenSetTargetCPU(codegen, "generic") == true);

    Xvr_LLVMCodegenDestroy(codegen);
}

TEST_CASE("Full compilation pipeline", "[llvm_backend][llvm]") {
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
            Xvr_freeASTNode(node);
            for (int i = 0; i < nodeCount; i++) {
                Xvr_freeASTNode(nodes[i]);
            }
            free(nodes);
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

    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("pipeline_test");
    REQUIRE(codegen != nullptr);

    for (int i = 0; i < nodeCount; i++) {
        Xvr_LLVMCodegenEmitAST(codegen, nodes[i]);
    }

    REQUIRE_FALSE(Xvr_LLVMCodegenHasError(codegen));

    size_t ir_len = 0;
    char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
    REQUIRE(ir != nullptr);
    REQUIRE(ir_len > 0);
    REQUIRE(strstr(ir, "define i32 @main") != nullptr);
    free(ir);

    Xvr_LLVMCodegenDestroy(codegen);

    for (int i = 0; i < nodeCount; i++) {
        Xvr_freeASTNode(nodes[i]);
    }
    free(nodes);
}

TEST_CASE("Data types compilation - Integer", "[llvm_backend][llvm]") {
    compileAndVerify("var x = 42;\n");
}

TEST_CASE("Data types compilation - Float", "[llvm_backend][llvm]") {
    compileAndVerify("var x = 3.14;\n");
}

TEST_CASE("Data types compilation - String", "[llvm_backend][llvm]") {
    compileAndVerify("var x = \"hello\";\n");
}

TEST_CASE("Data types compilation - Boolean", "[llvm_backend][llvm]") {
    compileAndVerify("var x = true;\n");
}

TEST_CASE("Data types compilation - Negative", "[llvm_backend][llvm]") {
    compileAndVerify("var x = -42;\n");
}
