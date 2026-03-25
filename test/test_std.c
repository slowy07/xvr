/**
 * XVR Standard Library Test Suite
 * Tests for std::max, std::print and other stdlib functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/backend/xvr_llvm_codegen.h"
#include "../src/xvr_ast_node.h"
#include "../src/xvr_console_colors.h"
#include "../src/xvr_lexer.h"
#include "../src/xvr_memory.h"
#include "../src/xvr_parser.h"

typedef struct {
    const char* name;
    const char* source;
    const char* expected_output;
    int should_pass;
} StdTestCase;

typedef struct {
    const char* name;
    int (*run)(void);
} StdTestEntry;

static void normalize_output(char* output) {
    char* marker = strstr(output, "Compiled to:");
    if (marker) {
        *marker = '\0';
    }
    size_t len = strlen(output);
    while (len > 0 && (output[len - 1] == '\n' || output[len - 1] == '\r')) {
        output[--len] = '\0';
    }
}

static int capture_compile_run(const char* source, char* output,
                               size_t output_size) {
    FILE* tmp = fopen("/tmp/xvr_std_test.xvr", "w");
    if (!tmp) return -1;
    fputs(source, tmp);
    fclose(tmp);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./out/xvr /tmp/xvr_std_test.xvr 2>&1");

    FILE* proc = popen(cmd, "r");
    if (!proc) {
        unlink("/tmp/xvr_std_test.xvr");
        return -1;
    }

    size_t len = fread(output, 1, output_size - 1, proc);
    output[len] = '\0';
    int status = pclose(proc);

    unlink("/tmp/xvr_std_test.xvr");

    char* compiled_marker = strstr(output, "Compiled to:");
    if (compiled_marker) {
        *compiled_marker = '\0';
    }
    size_t out_len = strlen(output);
    while (out_len > 0 &&
           (output[out_len - 1] == '\n' || output[out_len - 1] == '\r')) {
        output[--out_len] = '\0';
    }

    return status;
}

static int run_max_tests(void) {
    printf("\n" XVR_CC_NOTICE "  std::max Tests\n" XVR_CC_RESET);

    int passed = 0;
    int failed = 0;

    StdTestCase tests[] = {
        {"max_single_arg",
         "include std;\nvar r = std::max(42);\nstd::print(\"{}\\n\", r);", "42",
         1},
        {"max_int_greater",
         "include std;\nvar r = std::max(10, 20);\nstd::print(\"{}\\n\", r);",
         "20", 1},
        {"max_int_less",
         "include std;\nvar r = std::max(30, 20);\nstd::print(\"{}\\n\", r);",
         "30", 1},
        {"max_int_equal",
         "include std;\nvar r = std::max(5, 5);\nstd::print(\"{}\\n\", r);",
         "5", 1},
        {"max_int_negative",
         "include std;\nvar r = std::max(-10, 5);\nstd::print(\"{}\\n\", r);",
         "5", 1},
        {"max_int_both_negative",
         "include std;\nvar r = std::max(-5, -20);\nstd::print(\"{}\\n\", r);",
         "-5", 1},
        {"max_int_zero",
         "include std;\nvar r = std::max(0, 0);\nstd::print(\"{}\\n\", r);",
         "0", 1},
        {"max_int_three_args",
         "include std;\nvar r = std::max(3, 1, 2);\nstd::print(\"{}\\n\", r);",
         "3", 1},
        {"max_int_many_args",
         "include std;\nvar r = std::max(10, 20, 30, 5, "
         "15);\nstd::print(\"{}\\n\", r);",
         "30", 1},
        {"max_float_greater",
         "include std;\nvar r = std::max(1.5, 2.5);\nstd::print(\"{}\\n\", r);",
         "2.500000", 1},
        {"max_float_less",
         "include std;\nvar r = std::max(3.0, 1.0);\nstd::print(\"{}\\n\", r);",
         "3.000000", 1},
        {"max_float_equal",
         "include std;\nvar r = std::max(1.0, 1.0);\nstd::print(\"{}\\n\", r);",
         "1.000000", 1},
        {"max_float_negative",
         "include std;\nvar r = std::max(-1.5, 0.5);\nstd::print(\"{}\\n\", "
         "r);",
         "0.500000", 1},
        {"max_float_three_args",
         "include std;\nvar r = std::max(1.5, 2.5, "
         "0.5);\nstd::print(\"{}\\n\", r);",
         "2.500000", 1},
        {"max_proc_two_args", "include std;\nstd::max(10, 20);", "", 1},
        {"max_proc_three_args", "include std;\nstd::max(3, 1, 2);", "", 1},
        {"max_proc_float", "include std;\nstd::max(1.5, 2.5);", "", 1},
        {"max_proc_many_args", "include std;\nstd::max(10, 20, 30, 5, 15);", "",
         1},
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < test_count; i++) {
        char output[4096] = {0};
        int status =
            capture_compile_run(tests[i].source, output, sizeof(output));

        int test_passed = 0;
        if (status == 0) {
            if (tests[i].expected_output[0] == '\0') {
                test_passed = 1;
            } else if (strcmp(output, tests[i].expected_output) == 0) {
                test_passed = 1;
            }
        }

        if (test_passed) {
            printf(XVR_CC_NOTICE "    [PASS] %s\n" XVR_CC_RESET, tests[i].name);
            passed++;
        } else {
            printf(XVR_CC_ERROR "    [FAIL] %s\n" XVR_CC_RESET, tests[i].name);
            printf("          expected: '%s', got: '%s'\n",
                   tests[i].expected_output, output);
            failed++;
        }
    }

    printf("\n  std::max: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}

static int run_print_tests(void) {
    printf("\n" XVR_CC_NOTICE "  std::print Tests\n" XVR_CC_RESET);

    int passed = 0;
    int failed = 0;

    StdTestCase tests[] = {
        {"print_int", "include std;\nstd::print(\"{}\\n\", 42);", "42", 1},
        {"print_negative_int", "include std;\nstd::print(\"{}\\n\", -123);",
         "-123", 1},
        {"print_float", "include std;\nstd::print(\"{}\\n\", 3.14);",
         "3.140000", 1},
        {"print_string", "include std;\nstd::print(\"{}\\n\", \"hello\");",
         "hello", 1},
        {"print_multiple_args",
         "include std;\nstd::print(\"a: {} b: {}\\n\", 1, 2);", "a: 1 b: 2", 1},
        {"print_no_args", "include std;\nstd::print(\"hello world\\n\");",
         "hello world", 1},
        {"print_zero", "include std;\nstd::print(\"{}\\n\", 0);", "0", 1},
        {"print_large_number", "include std;\nstd::print(\"{}\\n\", 999999);",
         "999999", 1},
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < test_count; i++) {
        char output[4096] = {0};
        int status =
            capture_compile_run(tests[i].source, output, sizeof(output));

        if (status == 0 && strcmp(output, tests[i].expected_output) == 0) {
            printf(XVR_CC_NOTICE "    [PASS] %s\n" XVR_CC_RESET, tests[i].name);
            passed++;
        } else {
            printf(XVR_CC_ERROR "    [FAIL] %s\n" XVR_CC_RESET, tests[i].name);
            printf("          expected: '%s', got: '%s'\n",
                   tests[i].expected_output, output);
            failed++;
        }
    }

    printf("\n  std::print: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}

static int run_format_string_tests(void) {
    printf("\n" XVR_CC_NOTICE "  Format String Tests\n" XVR_CC_RESET);

    int passed = 0;
    int failed = 0;

    StdTestCase tests[] = {
        {"format_mixed_int_float",
         "include std;\nstd::print(\"int: {} float: {}\\n\", 42, 3.14);",
         "int: 42 float: 3.140000", 1},
        {"format_many_args",
         "include std;\nstd::print(\"{} {} {} {}\\n\", 1, 2, 3, 4);", "1 2 3 4",
         1},
        {"format_empty_placeholder",
         "include std;\nstd::print(\"value: {}\\n\", 0);", "value: 0", 1},
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < test_count; i++) {
        char output[4096] = {0};
        int status =
            capture_compile_run(tests[i].source, output, sizeof(output));

        if (status == 0 && strcmp(output, tests[i].expected_output) == 0) {
            printf(XVR_CC_NOTICE "    [PASS] %s\n" XVR_CC_RESET, tests[i].name);
            passed++;
        } else {
            printf(XVR_CC_ERROR "    [FAIL] %s\n" XVR_CC_RESET, tests[i].name);
            printf("          expected: '%s', got: '%s'\n",
                   tests[i].expected_output, output);
            failed++;
        }
    }

    printf("\n  Format strings: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}

int run_std_tests(void) {
    printf("\n" XVR_CC_NOTICE
           "===============================================\n" XVR_CC_RESET);
    printf(XVR_CC_NOTICE "  XVR Standard Library Test Suite\n" XVR_CC_RESET);
    printf(XVR_CC_NOTICE
           "===============================================\n" XVR_CC_RESET);

    int total_failed = 0;

    total_failed += run_max_tests();
    total_failed += run_print_tests();
    total_failed += run_format_string_tests();

    return total_failed;
}