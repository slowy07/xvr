/**
 * XVR Combined Test Suite - LLVM AOT Only
 * Runs all test modules in a single executable with verbose output
 */

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "xvr_console_colors.h"

static jmp_buf jump_buffer;
static volatile sig_atomic_t crashOccurred = 0;
static int totalPassed = 0;
static int totalFailed = 0;

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
int run_ast_node_tests(void);
int run_llvm_backend_tests(void);
int run_std_tests(void);

void print_header(const char* title) {
    printf("\n" XVR_CC_NOTICE "  %s\n" XVR_CC_RESET, title);
}

void print_test_start(const char* test_name) {
    printf(XVR_CC_NOTICE "  [RUN ] %s...\n" XVR_CC_RESET, test_name);
}

void print_test_pass(const char* test_name) {
    printf(XVR_CC_NOTICE "  [PASS] %s\n" XVR_CC_RESET, test_name);
    totalPassed++;
}

void print_test_fail(const char* test_name, const char* reason) {
    printf(XVR_CC_ERROR "  [FAIL] %s - %s\n" XVR_CC_RESET, test_name,
           reason ? reason : "unknown error");
    totalFailed++;
}

int run_test_with_crash_protection(const char* name, int (*test_func)(void),
                                   const char* description) {
    crashOccurred = 0;
    signal(SIGSEGV, segfault_handler);

    printf(XVR_CC_NOTICE "\n[RUN ] %s\n" XVR_CC_RESET, description);

    int result;
    if (setjmp(jump_buffer) == 0) {
        result = test_func();
        if (result == 0) {
            print_test_pass(name);
        } else {
            print_test_fail(name, "test returned non-zero");
        }
    } else {
        print_test_fail(name, "SEGFAULT");
        result = -1;
    }

    signal(SIGSEGV, SIG_DFL);
    return result;
}

int main(void) {
    print_header("XVR LLVM AOT COMPILER TEST SUITE");
    printf("\nTesting Version: 0.5.9\n");
    printf("Build: " __DATE__ " " __TIME__ "\n\n");

    /* AST Node Tests */
    print_header("AST Node Tests");
    run_test_with_crash_protection("ast_node_create", run_ast_node_tests,
                                   "AST Node Creation and Management");

    /* Lexer Tests */
    print_header("Lexer Tests");
    run_test_with_crash_protection("lexer_basic", run_lexer_tests,
                                   "Lexer Tokenization");

    /* Parser Tests */
    print_header("Parser Tests");
    run_test_with_crash_protection("parser_basic", run_parser_tests,
                                   "Parser AST Generation");

    /* Scope Tests */
    print_header("Scope Tests");
    run_test_with_crash_protection("scope_basic", run_scope_tests,
                                   "Scope Management");

    /* Literal Tests */
    print_header("Literal Tests");
    run_test_with_crash_protection("literal_basic", run_literal_tests,
                                   "Literal Values");

    /* Memory Tests */
    print_header("Memory Tests");
    run_test_with_crash_protection("memory_basic", run_memory_tests,
                                   "Memory Management");

    /* Compiler Tests */
    print_header("Compiler Tests");
    run_test_with_crash_protection("compiler_basic", run_compiler_tests,
                                   "Code Compilation");

    /* LLVM Backend Tests */
    print_header("LLVM Backend Tests");
    run_test_with_crash_protection("llvm_backend", run_llvm_backend_tests,
                                   "LLVM Backend Infrastructure");

    /* Standard Library Tests */
    print_header("Standard Library Tests");
    run_test_with_crash_protection("std_lib", run_std_tests,
                                   "Standard Library Functions");

    /* Summary */
    print_header("TEST SUMMARY");
    printf("\n");
    printf(XVR_CC_NOTICE "  Total Passed: %d\n" XVR_CC_RESET, totalPassed);
    printf(XVR_CC_ERROR "  Total Failed: %d\n" XVR_CC_RESET, totalFailed);
    printf("\n");

    if (totalFailed == 0) {
        printf(XVR_CC_NOTICE "  ALL TESTS PASSED!\n\n" XVR_CC_RESET);
        return 0;
    } else {
        printf(XVR_CC_ERROR "  SOME TESTS FAILED!\n\n" XVR_CC_RESET);
        return 1;
    }
}
