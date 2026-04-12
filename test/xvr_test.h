/**
 * XVR Test Framework
 * Simple, clean test framework for XVR compiler
 */

#ifndef XVR_TEST_H
#define XVR_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Color codes for terminal output */
#define XVR_TEST_RESET "\033[0m"
#define XVR_TEST_RED "\033[31m"
#define XVR_TEST_GREEN "\033[32m"
#define XVR_TEST_YELLOW "\033[33m"
#define XVR_TEST_BLUE "\033[34m"
#define XVR_TEST_CYAN "\033[36m"

/* Test result counters - use these in each test file */
typedef struct {
    int passed;
    int failed;
    int total;
    const char* name;
} Xvr_TestSuite;

/* Initialize a test suite */
static inline void xvr_test_suite_init(Xvr_TestSuite* suite, const char* name) {
    if (suite) {
        suite->passed = 0;
        suite->failed = 0;
        suite->total = 0;
        suite->name = name;
    }
}

/* Print test suite header */
static inline void xvr_test_suite_header(const char* name) {
    printf("\n%s[%s]%s ", XVR_TEST_BLUE, name, XVR_TEST_RESET);
    for (size_t i = 0; i < 50 - strlen(name); i++) printf(".");
    printf(" ");
}

/* Print test suite result */
static inline void xvr_test_suite_result(Xvr_TestSuite* suite) {
    if (!suite) return;

    if (suite->failed == 0) {
        printf("%s[PASS]%s\n", XVR_TEST_GREEN, XVR_TEST_RESET);
    } else {
        printf("%s[FAIL]%s\n", XVR_TEST_RED, XVR_TEST_RESET);
    }
}

/* Print test suite result from success flag */
static inline void xvr_test_suite_result_single(int success) {
    if (success) {
        printf("%s[PASS]%s\n", XVR_TEST_GREEN, XVR_TEST_RESET);
    } else {
        printf("%s[FAIL]%s\n", XVR_TEST_RED, XVR_TEST_RESET);
    }
}

/* Assert helper - returns 1 on failure */
static inline int xvr_assert(const char* test_name, int condition,
                             const char* expr_str, const char* file, int line) {
    if (!condition) {
        printf("    %s[FAIL]%s %s\n", XVR_TEST_RED, XVR_TEST_RESET, test_name);
        printf("        at %s:%d\n", file, line);
        printf("        assertion: %s\n", expr_str);
        return 1;
    }
    return 0;
}

/* Assert with message */
static inline int xvr_assert_msg(const char* test_name, int condition,
                                 const char* msg, const char* file, int line) {
    if (!condition) {
        printf("    %s[FAIL]%s %s\n", XVR_TEST_RED, XVR_TEST_RESET, test_name);
        printf("        at %s:%d\n", file, line);
        printf("        %s\n", msg);
        return 1;
    }
    return 0;
}

/* Assert equal integers */
static inline int xvr_assert_eq_int(const char* test_name, int expected,
                                    int actual, const char* file, int line) {
    if (expected != actual) {
        printf("    %s[FAIL]%s %s\n", XVR_TEST_RED, XVR_TEST_RESET, test_name);
        printf("        at %s:%d\n", file, line);
        printf("        expected: %d, actual: %d\n", expected, actual);
        return 1;
    }
    return 0;
}

/* Assert equal strings */
static inline int xvr_assert_eq_str(const char* test_name, const char* expected,
                                    const char* actual, const char* file,
                                    int line) {
    if (expected == NULL && actual == NULL) return 0;
    if (expected == NULL || actual == NULL || strcmp(expected, actual) != 0) {
        printf("    %s[FAIL]%s %s\n", XVR_TEST_RED, XVR_TEST_RESET, test_name);
        printf("        at %s:%d\n", file, line);
        printf("        expected: '%s', actual: '%s'\n",
               expected ? expected : "(null)", actual ? actual : "(null)");
        return 1;
    }
    return 0;
}

/* Assert not null */
static inline int xvr_assert_not_null(const char* test_name, const void* ptr,
                                      const char* file, int line) {
    if (ptr == NULL) {
        printf("    %s[FAIL]%s %s\n", XVR_TEST_RED, XVR_TEST_RESET, test_name);
        printf("        at %s:%d\n", file, line);
        printf("        expected non-null pointer\n");
        return 1;
    }
    return 0;
}

/* Assert null */
static inline int xvr_assert_null(const char* test_name, const void* ptr,
                                  const char* file, int line) {
    if (ptr != NULL) {
        printf("    %s[FAIL]%s %s\n", XVR_TEST_RED, XVR_TEST_RESET, test_name);
        printf("        at %s:%d\n", file, line);
        printf("        expected null pointer, got: %p\n", ptr);
        return 1;
    }
    return 0;
}

/* Test case macro - use in test functions */
#define XVR_TEST(suite, name, test_fn)                                 \
    do {                                                               \
        printf("    %s", name);                                        \
        for (size_t _i = 0; _i < 40 - strlen(name); _i++) printf(" "); \
        fflush(stdout);                                                \
        int _result = test_fn();                                       \
        if (_result == 0) {                                            \
            printf("%s[PASS]%s\n", XVR_TEST_GREEN, XVR_TEST_RESET);    \
            (suite)->passed++;                                         \
        } else {                                                       \
            (suite)->failed++;                                         \
        }                                                              \
        (suite)->total++;                                              \
    } while (0)

/* Simple test function signature */
typedef int (*Xvr_TestFn)(void);

/* Run a test function */
#define XVR_RUN_TEST(name, test_fn)                                    \
    do {                                                               \
        printf("    %s", name);                                        \
        for (size_t _i = 0; _i < 40 - strlen(name); _i++) printf(" "); \
        fflush(stdout);                                                \
        int _result = test_fn();                                       \
        if (_result == 0) {                                            \
            printf("%s[PASS]%s\n", XVR_TEST_GREEN, XVR_TEST_RESET);    \
        } else {                                                       \
            printf("%s[FAIL]%s\n", XVR_TEST_RED, XVR_TEST_RESET);      \
        }                                                              \
    } while (0)

/* Main test runner macro */
#define XVR_TEST_MAIN(suites, num_suites)                                    \
    do {                                                                     \
        int total_passed = 0;                                                \
        int total_failed = 0;                                                \
        int total_tests = 0;                                                 \
                                                                             \
        printf("\n");                                                        \
        printf("%s===============================================%s\n",      \
               XVR_TEST_BLUE, XVR_TEST_RESET);                               \
        printf("%s  XVR Compiler Test Suite%s\n", XVR_TEST_BLUE,             \
               XVR_TEST_RESET);                                              \
        printf("%s===============================================%s\n",      \
               XVR_TEST_BLUE, XVR_TEST_RESET);                               \
        printf("\n");                                                        \
                                                                             \
        for (int _i = 0; _i < (num_suites); _i++) {                          \
            xvr_test_suite_header((suites)[_i].name);                        \
            xvr_test_suite_result(&(suites)[_i]);                            \
            total_passed += (suites)[_i].passed;                             \
            total_failed += (suites)[_i].failed;                             \
            total_tests += (suites)[_i].total;                               \
        }                                                                    \
                                                                             \
        printf("\n");                                                        \
        printf("%s-------------------------------------------%s\n",          \
               XVR_TEST_BLUE, XVR_TEST_RESET);                               \
        printf("  Total:  %d tests\n", total_tests);                         \
        printf("  %sPassed:%s %d\n", XVR_TEST_GREEN, XVR_TEST_RESET,         \
               total_passed);                                                \
        if (total_failed > 0) {                                              \
            printf("  %sFailed:%s %d\n", XVR_TEST_RED, XVR_TEST_RESET,       \
                   total_failed);                                            \
        }                                                                    \
        printf("%s-------------------------------------------%s\n",          \
               XVR_TEST_BLUE, XVR_TEST_RESET);                               \
        printf("\n");                                                        \
                                                                             \
        if (total_failed == 0) {                                             \
            printf("%s  ALL TESTS PASSED!%s\n\n", XVR_TEST_GREEN,            \
                   XVR_TEST_RESET);                                          \
            return 0;                                                        \
        } else {                                                             \
            printf("%s  %d TESTS FAILED!%s\n\n", XVR_TEST_RED, total_failed, \
                   XVR_TEST_RESET);                                          \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* XVR_TEST_H */
