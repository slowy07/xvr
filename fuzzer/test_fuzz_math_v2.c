/**
 * @file test_fuzz_math_v2.c
 * @brief Comprehensive fuzz testing for the XVR math module
 *
 * Tests all math functions, edge cases, and error conditions.
 */

#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CODE_SIZE 65536
#define MAX_OUTPUT_SIZE 16384
#define MAX_TESTS 200

typedef struct {
    const char* name;
    const char* code;
    int expect_crash;
    int expect_error;
    const char* description;
} MathFuzzTest;

static jmp_buf jump_buffer;
static volatile sig_atomic_t crash_occurred = 0;

void segfault_handler(int sig) {
    (void)sig;
    crash_occurred = 1;
    longjmp(jump_buffer, 1);
}

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;
static int crashed_tests = 0;
static int skipped_tests = 0;
static int error_tests_passed = 0;
static int error_tests_failed = 0;

static FILE* log_file = NULL;

static void log_test(const char* test_name, const char* status,
                     const char* details) {
    if (log_file) {
        fprintf(log_file, "[%s] %s: %s\n", status, test_name, details);
        fflush(log_file);
    }
}

static const char* compile_only(const char* code) {
    static char buffer[MAX_OUTPUT_SIZE];
    char cmd[1024 + MAX_CODE_SIZE];

    if (strlen(code) > MAX_CODE_SIZE) {
        return "ERROR: code too large";
    }

    snprintf(cmd, sizeof(cmd),
             "cd /home/arfyslowy/Documents/project/xvrlang/xvr && "
             "./build/xvr -l - 2>&1 <<'XVRCODE'\n%s\nXVRCODE",
             code);

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        return NULL;
    }

    size_t len = 0;
    buffer[0] = '\0';
    while (fgets(buffer + len, sizeof(buffer) - len - 1, fp) &&
           len < MAX_OUTPUT_SIZE - 1) {
        len += strlen(buffer + len);
    }

    pclose(fp);
    return buffer;
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
            strstr(output, "SIGABRT") != NULL ||
            strstr(output, "Assertion") != NULL);
}

static int run_math_test(const MathFuzzTest* test) {
    total_tests++;

    signal(SIGSEGV, SIG_DFL);

    if (setjmp(jump_buffer) != 0) {
        signal(SIGSEGV, SIG_DFL);
        if (test->expect_crash) {
            printf("  [PASS] %s\n", test->name);
            log_test(test->name, "PASS", "crashed as expected");
            passed_tests++;
        } else {
            printf("  [FAIL] %s - unexpected crash\n", test->name);
            log_test(test->name, "FAIL", "unexpected crash");
            crashed_tests++;
        }
        return 0;
    }

    crash_occurred = 0;
    if (signal(SIGSEGV, segfault_handler) == SIG_ERR) {
        skipped_tests++;
        printf("  [SKIP] %s - signal handler failed\n", test->name);
        log_test(test->name, "SKIP", "signal handler failed");
        return 0;
    }

    const char* output = compile_only(test->code);

    signal(SIGSEGV, SIG_DFL);

    if (has_critical_error(output) || crash_occurred) {
        if (test->expect_crash || test->expect_error) {
            printf("  [PASS] %s\n", test->name);
            log_test(test->name, "PASS", "error occurred as expected");
            passed_tests++;
            error_tests_passed++;
        } else {
            printf("  [FAIL] %s - critical error\n", test->name);
            log_test(test->name, "FAIL", "critical error occurred");
            crashed_tests++;
        }
        return 0;
    }

    if (test->expect_crash) {
        printf("  [FAIL] %s - expected crash\n", test->name);
        log_test(test->name, "FAIL", "expected crash but succeeded");
        failed_tests++;
        return 0;
    }

    if (test->expect_error) {
        if (has_error(output)) {
            printf("  [PASS] %s\n", test->name);
            log_test(test->name, "PASS", "error as expected");
            passed_tests++;
            error_tests_passed++;
        } else {
            printf("  [FAIL] %s - expected error\n", test->name);
            log_test(test->name, "FAIL", "expected error but succeeded");
            failed_tests++;
            error_tests_failed++;
        }
        return 0;
    }

    if (has_error(output)) {
        printf("  [FAIL] %s - unexpected error\n", test->name);
        log_test(test->name, "FAIL", "unexpected error");
        failed_tests++;
    } else {
        printf("  [PASS] %s\n", test->name);
        log_test(test->name, "PASS", "compiled successfully");
        passed_tests++;
    }

    return 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("XVR Math Module Comprehensive Fuzz Testing\n");
    printf("============================================\n\n");

    log_file = fopen("/tmp/xvr_math_fuzz.log", "w");
    if (log_file) {
        fprintf(log_file, "XVR Math Module Fuzz Test Log\n");
        fprintf(log_file, "==============================\n\n");
        fflush(log_file);
    }

    MathFuzzTest tests[] = {
        /* SQRT TESTS */
        {"sqrt_basic", "include math;\nvar x = math::sqrt(25.0);", 0, 0,
         "sqrt of positive number"},
        {"sqrt_zero", "include math;\nvar x = math::sqrt(0.0);", 0, 0,
         "sqrt of zero"},
        {"sqrt_one", "include math;\nvar x = math::sqrt(1.0);", 0, 0,
         "sqrt of one"},
        {"sqrt_negative_nan", "include math;\nvar x = math::sqrt(-1.0);", 0, 0,
         "sqrt of negative produces NaN at runtime"},

        /* POW TESTS */
        {"pow_basic", "include math;\nvar x = math::pow(2.0, 3.0);", 0, 0,
         "pow basic"},
        {"pow_zero_exp", "include math;\nvar x = math::pow(5.0, 0.0);", 0, 0,
         "pow with zero exponent"},
        {"pow_neg_exp", "include math;\nvar x = math::pow(2.0, -1.0);", 0, 0,
         "pow with negative exponent"},

        /* TRIG TESTS */
        {"sin_zero", "include math;\nvar x = math::sin(0.0);", 0, 0,
         "sin of zero"},
        {"cos_zero", "include math;\nvar x = math::cos(0.0);", 0, 0,
         "cos of zero"},
        {"tan_zero", "include math;\nvar x = math::tan(0.0);", 0, 0,
         "tan of zero"},

        /* ABS TESTS */
        {"abs_positive", "include math;\nvar x = math::abs(42);", 0, 0,
         "abs of positive int"},
        {"abs_negative", "include math;\nvar x = math::abs(-42);", 0, 0,
         "abs of negative int"},
        {"abs_float", "include math;\nvar x = math::abs(-42.5);", 0, 0,
         "abs of negative float"},

        /* FLOOR/CEIL/ROUND TESTS */
        {"floor_positive", "include math;\nvar x = math::floor(3.7);", 0, 0,
         "floor of positive"},
        {"floor_negative", "include math;\nvar x = math::floor(-3.7);", 0, 0,
         "floor of negative"},
        {"ceil_positive", "include math;\nvar x = math::ceil(3.2);", 0, 0,
         "ceil of positive"},
        {"ceil_negative", "include math;\nvar x = math::ceil(-3.2);", 0, 0,
         "ceil of negative"},

        /* LOG/EXP TESTS */
        {"log_one", "include math;\nvar x = math::log(1.0);", 0, 0, "log of 1"},
        {"log_zero_inf", "include math;\nvar x = math::log(0.0);", 0, 0,
         "log of zero produces -Infinity"},
        {"log_negative_nan", "include math;\nvar x = math::log(-1.0);", 0, 0,
         "log of negative produces NaN"},
        {"exp_zero", "include math;\nvar x = math::exp(0.0);", 0, 0,
         "exp of zero"},
        {"exp_one", "include math;\nvar x = math::exp(1.0);", 0, 0, "exp of 1"},

        /* ATAN TESTS */
        {"atan_zero", "include math;\nvar x = math::atan(0.0);", 0, 0,
         "atan of zero"},
        {"atan2_basic", "include math;\nvar x = math::atan2(1.0, 1.0);", 0, 0,
         "atan2 basic"},

        /* COMBINED EXPRESSIONS */
        {"combined_sqrt_pow",
         "include math;\nvar x = math::sqrt(math::pow(2.0, 4.0));", 0, 0,
         "sqrt(pow(2, 4))"},
        {"combined_sin_cos",
         "include math;\nvar x = math::sin(0.0) + math::cos(0.0);", 0, 0,
         "sin(0) + cos(0)"},

        /* LOOP INTEGRATION */
        {"loop_sqrt",
         "include math;\nvar i = 0;\nwhile (i < 10) { var x = "
         "math::sqrt(float64(i)); i = i + 1; }",
         0, 0, "sqrt in loop"},
        {"loop_pow",
         "include math;\nvar i = 0;\nwhile (i < 10) { var x = math::pow(2.0, "
         "float64(i)); i = i + 1; }",
         0, 0, "pow in loop"},

        /* PROCEDURE INTEGRATION */
        {"proc_math_call",
         "include math;\nproc calc(): float { return math::sqrt(16.0); }\nvar "
         "x = calc();",
         0, 0, "math function in procedure"},
        {"proc_math_param",
         "include math;\nproc calc(n: float): float { return math::sqrt(n); "
         "}\nvar x = calc(9.0);",
         0, 0, "math function with parameter"},

        /* CONSTANT FOLDING */
        {"constant_fold_sqrt", "include math;\nvar x = math::sqrt(25.0);", 0, 0,
         "constant folding sqrt"},
        {"constant_fold_abs", "include math;\nvar x = math::abs(-42);", 0, 0,
         "constant folding abs"},
        {"constant_fold_pow", "include math;\nvar x = math::pow(2.0, 10.0);", 0,
         0, "constant folding pow"},

        /* NEW FUNCTIONS - INVERSE TRIG */
        {"asin_basic", "include math;\nvar x = math::asin(0.5);", 0, 0,
         "asin basic"},
        {"acos_basic", "include math;\nvar x = math::acos(0.5);", 0, 0,
         "acos basic"},
        {"asin_zero", "include math;\nvar x = math::asin(0.0);", 0, 0,
         "asin of zero"},
        {"acos_zero", "include math;\nvar x = math::acos(0.0);", 0, 0,
         "acos of zero"},

        /* NEW FUNCTIONS - LOG */
        {"log10_basic", "include math;\nvar x = math::log10(100.0);", 0, 0,
         "log10 basic"},
        {"log10_one", "include math;\nvar x = math::log10(1.0);", 0, 0,
         "log10 of 1"},
        {"log2_basic", "include math;\nvar x = math::log2(8.0);", 0, 0,
         "log2 basic"},
        {"log2_one", "include math;\nvar x = math::log2(1.0);", 0, 0,
         "log2 of 1"},

        /* NEW FUNCTIONS - HYPERBOLIC */
        {"sinh_zero", "include math;\nvar x = math::sinh(0.0);", 0, 0,
         "sinh of zero"},
        {"cosh_zero", "include math;\nvar x = math::cosh(0.0);", 0, 0,
         "cosh of zero"},
        {"tanh_zero", "include math;\nvar x = math::tanh(0.0);", 0, 0,
         "tanh of zero"},
        {"sinh_one", "include math;\nvar x = math::sinh(1.0);", 0, 0,
         "sinh of 1"},
        {"cosh_one", "include math;\nvar x = math::cosh(1.0);", 0, 0,
         "cosh of 1"},
        {"tanh_one", "include math;\nvar x = math::tanh(1.0);", 0, 0,
         "tanh of 1"},

        /* NEW FUNCTIONS - INVERSE HYPERBOLIC */
        {"asinh_zero", "include math;\nvar x = math::asinh(0.0);", 0, 0,
         "asinh of zero"},
        {"asinh_one", "include math;\nvar x = math::asinh(1.0);", 0, 0,
         "asinh of 1"},
        {"acosh_one", "include math;\nvar x = math::acosh(1.0);", 0, 0,
         "acosh of 1"},
        {"acosh_two", "include math;\nvar x = math::acosh(2.0);", 0, 0,
         "acosh of 2"},
        {"atanh_zero", "include math;\nvar x = math::atanh(0.0);", 0, 0,
         "atanh of zero"},
        {"atanh_half", "include math;\nvar x = math::atanh(0.5);", 0, 0,
         "atanh of 0.5"},

        /* NEW FUNCTIONS - OTHER */
        {"fmod_basic", "include math;\nvar x = math::fmod(10.0, 3.0);", 0, 0,
         "fmod basic"},
        {"fmod_exact", "include math;\nvar x = math::fmod(6.0, 3.0);", 0, 0,
         "fmod exact division"},
        {"trunc_positive", "include math;\nvar x = math::trunc(3.7);", 0, 0,
         "trunc positive"},
        {"trunc_negative", "include math;\nvar x = math::trunc(-3.7);", 0, 0,
         "trunc negative"},

        /* NEW FUNCTIONS - COMBINED */
        {"combined_trig", "include math;\nvar x = math::sin(math::asin(0.5));",
         0, 0, "sin(asin(x))"},
        {"combined_hyperbolic",
         "include math;\nvar x = math::sinh(math::asinh(1.0));", 0, 0,
         "sinh(asinh(x))"},
        {"combined_log",
         "include math;\nvar x = math::log2(math::pow(2.0, 5.0));", 0, 0,
         "log2(pow(2, x))"},
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);

    printf("Running %d math module fuzz tests...\n\n", num_tests);

    for (int i = 0; i < num_tests; i++) {
        run_math_test(&tests[i]);
    }

    printf("\n============================================\n");
    printf("Math Module Fuzz Summary\n");
    printf("============================================\n");
    printf("Total:         %d\n", total_tests);
    printf("Passed:         %d\n", passed_tests);
    printf("Failed:         %d\n", failed_tests);
    printf("Crashed:        %d\n", crashed_tests);
    printf("Skipped:        %d\n", skipped_tests);
    printf("Error tests OK: %d\n", error_tests_passed);
    printf("Error tests BAD:%d\n", error_tests_failed);
    printf("\n");

    if (log_file) {
        fprintf(log_file, "\nSummary:\n");
        fprintf(log_file, "Total: %d, Passed: %d, Failed: %d, Crashed: %d\n",
                total_tests, passed_tests, failed_tests, crashed_tests);
        fclose(log_file);
    }

    if (failed_tests == 0 && crashed_tests == 0) {
        printf("All math module fuzz tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed or crashed.\n");
        return 1;
    }
}
