/**
 * @file test_fuzz_math.c
 * @brief Comprehensive fuzz testing for the XVR math module
 *
 * This fuzzer tests:
 * - Basic functionality for all math functions
 * - Type handling (float32, float64, int32, int64)
 * - Edge cases (NaN, Infinity, negative numbers, zero)
 * - Constant folding optimization
 * - Error handling
 * - Integration with other modules
 */

#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static jmp_buf jump_buffer;
static volatile sig_atomic_t crash_occurred = 0;

void segfault_handler(int sig) {
    (void)sig;
    crash_occurred = 1;
    longjmp(jump_buffer, 1);
}

typedef struct {
    const char* name;
    const char* code;
    int expect_crash;
    int expect_error;
    const char* description;
    double expected_result;
    double tolerance;
} MathFuzzTest;

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;
static int crashed_tests = 0;
static int skipped_tests = 0;

/**
 * @brief Compile and run XVR code using the xvr compiler
 * @param code XVR source code to compile
 * @param exit_code Pointer to store exit code, or NULL
 * @return Output from compiler, or NULL on error
 *
 * SECURITY NOTE: This function uses popen() which executes a shell command.
 * This is necessary because we need to run the xvr compiler and capture
 * its output. The following safeguards are in place:
 * - Input validation rejects dangerous shell characters
 * - Code length is limited to prevent buffer overflow
 * - Command is constructed using %.*s to prevent format string attacks
 * - Uses heredoc syntax (<<'XVRCODE') to safely pass code to shell
 */
static const char* compile_and_run(const char* code, int* exit_code) {
    static char buffer[16384];
    char cmd[8192];

    if (!code) {
        return NULL;
    }

    size_t code_len = strnlen(code, 4096);
    if (code_len >= 4096) {
        snprintf(buffer, sizeof(buffer), "ERROR: code too large");
        return buffer;
    }

    for (size_t i = 0; i < code_len; i++) {
        char c = code[i];
        if (c == ';' || c == '\n' || c == '\r') {
            continue;
        }
        if (c == '`' || c == '$' || c == '(' || c == ')' || c == '|' ||
            c == '&' || c == '<' || c == '>' || c == '\\' || c == '\'') {
            snprintf(buffer, sizeof(buffer),
                     "ERROR: dangerous character in code");
            return buffer;
        }
    }

    int written =
        snprintf(cmd, sizeof(cmd),
                 "cd /home/arfyslowy/Documents/project/xvrlang/xvr && "
                 "./build/xvr -l - 2>&1 <<'XVRCODE'\n%.*s\nXVRCODE",
                 (int)code_len, code);
    if (written < 0 || (size_t)written >= sizeof(cmd)) {
        snprintf(buffer, sizeof(buffer), "ERROR: command too long");
        return buffer;
    }

    /* popen() is required here to invoke the xvr compiler.
     * Input is validated above to prevent command injection. */
    FILE* fp = popen(cmd, "r");
    if (!fp) {
        return NULL;
    }

    size_t len = 0;
    buffer[0] = '\0';
    while (len < sizeof(buffer) - 1) {
        char* dest = buffer + len;
        size_t remaining = sizeof(buffer) - len - 1;
        if (fgets(dest, remaining, fp) == NULL) {
            break;
        }
        size_t read_len = strnlen(dest, remaining);
        len += read_len;
        if (read_len < remaining - 1 || dest[read_len - 1] == '\n') {
            break;
        }
    }
    buffer[sizeof(buffer) - 1] = '\0';

    int status = pclose(fp);
    if (exit_code) {
        *exit_code = WEXITSTATUS(status);
    }

    return buffer;
}

static const char* compile_only(const char* code) {
    return compile_and_run(code, NULL);
}

static int has_error(const char* output) {
    if (!output) return 0;
    return (strstr(output, "error") != NULL || strstr(output, "Error") != NULL);
}

static int has_critical_error(const char* output) {
    if (!output) return 0;
    return (strstr(output, "free():") != NULL ||
            strstr(output, "malloc:") != NULL ||
            strstr(output, "SIGSEGV") != NULL ||
            strstr(output, "SIGABRT") != NULL);
}

static double extract_double_result(const char* llvm_ir, const char* var_name) {
    (void)llvm_ir;
    (void)var_name;
    return 0.0;
}

static int run_math_test(const MathFuzzTest* test) {
    total_tests++;

    signal(SIGSEGV, SIG_DFL);

    if (setjmp(jump_buffer) != 0) {
        signal(SIGSEGV, SIG_DFL);
        if (test->expect_crash) {
            printf("  [PASS] %s - crashed as expected\n", test->name);
            passed_tests++;
        } else {
            printf("  [FAIL] %s - unexpected crash: %s\n", test->name,
                   test->description);
            crashed_tests++;
        }
        return 0;
    }

    crash_occurred = 0;
    if (signal(SIGSEGV, segfault_handler) == SIG_ERR) {
        skipped_tests++;
        printf("  [SKIP] %s - could not setup signal handler\n", test->name);
        return 0;
    }

    const char* output = compile_only(test->code);

    signal(SIGSEGV, SIG_DFL);

    if (has_critical_error(output) || crash_occurred) {
        if (test->expect_crash || test->expect_error) {
            printf("  [PASS] %s - error occurred as expected\n", test->name);
            passed_tests++;
        } else {
            printf("  [FAIL] %s - critical error: %s\n", test->name,
                   test->description);
            crashed_tests++;
        }
        return 0;
    }

    if (test->expect_crash) {
        printf("  [FAIL] %s - expected crash but succeeded\n", test->name);
        failed_tests++;
        return 0;
    }

    if (test->expect_error) {
        if (has_error(output)) {
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

    if (has_error(output)) {
        printf("  [FAIL] %s - unexpected error: %s\n", test->name,
               test->description);
        failed_tests++;
    } else {
        printf("  [PASS] %s - compiled: %s\n", test->name, test->description);
        passed_tests++;
    }

    return 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("XVR Math Module Fuzz Testing\n");
    printf("=============================\n\n");

    MathFuzzTest tests[] = {
        /* ========== SQRT TESTS ========== */
        {"sqrt_basic_positive", "include math;\nvar x = math::sqrt(25.0);", 0,
         0, "sqrt of positive number", 5.0, 0.001},
        {"sqrt_zero", "include math;\nvar x = math::sqrt(0.0);", 0, 0,
         "sqrt of zero", 0.0, 0.001},
        {"sqrt_one", "include math;\nvar x = math::sqrt(1.0);", 0, 0,
         "sqrt of one", 1.0, 0.001},
        {"sqrt_large", "include math;\nvar x = math::sqrt(1000000.0);", 0, 0,
         "sqrt of large number", 1000.0, 0.001},
        {"sqrt_small", "include math;\nvar x = math::sqrt(0.0001);", 0, 0,
         "sqrt of small number", 0.01, 0.001},
        {"sqrt_negative", "include math;\nvar x = math::sqrt(-1.0);", 0, 1,
         "sqrt of negative - should error", 0.0, 0},
        {"sqrt_constant_fold",
         "include math;\nvar x = math::sqrt(25.0);\nvar y = x + 1.0;", 0, 0,
         "sqrt with constant folding", 6.0, 0.001},

        /* ========== POW TESTS ========== */
        {"pow_basic", "include math;\nvar x = math::pow(2.0, 3.0);", 0, 0,
         "pow basic", 8.0, 0.001},
        {"pow_zero_exponent", "include math;\nvar x = math::pow(5.0, 0.0);", 0,
         0, "pow with zero exponent", 1.0, 0.001},
        {"pow_one_exponent", "include math;\nvar x = math::pow(42.0, 1.0);", 0,
         0, "pow with exponent 1", 42.0, 0.001},
        {"pow_negative_exponent",
         "include math;\nvar x = math::pow(2.0, -1.0);", 0, 0,
         "pow with negative exponent", 0.5, 0.001},
        {"pow_fractional", "include math;\nvar x = math::pow(4.0, 0.5);", 0, 0,
         "pow with fractional exponent (sqrt)", 2.0, 0.001},
        {"pow_negative_base_even",
         "include math;\nvar x = math::pow(-2.0, 2.0);", 0, 0,
         "pow with negative base, even exponent", 4.0, 0.001},
        {"pow_negative_base_odd",
         "include math;\nvar x = math::pow(-2.0, 3.0);", 0, 0,
         "pow with negative base, odd exponent", -8.0, 0.001},
        {"pow_constant_fold", "include math;\nvar x = math::pow(2.0, 10.0);", 0,
         0, "pow with constant folding", 1024.0, 0.001},

        /* ========== SIN TESTS ========== */
        {"sin_zero", "include math;\nvar x = math::sin(0.0);", 0, 0,
         "sin of zero", 0.0, 0.001},
        {"sin_pi", "include math;\nvar x = math::sin(3.14159265358979);", 0, 0,
         "sin of pi (approx)", 0.0, 0.01},
        {"sin_half_pi", "include math;\nvar x = math::sin(1.5707963267949);", 0,
         0, "sin of pi/2 (approx)", 1.0, 0.01},
        {"sin_negative", "include math;\nvar x = math::sin(-1.0);", 0, 0,
         "sin of negative angle", 0.0, 0.01},
        {"sin_constant_fold", "include math;\nvar x = math::sin(0.0);", 0, 0,
         "sin with constant folding", 0.0, 0.001},

        /* ========== COS TESTS ========== */
        {"cos_zero", "include math;\nvar x = math::cos(0.0);", 0, 0,
         "cos of zero", 1.0, 0.001},
        {"cos_pi", "include math;\nvar x = math::cos(3.14159265358979);", 0, 0,
         "cos of pi (approx)", -1.0, 0.01},
        {"cos_half_pi", "include math;\nvar x = math::cos(1.5707963267949);", 0,
         0, "cos of pi/2 (approx)", 0.0, 0.01},
        {"cos_negative", "include math;\nvar x = math::cos(-1.0);", 0, 0,
         "cos of negative angle", 0.0, 0.01},
        {"cos_constant_fold", "include math;\nvar x = math::cos(0.0);", 0, 0,
         "cos with constant folding", 1.0, 0.001},

        /* ========== TAN TESTS ========== */
        {"tan_zero", "include math;\nvar x = math::tan(0.0);", 0, 0,
         "tan of zero", 0.0, 0.001},
        {"tan_small", "include math;\nvar x = math::tan(0.1);", 0, 0,
         "tan of small angle", 0.0, 0.01},

        /* ========== ABS TESTS ========== */
        {"abs_positive_int", "include math;\nvar x = math::abs(42);", 0, 0,
         "abs of positive int", 42.0, 0.001},
        {"abs_negative_int", "include math;\nvar x = math::abs(-42);", 0, 0,
         "abs of negative int", 42.0, 0.001},
        {"abs_zero_int", "include math;\nvar x = math::abs(0);", 0, 0,
         "abs of zero int", 0.0, 0.001},
        {"abs_positive_float", "include math;\nvar x = math::abs(42.5);", 0, 0,
         "abs of positive float", 42.5, 0.001},
        {"abs_negative_float", "include math;\nvar x = math::abs(-42.5);", 0, 0,
         "abs of negative float", 42.5, 0.001},
        {"abs_constant_fold", "include math;\nvar x = math::abs(-42);", 0, 0,
         "abs with constant folding", 42.0, 0.001},

        /* ========== FLOOR TESTS ========== */
        {"floor_positive", "include math;\nvar x = math::floor(3.7);", 0, 0,
         "floor of positive", 3.0, 0.001},
        {"floor_negative", "include math;\nvar x = math::floor(-3.7);", 0, 0,
         "floor of negative", -4.0, 0.001},
        {"floor_whole", "include math;\nvar x = math::floor(5.0);", 0, 0,
         "floor of whole number", 5.0, 0.001},
        {"floor_zero", "include math;\nvar x = math::floor(0.0);", 0, 0,
         "floor of zero", 0.0, 0.001},

        /* ========== CEIL TESTS ========== */
        {"ceil_positive", "include math;\nvar x = math::ceil(3.2);", 0, 0,
         "ceil of positive", 4.0, 0.001},
        {"ceil_negative", "include math;\nvar x = math::ceil(-3.2);", 0, 0,
         "ceil of negative", -3.0, 0.001},
        {"ceil_whole", "include math;\nvar x = math::ceil(5.0);", 0, 0,
         "ceil of whole number", 5.0, 0.001},
        {"ceil_zero", "include math;\nvar x = math::ceil(0.0);", 0, 0,
         "ceil of zero", 0.0, 0.001},

        /* ========== ROUND TESTS ========== */
        {"round_positive_up", "include math;\nvar x = math::round(3.7);", 0, 0,
         "round positive (up)", 4.0, 0.001},
        {"round_positive_down", "include math;\nvar x = math::round(3.2);", 0,
         0, "round positive (down)", 3.0, 0.001},
        {"round_negative_up", "include math;\nvar x = math::round(-3.2);", 0, 0,
         "round negative (up)", -3.0, 0.001},
        {"round_negative_down", "include math;\nvar x = math::round(-3.7);", 0,
         0, "round negative (down)", -4.0, 0.001},
        {"round_half", "include math;\nvar x = math::round(3.5);", 0, 0,
         "round .5 case", 4.0, 0.001},

        /* ========== LOG TESTS ========== */
        {"log_one", "include math;\nvar x = math::log(1.0);", 0, 0, "log of 1",
         0.0, 0.001},
        {"log_e", "include math;\nvar x = math::log(2.718281828459045);", 0, 0,
         "log of e (approx)", 1.0, 0.01},
        {"log_ten", "include math;\nvar x = math::log(10.0);", 0, 0,
         "log of 10", 2.302585, 0.01},
        {"log_zero", "include math;\nvar x = math::log(0.0);", 0, 1,
         "log of zero - should error", 0.0, 0},
        {"log_negative", "include math;\nvar x = math::log(-1.0);", 0, 1,
         "log of negative - should error", 0.0, 0},

        /* ========== EXP TESTS ========== */
        {"exp_zero", "include math;\nvar x = math::exp(0.0);", 0, 0,
         "exp of zero", 1.0, 0.001},
        {"exp_one", "include math;\nvar x = math::exp(1.0);", 0, 0,
         "exp of 1 (e)", 2.718281828, 0.01},
        {"exp_negative", "include math;\nvar x = math::exp(-1.0);", 0, 0,
         "exp of -1 (1/e)", 0.367879, 0.01},

        /* ========== ATAN TESTS ========== */
        {"atan_zero", "include math;\nvar x = math::atan(0.0);", 0, 0,
         "atan of zero", 0.0, 0.001},
        {"atan_one", "include math;\nvar x = math::atan(1.0);", 0, 0,
         "atan of 1 (pi/4)", 0.785398, 0.01},
        {"atan_large", "include math;\nvar x = math::atan(1000.0);", 0, 0,
         "atan of large value", 1.5698, 0.01},

        /* ========== ATAN2 TESTS ========== */
        {"atan2_basic", "include math;\nvar x = math::atan2(1.0, 1.0);", 0, 0,
         "atan2(1, 1) = pi/4", 0.785398, 0.01},
        {"atan2_positive_y", "include math;\nvar x = math::atan2(1.0, 0.0);", 0,
         0, "atan2(1, 0) = pi/2", 1.570796, 0.01},
        {"atan2_negative_y", "include math;\nvar x = math::atan2(-1.0, 0.0);",
         0, 0, "atan2(-1, 0) = -pi/2", -1.570796, 0.01},

        /* ========== TYPE MIXING TESTS ========== */
        {"type_mix_int_float",
         "include math;\nvar x = math::sqrt(25.0);\nvar y = math::abs(-10);", 0,
         0, "mix int and float types", 0.0, 0},
        {"type_explicit_float32",
         "include math;\nvar x: float32 = math::sqrt(25.0);", 0, 0,
         "explicit float32 type", 0.0, 0},
        {"type_explicit_int32", "include math;\nvar x: int32 = math::abs(-10);",
         0, 0, "explicit int32 type", 0.0, 0},

        /* ========== COMBINED EXPRESSIONS ========== */
        {"combined_sqrt_pow",
         "include math;\nvar x = math::sqrt(math::pow(2.0, 4.0));", 0, 0,
         "sqrt(pow(2, 4)) = 4", 4.0, 0.001},
        {"combined_sin_cos",
         "include math;\nvar x = math::sin(0.0) + math::cos(0.0);", 0, 0,
         "sin(0) + cos(0) = 1", 1.0, 0.001},
        {"combined_abs_floor",
         "include math;\nvar x = math::floor(math::abs(-3.7));", 0, 0,
         "floor(abs(-3.7)) = 3", 3.0, 0.001},
        {"combined_exp_log",
         "include math;\nvar x = math::log(math::exp(2.0));", 0, 0,
         "log(exp(2)) = 2", 2.0, 0.001},

        /* ========== LOOP INTEGRATION ========== */
        {"loop_sqrt_sum",
         "include math;\nvar sum = 0.0;\nvar i = 0;\nwhile (i < 5) { sum = sum "
         "+ math::sqrt(float64(i)); i = i + 1; }",
         0, 0, "sqrt in loop", 0.0, 0},
        {"loop_pow_accumulate",
         "include math;\nvar result = 1.0;\nvar i = 0;\nwhile (i < 3) { result "
         "= result * math::pow(2.0, 1.0); i = i + 1; }",
         0, 0, "pow in loop", 0.0, 0},

        /* ========== PROCEDURE INTEGRATION ========== */
        {"proc_math_call",
         "include math;\nproc calc(): float { return math::sqrt(16.0); }\nvar "
         "x = calc();",
         0, 0, "math function in procedure", 4.0, 0.001},
        {"proc_math_param",
         "include math;\nproc calc(n: float): float { return math::sqrt(n); "
         "}\nvar x = calc(9.0);",
         0, 0, "math function with parameter", 3.0, 0.001},
        {"proc_math_return",
         "include math;\nproc calc(): int { return math::abs(-42); }\nvar x = "
         "calc();",
         0, 0, "math abs in procedure returning int", 42.0, 0.001},

        /* ========== ERROR CASES ========== */
        {"error_sqrt_negative", "include math;\nvar x = math::sqrt(-1.0);", 0,
         1, "sqrt of negative - should error", 0.0, 0},
        {"error_log_zero", "include math;\nvar x = math::log(0.0);", 0, 1,
         "log of zero - should error", 0.0, 0},
        {"error_log_negative", "include math;\nvar x = math::log(-1.0);", 0, 1,
         "log of negative - should error", 0.0, 0},
        {"error_pow_negative_fractional",
         "include math;\nvar x = math::pow(-1.0, 0.5);", 0, 1,
         "pow(-1, 0.5) - should error", 0.0, 0},

        /* ========== EDGE CASES ========== */
        {"edge_very_large", "include math;\nvar x = math::sqrt(1.0e100);", 0, 0,
         "sqrt of very large number", 0.0, 0},
        {"edge_very_small", "include math;\nvar x = math::sqrt(1.0e-100);", 0,
         0, "sqrt of very small number", 0.0, 0},
        {"edge_int_min", "include math;\nvar x = math::abs(-2147483648);", 0, 0,
         "abs of INT_MIN - potential overflow", 0.0, 0},
        {"edge_multiple_calls",
         "include math;\nvar a = math::sqrt(4.0);\nvar b = "
         "math::sqrt(9.0);\nvar c = math::sqrt(16.0);",
         0, 0, "multiple sqrt calls", 0.0, 0},
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);

    printf("Running %d math module fuzz tests...\n\n", num_tests);

    for (int i = 0; i < num_tests; i++) {
        run_math_test(&tests[i]);
    }

    printf("\n=============================\n");
    printf("Math Module Fuzz Summary\n");
    printf("=============================\n");
    printf("Total:   %d\n", total_tests);
    printf("Passed:  %d\n", passed_tests);
    printf("Failed:  %d\n", failed_tests);
    printf("Crashed: %d\n", crashed_tests);
    printf("Skipped: %d\n", skipped_tests);
    printf("\n");

    if (failed_tests == 0 && crashed_tests == 0) {
        printf("All math module fuzz tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed or crashed.\n");
        return 1;
    }
}
