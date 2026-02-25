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
#include "../src/xvr_console_colors.h"
#include "../src/xvr_literal.h"

#define ASSERT(test_for_true)                                            \
    if (!(test_for_true)) {                                              \
        fprintf(stderr, XVR_CC_ERROR "ASSERT FAILED: %s\n" XVR_CC_RESET, \
                #test_for_true);                                         \
        exit(1);                                                         \
    }

#define ASSERT_MSG(test_for_true, msg)                                        \
    if (!(test_for_true)) {                                                   \
        fprintf(stderr, XVR_CC_ERROR "ASSERT FAILED: %s - %s\n" XVR_CC_RESET, \
                #test_for_true, msg);                                         \
        exit(1);                                                              \
    }

static void test_llvm_context_create_destroy(void) {
    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    ASSERT(ctx != NULL);
    ASSERT(!Xvr_LLVMContextHasError(ctx));

    LLVMContextRef llvm_ctx = Xvr_LLVMContextGetLLVMContext(ctx);
    ASSERT(llvm_ctx != NULL);

    Xvr_LLVMContextDestroy(ctx);
    printf(XVR_CC_NOTICE
           "PASS: test_llvm_context_create_destroy\n" XVR_CC_RESET);
}

static void test_llvm_type_mapper(void) {
    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    ASSERT(ctx != NULL);

    Xvr_LLVMTypeMapper* mapper = Xvr_LLVMTypeMapperCreate(ctx);
    ASSERT(mapper != NULL);

    LLVMTypeRef int_type =
        Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_INTEGER);
    ASSERT(int_type != NULL);

    LLVMTypeRef bool_type =
        Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_BOOLEAN);
    ASSERT(bool_type != NULL);

    LLVMTypeRef float_type =
        Xvr_LLVMTypeMapperGetType(mapper, XVR_LITERAL_FLOAT);
    ASSERT(float_type != NULL);

    LLVMTypeRef ptr_type =
        Xvr_LLVMTypeMapperGetPointerType(mapper, XVR_LITERAL_INTEGER);
    ASSERT(ptr_type != NULL);

    ASSERT(Xvr_LLVMTypeMapperIsSigned(XVR_LITERAL_INTEGER) == true);
    ASSERT(Xvr_LLVMTypeMapperIsSigned(XVR_LITERAL_BOOLEAN) == false);

    Xvr_LLVMTypeMapperDestroy(mapper);
    Xvr_LLVMContextDestroy(ctx);
    printf(XVR_CC_NOTICE "PASS: test_llvm_type_mapper\n" XVR_CC_RESET);
}

static void test_llvm_module_manager(void) {
    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    ASSERT(ctx != NULL);

    Xvr_LLVMModuleManager* mgr =
        Xvr_LLVMModuleManagerCreate(ctx, "test_module");
    ASSERT(mgr != NULL);

    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(mgr);
    ASSERT(module != NULL);

    size_t ir_len = 0;
    char* ir = Xvr_LLVMModuleManagerPrintIR(mgr, &ir_len);
    ASSERT(ir != NULL);
    ASSERT(ir_len > 0);
    free(ir);

    Xvr_LLVMModuleManagerDestroy(mgr);
    Xvr_LLVMContextDestroy(ctx);
    printf(XVR_CC_NOTICE "PASS: test_llvm_module_manager\n" XVR_CC_RESET);
}

static void test_llvm_ir_builder(void) {
    Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
    ASSERT(ctx != NULL);

    Xvr_LLVMModuleManager* mgr =
        Xvr_LLVMModuleManagerCreate(ctx, "test_module");
    ASSERT(mgr != NULL);

    LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(mgr);

    Xvr_LLVMIRBuilder* builder = Xvr_LLVMIRBuilderCreate(ctx, module);
    ASSERT(builder != NULL);

    LLVMBuilderRef llvm_builder = Xvr_LLVMIRBuilderGetLLVMBuilder(builder);
    ASSERT(llvm_builder != NULL);

    LLVMTypeRef fn_type = LLVMFunctionType(
        LLVMInt32TypeInContext(Xvr_LLVMContextGetLLVMContext(ctx)), NULL, 0,
        false);
    LLVMValueRef fn = LLVMAddFunction(module, "test_fn", fn_type);
    ASSERT(fn != NULL);

    LLVMBasicBlockRef block =
        Xvr_LLVMIRBuilderCreateBlockInFunction(builder, fn, "entry");
    ASSERT(block != NULL);

    Xvr_LLVMIRBuilderSetInsertPoint(builder, block);
    LLVMBasicBlockRef current = Xvr_LLVMIRBuilderGetInsertBlock(builder);
    ASSERT(current == block);

    Xvr_LLVMIRBuilderDestroy(builder);
    Xvr_LLVMModuleManagerDestroy(mgr);
    Xvr_LLVMContextDestroy(ctx);
    printf(XVR_CC_NOTICE "PASS: test_llvm_ir_builder\n" XVR_CC_RESET);
}

static void test_llvm_optimizer(void) {
    Xvr_LLVMOptimizer* opt = Xvr_LLVMOptimizerCreate();
    ASSERT(opt != NULL);

    ASSERT(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_O2) == true);
    ASSERT(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_O3) == true);
    ASSERT(Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_NONE) == true);

    ASSERT(Xvr_LLVMOptimizerAddStandardPasses(opt) == true);

    Xvr_LLVMOptimizerDestroy(opt);
    printf(XVR_CC_NOTICE "PASS: test_llvm_optimizer\n" XVR_CC_RESET);
}

static void test_llvm_target_config(void) {
    Xvr_LLVMTargetConfig* config = Xvr_LLVMTargetConfigCreate();
    ASSERT(config != NULL);

    ASSERT(Xvr_LLVMTargetConfigSetTriple(config, "x86_64-pc-linux-gnu") ==
           true);
    ASSERT(Xvr_LLVMTargetConfigSetCPU(config, "generic") == true);
    ASSERT(Xvr_LLVMTargetConfigSetFeatures(config, "+sse,+sse2") == true);
    ASSERT(Xvr_LLVMTargetConfigSetReloc(config, "default") == true);
    ASSERT(Xvr_LLVMTargetConfigSetCodeModel(config, "jitdefault") == true);

    const char* default_triple = Xvr_LLVMTargetMachineGetDefaultTargetTriple();
    ASSERT(default_triple != NULL);

    const char* default_cpu = Xvr_LLVMTargetMachineGetDefaultCPU();
    ASSERT(default_cpu != NULL);

    Xvr_LLVMTargetConfigDestroy(config);
    printf(XVR_CC_NOTICE "PASS: test_llvm_target_config\n" XVR_CC_RESET);
}

static void test_llvm_codegen_create_destroy(void) {
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test_codegen");
    ASSERT(codegen != NULL);
    ASSERT(!Xvr_LLVMCodegenHasError(codegen));

    Xvr_LLVMCodegenDestroy(codegen);
    printf(XVR_CC_NOTICE
           "PASS: test_llvm_codegen_create_destroy\n" XVR_CC_RESET);
}

static void test_llvm_codegen_optimization_levels(void) {
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test_codegen");
    ASSERT(codegen != NULL);

    ASSERT(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O1) ==
           true);
    ASSERT(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O2) ==
           true);
    ASSERT(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O3) ==
           true);
    ASSERT(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_OS) ==
           true);
    ASSERT(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_OZ) ==
           true);
    ASSERT(Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_NONE) ==
           true);

    Xvr_LLVMCodegenDestroy(codegen);
    printf(XVR_CC_NOTICE
           "PASS: test_llvm_codegen_optimization_levels\n" XVR_CC_RESET);
}

static void test_llvm_codegen_target(void) {
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("test_codegen");
    ASSERT(codegen != NULL);

    ASSERT(Xvr_LLVMCodegenSetTargetTriple(codegen, "x86_64-pc-linux-gnu") ==
           true);
    ASSERT(Xvr_LLVMCodegenSetTargetCPU(codegen, "generic") == true);

    Xvr_LLVMCodegenDestroy(codegen);
    printf(XVR_CC_NOTICE "PASS: test_llvm_codegen_target\n" XVR_CC_RESET);
}

static void test_llvm_codegen_print_ir(void) {
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("empty_module");
    ASSERT(codegen != NULL);

    size_t ir_len = 0;
    char* ir = Xvr_LLVMCodegenPrintIR(codegen, &ir_len);
    ASSERT(ir != NULL);
    ASSERT(ir_len > 0);

    ASSERT(strstr(ir, "empty_module") != NULL);

    free(ir);
    Xvr_LLVMCodegenDestroy(codegen);
    printf(XVR_CC_NOTICE "PASS: test_llvm_codegen_print_ir\n" XVR_CC_RESET);
}

static void test_llvm_codegen_bitcode(void) {
    Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("bitcode_test");
    ASSERT(codegen != NULL);

    ASSERT(Xvr_LLVMCodegenWriteBitcode(codegen, "/tmp/test_bitcode.bc") ==
           true);

    Xvr_LLVMCodegenDestroy(codegen);
    printf(XVR_CC_NOTICE "PASS: test_llvm_codegen_bitcode\n" XVR_CC_RESET);
}

int main(void) {
    printf("Testing Context...\n");
    test_llvm_context_create_destroy();

    printf("\nTesting Type Mapper...\n");
    test_llvm_type_mapper();

    printf("\nTesting Module Manager...\n");
    test_llvm_module_manager();

    printf("\nTesting IR Builder...\n");
    test_llvm_ir_builder();

    printf("\nTesting Optimizer...\n");
    test_llvm_optimizer();

    printf("\nTesting Target Configuration...\n");
    test_llvm_target_config();

    printf("\nTesting Codegen...\n");
    test_llvm_codegen_create_destroy();
    test_llvm_codegen_optimization_levels();
    test_llvm_codegen_target();
    test_llvm_codegen_print_ir();
    test_llvm_codegen_bitcode();
    return 0;
}
