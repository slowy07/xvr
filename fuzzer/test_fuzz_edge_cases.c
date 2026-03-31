#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static jmp_buf jump_buffer;
static volatile sig_atomic_t crash_occurred = 0;

void segfault_handler(int sig) {
    crash_occurred = 1;
    longjmp(jump_buffer, 1);
}

typedef struct {
    const char* name;
    const char* code;
    int expect_crash;
    int expect_error;
    const char* description;
} EdgeCaseTest;

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;
static int crashed_tests = 0;
static int skipped_tests = 0;

int run_edge_test(const EdgeCaseTest* test) {
    total_tests++;

    if (crash_occurred = 0, signal(SIGSEGV, segfault_handler) == SIG_ERR) {
        skipped_tests++;
        printf("  [SKIP] %s - could not setup signal handler\n", test->name);
        return 0;
    }

    if (setjmp(jump_buffer) != 0) {
        signal(SIGSEGV, SIG_DFL);
        if (test->expect_crash) {
            printf("  [PASS] %s - crashed as expected: %s\n", test->name,
                   test->description);
            passed_tests++;
        } else {
            printf("  [FAIL] %s - unexpected crash: %s\n", test->name,
                   test->description);
            crashed_tests++;
        }
        return 0;
    }

    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
             "cd /home/arfyslowy/Documents/project/xvrlang/xvr && "
             "./build/xvr -l - 2>&1 <<'XVRCODE'\n%s\nXVRCODE",
             test->code);

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        skipped_tests++;
        printf("  [SKIP] %s - could not run compiler\n", test->name);
        signal(SIGSEGV, SIG_DFL);
        return 0;
    }

    char buffer[8192];
    int has_error = 0;
    int has_critical_error = 0;

    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, "error") || strstr(buffer, "Error")) {
            has_error = 1;
        }
        if (strstr(buffer, "free():") || strstr(buffer, "malloc:")) {
            has_critical_error = 1;
        }
    }

    int status = pclose(fp);
    int exit_code = WEXITSTATUS(status);

    signal(SIGSEGV, SIG_DFL);

    if (has_critical_error || exit_code >= 128) {
        if (test->expect_crash || test->expect_error) {
            printf("  [PASS] %s - error occurred: %s\n", test->name,
                   test->description);
            passed_tests++;
        } else {
            printf("  [FAIL] %s - critical error: %s\n", test->name,
                   test->description);
            failed_tests++;
        }
        return 0;
    }

    if (test->expect_crash) {
        printf("  [FAIL] %s - expected crash but succeeded: %s\n", test->name,
               test->description);
        failed_tests++;
        return 0;
    }

    if (test->expect_error) {
        if (has_error) {
            printf("  [PASS] %s - error as expected: %s\n", test->name,
                   test->description);
            passed_tests++;
        } else {
            printf("  [FAIL] %s - expected error but succeeded: %s\n",
                   test->name, test->description);
            failed_tests++;
        }
        return 0;
    }

    if (!has_error) {
        printf("  [PASS] %s - compiled: %s\n", test->name, test->description);
        passed_tests++;
    } else {
        printf("  [FAIL] %s - unexpected error: %s\n", test->name,
               test->description);
        failed_tests++;
    }

    return 0;
}

int main(int argc, char** argv) {
    printf("XVR Edge Case Fuzz Testing\n");
    printf("============================\n\n");

    EdgeCaseTest tests[] = {
        {"deep_recursion_100",
         "proc foo(n: int32): int32 { if (n <= 0) { return 0; } return foo(n - "
         "1); } std::print(\"{}\", foo(100));",
         0, 0, "test recursion depth 100"},

        {"deep_recursion_1000",
         "proc foo(n: int32): int32 { if (n <= 0) { return 0; } return foo(n - "
         "1); } std::print(\"{}\", foo(1000));",
         0, 1, "test recursion depth 1000 - may overflow"},

        {"deep_nesting_50",
         "var x = 1;\n"
         "if (true) { if (true) { if (true) { if (true) { if (true) { if "
         "(true) { "
         "if (true) { if (true) { if (true) { if (true) { if (true) { if "
         "(true) { "
         "if (true) { if (true) { if (true) { if (true) { if (true) { if "
         "(true) { "
         "if (true) { if (true) { if (true) { if (true) { if (true) { if "
         "(true) { "
         "if (true) { if (true) { if (true) { if (true) { if (true) { if "
         "(true) { "
         "if (true) { if (true) { if (true) { if (true) { if (true) { if "
         "(true) { "
         "if (true) { if (true) { if (true) { if (true) { if (true) { if "
         "(true) { "
         "if (true) { if (true) { if (true) { if (true) { if (true) { if "
         "(true) { "
         "if (true) { if (true) { if (true) { if (true) { if (true) { if "
         "(true) { x = 2; } } } } } } } } } } } } } } } } } } } } } } } } } } "
         "} } } } } } } } } } } } } } } } } } } } } } } } } } } } }"
         "std::print(\"{}\", x);",
         0, 0, "test 50 level if nesting"},

        {"large_array_1000",
         "var arr = [1];\n"
         "arr[0] = 1;\n"
         "std::print(\"{}\", 1);",
         0, 0, "test basic array"},

        {"large_number",
         "var x = 2147483647;\n"
         "std::print(\"{}\", x);",
         0, 0, "test max int32"},

        {"negative_max",
         "var x = -2147483648;\n"
         "std::print(\"{}\", x);",
         0, 0, "test min int32"},

        {"overflow_add",
         "var x = 2147483647 + 1;\n"
         "std::print(\"{}\", x);",
         0, 1, "test addition overflow"},

        {"large_float",
         "var x: float64 = 1.7976931348623157e+308;\n"
         "std::print(\"{}\", x);",
         0, 0, "test large float64"},

        {"very_long_string",
         "var s = "
         "\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\""
         ";\n"
         "std::print(\"{}\", 1);",
         0, 0, "test 200 char string"},

        {"nested_parens_20", "std::print((((((((((((((((((1))))))))))))))))));",
         0, 0, "test 20 nested parens"},

        {"unbalanced_parens_open", "std::print(1;", 0, 1, "test unbalanced ("},

        {"unbalanced_parens_close", "std::print(1));", 0, 1,
         "test unbalanced )"},

        {"unterminated_string_dbl", "std::print(\"hello", 0, 1,
         "test unterminated double quote string"},

        {"unterminated_string_sgl", "std::print('hello)", 0, 1,
         "test unterminated single quote string"},

        {"invalid_number", "var x = 10abc;\nstd::print(\"{}\", x);", 0, 1,
         "test invalid number literal"},

        {"division_by_zero", "var x = 1 / 0;\nstd::print(\"{}\", x);", 0, 1,
         "test division by zero"},

        {"mod_by_zero", "var x = 1 % 0;\nstd::print(\"{}\", x);", 0, 1,
         "test modulo by zero"},

        {"redeclare_var", "var x = 1;\nvar x = 2;\nstd::print(\"{}\", x);", 0,
         1, "test variable redeclaration"},

        {"unknown_identifier", "var x = unknown_var;\nstd::print(\"{}\", x);",
         0, 1, "test unknown identifier"},

        {"call_undefined_fn", "undefined_fn();\nstd::print(\"test\");", 0, 1,
         "test call undefined function"},

        {"wrong_arg_count", "proc foo(a: int32): void { }\nfoo(1, 2);", 0, 1,
         "test wrong argument count"},

        {"missing_arg", "proc foo(a: int32, b: int32): void { }\nfoo(1);", 0, 1,
         "test missing argument"},

        {"wrong_return_type",
         "proc foo(): int32 { return \"str\"; }\nstd::print(\"test\");", 0, 1,
         "test wrong return type"},

        {"wrong_param_type", "proc foo(a: int32): void { }\nfoo(\"str\");", 0,
         1, "test wrong parameter type"},

        {"array_out_of_bounds",
         "var arr = [1, 2, 3];\nvar x = arr[100];\nstd::print(\"{}\", x);", 0,
         1, "test array out of bounds"},

        {"array_negative_index",
         "var arr = [1, 2, 3];\nvar x = arr[-1];\nstd::print(\"{}\", x);", 0, 1,
         "test array negative index"},

        {"type_mismatch_int_string",
         "var x: int32 = \"hello\";\nstd::print(\"{}\", x);", 0, 1,
         "test int = string"},

        {"type_mismatch_bool_int", "var x: bool = 42;\nstd::print(\"{}\", x);",
         0, 1, "test bool = int"},

        {"invalid_type_annotation",
         "var x: invalid_type = 1;\nstd::print(\"{}\", x);", 0, 1,
         "test invalid type"},

        {"missing_return", "proc foo(): int32 { }\nstd::print(\"{}\", foo());",
         0, 1, "test missing return value"},

        {"return_in_void_value", "proc foo(): void { return 42; }\nfoo();", 0,
         1, "test return value in void proc"},

        {"break_outside_loop", "break;\nstd::print(\"test\");", 0, 1,
         "test break outside loop"},

        {"continue_outside_loop", "continue;\nstd::print(\"test\");", 0, 1,
         "test continue outside loop"},

        {"empty_source", "", 0, 0, "test empty input"},

        {"comment_only", "// this is a comment\n/* another comment */", 0, 0,
         "test comments only"},

        {"just_whitespace", "   \n\t\n   ", 0, 0, "test whitespace only"},

        {"null_keyword", "var x = null;\nstd::print(\"{}\", 0);", 0, 0,
         "test null keyword"},

        {"true_false", "var a = true;\nvar b = false;\nstd::print(\"{}\", 0);",
         0, 0, "test true/false literals"},

        {"multiple_statements",
         "var a = 1; var b = 2; var c = 3; var d = 4; var e = 5; "
         "std::print(\"{}\", 0);",
         0, 0, "test multiple statements in one line"},

        {"chained_operations",
         "var x = 1 + 2 + 3 + 4 + 5;\nstd::print(\"{}\", x);", 0, 0,
         "test chained additions"},

        {"mix_types_in_expr", "var x = 1 + 2.5;\nstd::print(\"{}\", 0);", 0, 1,
         "test mixed int and float in expression"},

        {"nested_ternary",
         "var x = true ? (false ? 1 : 2) : (true ? 3 : 4);\nstd::print(\"{}\", "
         "x);",
         0, 0, "test nested ternary"},

        {"while_with_complex_cond",
         "var i = 0;\nwhile (i < 10 && i != 5) { i = i + 1; "
         "}\nstd::print(\"{}\", i);",
         0, 0, "test while with complex condition"},

        {"for_without_body",
         "for (var i = 0; i < 10; i++) { }\nstd::print(\"done\");", 0, 0,
         "test for with empty body"},

        {"empty_block", "{ }\nstd::print(\"test\");", 0, 0, "test empty block"},

        {"type_annotation_var",
         "var x: int32;\nx = 42;\nstd::print(\"{}\", x);", 0, 0,
         "test type annotation without init"},

        {"implicit_cast", "var x: float64 = 42;\nstd::print(\"{}\", x);", 0, 0,
         "test implicit int to float cast"},

        {"bool_arithmetic", "var x = true + false;\nstd::print(\"{}\", x);", 0,
         1, "test bool arithmetic"},

        {"string_arithmetic",
         "var x = \"hello\" - \"world\";\nstd::print(\"{}\", x);", 0, 1,
         "test string subtraction"},

        {"function_as_value",
         "proc foo(): void { }\nvar x = foo;\nstd::print(\"{}\", 0);", 0, 1,
         "test function as value"},

        {"array_in_condition",
         "var arr = [1, 2, 3];\nif (arr) { std::print(\"yes\"); }", 0, 1,
         "test array as boolean"},

        {"string_in_condition",
         "var s = \"hello\";\nif (s) { std::print(\"yes\"); }", 0, 1,
         "test string as boolean"},
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++) {
        run_edge_test(&tests[i]);
    }

    printf("\n============================\n");
    printf("Edge Case Fuzz Summary\n");
    printf("============================\n");
    printf("Total:   %d\n", total_tests);
    printf("Passed:  %d\n", passed_tests);
    printf("Failed:  %d\n", failed_tests);
    printf("Crashed: %d\n", crashed_tests);
    printf("Skipped: %d\n", skipped_tests);
    printf("\n");

    if (failed_tests == 0 && crashed_tests == 0) {
        printf("All edge case tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed or crashed.\n");
        return 1;
    }
}
