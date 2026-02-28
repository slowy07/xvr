/**
 * XVR Combined Test Suite
 * Runs all test modules in a single executable
 */

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "xvr_console_colors.h"

static jmp_buf jump_buffer;
static volatile sig_atomic_t crashOccurred = 0;

static void segfault_handler(int sig) {
    crashOccurred = 1;
    longjmp(jump_buffer, 1);
}

int run_lexer_tests(void);
int run_parser_tests(void);
int run_scope_tests(void);
int run_compiler_tests(void);
int run_literal_tests(void);
int run_memory_tests(void);
int run_opaque_tests(void);
int run_ast_node_tests(void);
int run_llvm_backend_tests(void);
int run_interpreter_tests(void);

int run_test_with_crash_protection(const char* name, int (*test_func)(void)) {
    crashOccurred = 0;
    signal(SIGSEGV, segfault_handler);

    int result;
    if (setjmp(jump_buffer) == 0) {
        result = test_func();
    } else {
        printf(XVR_CC_ERROR "CRASHED: %s\n" XVR_CC_RESET, name);
        return -1;
    }

    signal(SIGSEGV, SIG_DFL);
    return result;
}

int main(void) {
    int totalFailed = 0;
    int totalPassed = 0;

    printf(XVR_CC_NOTICE "   XVR TEST SUITE\n");

    printf(XVR_CC_NOTICE "Running AST Node tests...\n");
    if (run_test_with_crash_protection("ast_node", run_ast_node_tests) != 0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: ast_node tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: ast_node tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "Running Lexer tests...\n");
    if (run_test_with_crash_protection("lexer", run_lexer_tests) != 0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: lexer tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: lexer tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "Running Parser tests...\n");
    if (run_test_with_crash_protection("parser", run_parser_tests) != 0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: parser tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: parser tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "Running Compiler tests...\n");
    if (run_test_with_crash_protection("compiler", run_compiler_tests) != 0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: compiler tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: compiler tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "Running Literal tests...\n");
    if (run_test_with_crash_protection("literal", run_literal_tests) != 0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: literal tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: literal tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "Running Memory tests...\n");
    if (run_test_with_crash_protection("memory", run_memory_tests) != 0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: memory tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: memory tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "Running Opaque tests...\n");
    if (run_test_with_crash_protection("opaque", run_opaque_tests) != 0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: opaque tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: opaque tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "Running Scope tests...\n");
    if (run_test_with_crash_protection("scope", run_scope_tests) != 0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: scope tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: scope tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "Running LLVM Backend tests...\n");
    if (run_test_with_crash_protection("llvm_backend",
                                       run_llvm_backend_tests) != 0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: llvm_backend tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: llvm_backend tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "Running Interpreter tests...\n");
    if (run_test_with_crash_protection("interpreter", run_interpreter_tests) !=
        0) {
        totalFailed++;
        printf(XVR_CC_ERROR "FAILED: interpreter tests\n" XVR_CC_RESET);
    } else {
        totalPassed++;
        printf(XVR_CC_NOTICE "PASSED: interpreter tests\n" XVR_CC_RESET);
    }
    printf("\n");

    printf(XVR_CC_NOTICE "   SUMMARY: %d passed, %d failed\n", totalPassed,
           totalFailed);
    return totalFailed > 0 ? 1 : 0;
}
