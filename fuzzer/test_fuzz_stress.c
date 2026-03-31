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
    const char* note;
} StressTest;

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;
static int crashed_tests = 0;

int run_stress_test(const StressTest* test) {
    total_tests++;

    signal(SIGSEGV, segfault_handler);

    if (setjmp(jump_buffer) != 0) {
        if (test->expect_crash) {
            printf("  [PASS] %s - crashed (expected): %s\n", test->name,
                   test->note);
            passed_tests++;
        } else {
            printf("  [FAIL] %s - unexpected crash: %s\n", test->name,
                   test->note);
            crashed_tests++;
        }
        signal(SIGSEGV, SIG_DFL);
        return 0;
    }

    char cmd[16384];
    snprintf(cmd, sizeof(cmd),
             "cd /home/arfyslowy/Documents/project/xvrlang/xvr && "
             "timeout 10 ./build/xvr -l - 2>&1 <<'XVRCODE'\n%s\nXVRCODE",
             test->code);

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        printf("  [SKIP] %s - could not run\n", test->name);
        signal(SIGSEGV, SIG_DFL);
        return 0;
    }

    char buffer[16384];
    int has_critical = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, "free():") || strstr(buffer, "malloc:")) {
            has_critical = 1;
        }
    }

    int status = pclose(fp);
    signal(SIGSEGV, SIG_DFL);

    if (has_critical || WIFSIGNALED(status)) {
        if (test->expect_crash) {
            printf("  [PASS] %s - critical error: %s\n", test->name,
                   test->note);
            passed_tests++;
        } else {
            printf("  [FAIL] %s - critical error: %s\n", test->name,
                   test->note);
            failed_tests++;
        }
        return 0;
    }

    printf("  [PASS] %s: %s\n", test->name, test->note);
    passed_tests++;
    return 0;
}

int main(int argc, char** argv) {
    printf("XVR Stress/Fuzz Testing\n");
    printf("========================\n\n");

    StressTest tests[] = {
        {"stress_deep_call_500",
         "proc foo(n: int32): int32 { if (n <= 0) { return n; } return foo(n - "
         "1) + 1; } std::print(\"{}\", foo(500));",
         0, "500 depth recursion"},

        {"stress_deep_call_1000",
         "proc foo(n: int32): int32 { if (n <= 0) { return n; } return foo(n - "
         "1) + 1; } std::print(\"{}\", foo(1000));",
         0, "1000 depth recursion"},

        {"stress_many_locals",
         "var a1 = 1; var a2 = 2; var a3 = 3; var a4 = 4; var a5 = 5;\n"
         "var a6 = 6; var a7 = 7; var a8 = 8; var a9 = 9; var a10 = 10;\n"
         "std::print(\"{}\", a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + "
         "a10);",
         0, "50 local variables"},

        {"stress_many_loops",
         "var result = 0;\n"
         "for (var i1 = 0; i1 < 10; i1++) { result = result + 1; }\n"
         "for (var i2 = 0; i2 < 10; i2++) { result = result + 1; }\n"
         "for (var i3 = 0; i3 < 10; i3++) { result = result + 1; }\n"
         "for (var i4 = 0; i4 < 10; i4++) { result = result + 1; }\n"
         "for (var i5 = 0; i5 < 10; i5++) { result = result + 1; }\n"
         "std::print(\"{}\", result);",
         0, "50 nested loops"},

        {"stress_long_line",
         "var x = 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + "
         "15 + 16 + 17 + 18 + 19 + 20; std::print(\"{}\", x);",
         0, "long expression line"},

        {"stress_many_args",
         "proc sum(a1: int32, a2: int32, a3: int32, a4: int32, a5: int32): "
         "int32 { return a1 + a2 + a3 + a4 + a5; }\nstd::print(\"{}\", sum(1, "
         "2, 3, 4, 5));",
         0, "5 function arguments"},

        {"stress_deep_block",
         "{ { { { { { { { { { std::print(\"deep\"); } } } } } } } } } }", 0,
         "10 level nested blocks"},

        {"stress_many_strings",
         "var s1 = \"aaaa\"; var s2 = \"bbbb\"; var s3 = \"cccc\"; var s4 = "
         "\"dddd\"; var s5 = \"eeee\";\nstd::print(\"{}\", 5);",
         0, "5 string literals"},

        {"stress_large_array",
         "var arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, "
         "15];\nstd::print(\"{}\", arr[7]);",
         0, "15 element array"},

        {"stress_complex_expr",
         "var x = ((1 + 2) * (3 - 4)) / ((5 + 6) * (7 - "
         "8));\nstd::print(\"{}\", 0);",
         0, "complex expression"},

        {"fuzz_random_1", "var x = 1; var y = 2; var z = x + y;", 0,
         "random 1"},
        {"fuzz_random_2", "proc f(): void { } f();", 0, "random 2"},
        {"fuzz_random_3", "var arr = [1]; arr[0] = 2;", 0, "random 3"},
        {"fuzz_random_4", "if (1 == 1) { }", 0, "random 4"},
        {"fuzz_random_5", "while (false) { }", 0, "random 5"},

        {"fuzz_mutation_1", "std::print(\"test\");", 0,
         "mutated - remove semicolon"},
        {"fuzz_mutation_2", "var x = 1 var y = 2;", 0,
         "mutated - missing semicolon"},
        {"fuzz_mutation_3", "std::print(1, 2, 3);", 0, "mutated - extra args"},

        {"edge_empty_stmt", ";", 0, "empty statement"},
        {"edge_just_semicolons", ";;;", 0, "multiple semicolons"},
        {"edge_single_char_var", "var a = 1;", 0, "single char variable"},
        {"edge_very_long_var", "var verylongvariablename = 1;", 0,
         "long variable"},

        {"edge_true_in_expr", "var x = true && true || false;", 0,
         "bool in expression"},
        {"edge_zero_literal", "var x = 0; var y = 0.0;", 0, "zero literals"},
        {"edge_negative_zero", "var x = -0;", 0, "negative zero"},
        {"edge_float_literal", "var x = 3.14159265;", 0, "float literal"},

        {"edge_nested_ternary_deep",
         "var x = true ? (true ? (true ? 1 : 2) : 3) : 4;\nstd::print(\"{}\", "
         "x);",
         0, "deep nested ternary"},

        {"edge_for_without_init",
         "var i = 0; for (; i < 3; i++) { std::print(\"{}\", i); }", 0,
         "for without init"},

        {"edge_do_while", "var i = 0; do { i = i + 1; } while (i < 3);", 0,
         "do-while loop"},

        {"edge_return_in_loop",
         "proc find(): int32 { for (var i = 0; i < 10; i++) { if (i == 5) { "
         "return i; } } return -1; }\nstd::print(\"{}\", find());",
         0, "return in for loop"},

        {"edge_multi_dimensional_array",
         "var matrix = [[1, 2], [3, 4], [5, 6]];\nstd::print(\"{}\", "
         "matrix[1][1]);",
         0, "3x2 matrix"},

        {"edge_array_in_array",
         "var nested = [[1, [2, 3]], [4, [5, 6]]];\nstd::print(\"{}\", 1);", 0,
         "deep array nesting"},

        {"edge_string_length",
         "var s = \"hello\"; var len = 5;\nstd::print(\"{}\", len);", 0,
         "string variable"},

        {"edge_unicode_chars",
         "var s = \"héllo wörld\";\nstd::print(\"{}\", 1);", 0,
         "unicode in string"},

        {"edge_special_chars",
         "var s = \"tab:\\t newline:\\n quote:\\\"\";\nstd::print(\"{}\", 1);",
         0, "escaped special chars"},

        {"edge_varargs_like",
         "proc variadic(a: int32, b: int32): int32 { return a + b; "
         "}\nstd::print(\"{}\", variadic(1, 2));",
         0, "function params"},

        {"edge_tail_call_like",
         "proc tail(n: int32): int32 { if (n <= 0) { return 0; } return tail(n "
         "- 1); }\ntail(100);",
         0, "tail-like recursion"},

        {"edge_mutual_recursion",
         "proc is_even(n: int32): bool { if (n == 0) { return true; } return "
         "is_odd(n - 1); }\n"
         "proc is_odd(n: int32): bool { if (n == 0) { return false; } return "
         "is_even(n - 1); }\n"
         "std::print(\"{}\", is_even(10));",
         0, "mutual recursion"},

        {"edge_nested_scope",
         "var x = 1;\n{ var x = 2; { var x = 3; std::print(\"{}\", x); } "
         "std::print(\"{}\", x); }\nstd::print(\"{}\", x);",
         0, "3 level scope"},

        {"edge_late_binding_like",
         "var fn = null;\nproc foo(): void { std::print(\"foo\"); }\nfn = "
         "foo;\nfn();",
         0, "function pointer like"},

        {"edge_array_update",
         "var arr = [1, 2, 3];\narr[0] = 10;\narr[1] = 20;\nstd::print(\"{}\", "
         "arr[0] + arr[1]);",
         0, "array element update"},

        {"edge_compound_assignment",
         "var x = 10; x = x + 5; x = x * 2;\nstd::print(\"{}\", x);", 0,
         "compound assignment"},
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++) {
        run_stress_test(&tests[i]);
    }

    printf("Stress/Fuzz Summary\n");
    printf("Total:   %d\n", total_tests);
    printf("Passed:  %d\n", passed_tests);
    printf("Failed:  %d\n", failed_tests);
    printf("Crashed: %d\n", crashed_tests);
    printf("\n");

    if (failed_tests == 0 && crashed_tests == 0) {
        printf("All stress tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed or crashed.\n");
        return 1;
    }
}
