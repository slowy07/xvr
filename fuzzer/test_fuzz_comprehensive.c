#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef XVR_FUZZ_STANDALONE
#    define FUZZ_IMPLEMENTATION
#endif

typedef struct {
    const char* name;
    const char* code;
    int expect_crash;
    int expect_error;
} FuzzTest;

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;
static int crashed_tests = 0;

int run_fuzz_test(const FuzzTest* test) {
    total_tests++;

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
             "cd /home/arfyslowy/Documents/project/xvrlang/xvr && "
             "./build/xvr -l - 2>&1 <<'XVRCODE'\n%s\nXVRCODE",
             test->code);

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        printf("  [FAIL] %s - could not run compiler\n", test->name);
        failed_tests++;
        return 0;
    }

    char buffer[4096];
    size_t output_len = 0;
    int has_error = 0;

    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, "error") || strstr(buffer, "Error")) {
            has_error = 1;
        }
        output_len += strlen(buffer);
        if (output_len > 4096) break;
    }

    int status = pclose(fp);
    int exit_code = WEXITSTATUS(status);

    int crashed = (status != 0 && WIFSIGNALED(status));

    if (crashed) {
        printf("  [CRASH] %s - compiler crashed (signal %d)\n", test->name,
               WTERMSIG(status));
        crashed_tests++;
        return 0;
    }

    if (test->expect_crash) {
        if (crashed) {
            printf("  [PASS] %s - expected crash occurred\n", test->name);
            passed_tests++;
        } else {
            printf("  [FAIL] %s - expected crash but didn't crash\n",
                   test->name);
            failed_tests++;
        }
        return 0;
    }

    if (test->expect_error) {
        if (has_error || exit_code != 0) {
            printf("  [PASS] %s - expected error occurred\n", test->name);
            passed_tests++;
        } else {
            printf("  [FAIL] %s - expected error but compiled successfully\n",
                   test->name);
            failed_tests++;
        }
        return 0;
    }

    if (!has_error && exit_code == 0) {
        printf("  [PASS] %s\n", test->name);
        passed_tests++;
    } else if (has_error) {
        printf("  [FAIL] %s - unexpected compilation error\n", test->name);
        failed_tests++;
    } else {
        printf("  [FAIL] %s - unexpected exit code %d\n", test->name,
               exit_code);
        failed_tests++;
    }

    return 0;
}

int main(int argc, char** argv) {
    printf("XVR Fuzz Test Suite\n");
    printf("===================\n\n");

    FuzzTest tests[] = {
        {"empty_input", "", 0, 0},
        {"whitespace_only", "   \n\t  ", 0, 0},

        {"basic_var_declaration", "var x = 42;\nstd::print(\"{}\", x);", 0, 0},
        {"multiple_var_declarations", "var a = 1;\nvar b = 2;\nvar c = 3;", 0,
         0},

        {"arithmetic_add", "var x = 1 + 2;\nstd::print(\"{}\", x);", 0, 0},
        {"arithmetic_sub", "var x = 5 - 3;\nstd::print(\"{}\", x);", 0, 0},
        {"arithmetic_mul", "var x = 4 * 3;\nstd::print(\"{}\", x);", 0, 0},
        {"arithmetic_div", "var x = 10 / 2;\nstd::print(\"{}\", x);", 0, 0},
        {"arithmetic_mod", "var x = 10 % 3;\nstd::print(\"{}\", x);", 0, 0},

        {"arithmetic_precedence", "var x = 2 + 3 * 4;\nstd::print(\"{}\", x);",
         0, 0},
        {"arithmetic_parens", "var x = (2 + 3) * 4;\nstd::print(\"{}\", x);", 0,
         0},

        {"comparison_eq", "var x = 1 == 1;\nstd::print(\"{}\", x);", 0, 0},
        {"comparison_ne", "var x = 1 != 2;\nstd::print(\"{}\", x);", 0, 0},
        {"comparison_gt", "var x = 5 > 3;\nstd::print(\"{}\", x);", 0, 0},
        {"comparison_lt", "var x = 3 < 5;\nstd::print(\"{}\", x);", 0, 0},
        {"comparison_ge", "var x = 5 >= 5;\nstd::print(\"{}\", x);", 0, 0},
        {"comparison_le", "var x = 3 <= 3;\nstd::print(\"{}\", x);", 0, 0},

        {"logical_and", "var x = true && true;\nstd::print(\"{}\", x);", 0, 0},
        {"logical_or", "var x = false || true;\nstd::print(\"{}\", x);", 0, 0},
        {"logical_not", "var x = !false;\nstd::print(\"{}\", x);", 0, 0},

        {"if_true", "if (true) { std::print(\"yes\"); }", 0, 0},
        {"if_false", "if (false) { std::print(\"no\"); }", 0, 0},
        {"if_else",
         "if (true) { std::print(\"yes\"); } else { std::print(\"no\"); }", 0,
         0},
        {"if_else_if",
         "if (false) { std::print(\"a\"); } else if (true) { "
         "std::print(\"b\"); }",
         0, 0},

        {"if_with_var", "var x = 5;\nif (x > 3) { std::print(\"big\"); }", 0,
         0},

        {"while_basic",
         "var i = 0;\nwhile (i < 3) { i = i + 1; }\nstd::print(\"{}\", i);", 0,
         0},
        {"while_true_break", "while (true) { break; }\nstd::print(\"done\");",
         0, 0},

        {"for_basic",
         "var sum = 0;\nfor (var i = 0; i < 5; i++) { sum = sum + i; "
         "}\nstd::print(\"{}\", sum);",
         0, 0},
        {"for_with_break",
         "for (var i = 0; i < 10; i++) { if (i == 5) { break; } "
         "}\nstd::print(\"5\");",
         0, 0},
        {"for_with_continue",
         "var sum = 0;\nfor (var i = 0; i < 5; i++) { if (i == 2) { continue; "
         "} sum = sum + i; }\nstd::print(\"{}\", sum);",
         0, 0},

        {"nested_for",
         "var total = 0;\nfor (var i = 0; i < 3; i++) { for (var j = 0; j < 3; "
         "j++) { total = total + 1; } }\nstd::print(\"{}\", total);",
         0, 0},

        {"proc_void_no_params",
         "proc greet(): void { std::print(\"hi\"); }\ngreet();", 0, 0},
        {"proc_void_with_params",
         "proc say(msg: string): void { std::print(\"{}\", msg); "
         "}\nsay(\"hello\");",
         0, 0},
        {"proc_return_int",
         "proc add(a: int32, b: int32): int32 { return a + b; "
         "}\nstd::print(\"{}\", add(1, 2));",
         0, 0},

        {"proc_nested_calls",
         "proc double(x: int32): int32 { return x * 2; }\nproc square(x: "
         "int32): int32 { return x * x; }\nstd::print(\"{}\", "
         "double(square(3)));",
         0, 0},

        {"proc_recursive",
         "proc factorial(n: int32): int32 { if (n <= 1) { return 1; } return n "
         "* factorial(n - 1); }\nstd::print(\"{}\", factorial(5));",
         0, 0},

        {"type_int32", "var x: int32 = 42;\nstd::print(\"{}\", x);", 0, 0},
        {"type_int64", "var x: int64 = 42;\nstd::print(\"{}\", x);", 0, 0},
        {"type_float32", "var x: float32 = 3.14;\nstd::print(\"{}\", x);", 0,
         0},
        {"type_float64", "var x: float64 = 3.14;\nstd::print(\"{}\", x);", 0,
         0},
        {"type_bool", "var x: bool = true;\nstd::print(\"{}\", x);", 0, 0},
        {"type_string", "var x: string = \"hello\";\nstd::print(\"{}\", x);", 0,
         0},

        {"cast_int32_to_float",
         "var x: int32 = 42;\nvar y = float64(x);\nstd::print(\"{}\", y);", 0,
         0},
        {"cast_float_to_int",
         "var x: float64 = 3.14;\nvar y = int32(x);\nstd::print(\"{}\", y);", 0,
         0},

        {"array_literal", "var arr = [1, 2, 3];\nstd::print(\"{}\", arr[0]);",
         0, 0},
        {"array_access", "var arr = [10, 20, 30];\nstd::print(\"{}\", arr[1]);",
         0, 0},
        {"array_multi",
         "var arr = [[1, 2], [3, 4]];\nstd::print(\"{}\", arr[0][0]);", 0, 0},

        {"string_basic", "var s = \"hello\";\nstd::print(\"{}\", s);", 0, 0},
        {"string_concat",
         "var s = \"hello\" + \" \" + \"world\";\nstd::print(\"{}\", s);", 0,
         0},

        {"ternary_basic", "var x = true ? 1 : 2;\nstd::print(\"{}\", x);", 0,
         0},
        {"ternary_nested",
         "var x = true ? (false ? 1 : 2) : 3;\nstd::print(\"{}\", x);", 0, 0},

        {"increment_post", "var x = 5;\nx++;\nstd::print(\"{}\", x);", 0, 0},
        {"increment_pre", "var x = 5;\n++x;\nstd::print(\"{}\", x);", 0, 0},
        {"decrement_post", "var x = 5;\nx--;\nstd::print(\"{}\", x);", 0, 0},
        {"decrement_pre", "var x = 5;\n--x;\nstd::print(\"{}\", x);", 0, 0},

        {"break_in_for",
         "for (var i = 0; i < 100; i++) { if (i == 10) { break; } "
         "}\nstd::print(\"10\");",
         0, 0},
        {"continue_in_for",
         "var sum = 0;\nfor (var i = 0; i < 5; i++) { if (i == 2) { continue; "
         "} sum = sum + i; }\nstd::print(\"{}\", sum);",
         0, 0},

        {"deep_nesting_if",
         "if (true) { if (true) { if (true) { std::print(\"deep\"); } } }", 0,
         0},

        {"var_shadowing",
         "var x = 1;\n{ var x = 2; std::print(\"{}\", x); "
         "}\nstd::print(\"{}\", x);",
         0, 0},

        {"complex_expression",
         "var x = 1 + 2 * 3 - 4 / 2 + (5 - 3);\nstd::print(\"{}\", x);", 0, 0},

        {"negative_numbers",
         "var x = -5;\nvar y = -3 + -2;\nstd::print(\"{}\", y);", 0, 0},

        {"zero_values",
         "var a = 0;\nvar b = 0.0;\nvar c = false;\nstd::print(\"{}\", a);", 0,
         0},

        {"print_multiple_args", "std::print(\"{} {} {}\", 1, 2, 3);", 0, 0},
        {"print_format_string", "std::print(\"a={} b={}\", 10, 20);", 0, 0},

        {"empty_array", "var arr = [];\nstd::print(\"{}\", 0);", 0, 0},
        {"empty_string", "var s = \"\";\nstd::print(\"{}\", 0);", 0, 0},

        {"single_char_string", "var s = \"a\";\nstd::print(\"{}\", s);", 0, 0},

        {"unicode_string", "var s = \"hello world\";\nstd::print(\"{}\", s);",
         0, 0},

        {"unused_var_warning", "var unused = 42;\nstd::print(\"test\");", 0, 0},

        {"long_variable_name",
         "var very_long_variable_name_that_is_descriptive = "
         "1;\nstd::print(\"{}\", very_long_variable_name_that_is_descriptive);",
         0, 0},

        {"chained_assignment",
         "var a; var b; var c;\na = 1; b = 2; c = a + b;\nstd::print(\"{}\", "
         "c);",
         0, 0},

        {"return_in_void", "proc foo(): void { return; }\nfoo();", 0, 0},
        {"return_value",
         "proc bar(): int32 { return 42; }\nstd::print(\"{}\", bar());", 0, 0},

        {"for_decrement",
         "for (var i = 5; i > 0; i--) { std::print(\"{}\", i); }", 0, 0},
        {"for_no_init",
         "var i = 0;\nfor (; i < 3; i++) { std::print(\"{}\", i); }", 0, 0},

        {"while_nested",
         "var i = 0;\nwhile (i < 2) { var j = 0; while (j < 2) { j = j + 1; } "
         "i = i + 1; }\nstd::print(\"done\");",
         0, 0},

        {"implicit_return",
         "proc five(): int32 { 5 }\nstd::print(\"{}\", five());", 0, 0},
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < num_tests; i++) {
        run_fuzz_test(&tests[i]);
    }

    printf("\n===================\n");
    printf("Test Summary\n");
    printf("===================\n");
    printf("Total:   %d\n", total_tests);
    printf("Passed:  %d\n", passed_tests);
    printf("Failed:  %d\n", failed_tests);
    printf("Crashed: %d\n", crashed_tests);
    printf("\n");

    if (failed_tests == 0 && crashed_tests == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed or crashed.\n");
        return 1;
    }
}
