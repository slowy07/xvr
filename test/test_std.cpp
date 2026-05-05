#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

static const char* get_xvr_path(void) {
    const char* env_path = getenv("XVR_TEST_PATH");
    if (env_path) return env_path;

    // Check if it's a regular file, not a directory
    struct stat st;
    if (stat("./xvr", &st) == 0 && S_ISREG(st.st_mode)) return "./xvr";
    if (stat("../xvr", &st) == 0 && S_ISREG(st.st_mode)) return "../xvr";
    if (stat("build/xvr", &st) == 0 && S_ISREG(st.st_mode)) return "build/xvr";

    return "./xvr";
}

/* 
 * Safe popen replacement with timeout and diagnostics.
 * Uses alarm() to enforce wall-clock timeout, then reports
 * WIFEXITED / WIFSIGNALED explicitly for CI observability.
 */
static int safe_capture_run(const char* source, char* output, size_t output_size) {
    FILE* tmp = fopen("/tmp/xvr_std_test.xvr", "w");
    if (!tmp) return -1;
    fputs(source, tmp);
    fclose(tmp);

    const char* xvr_path = get_xvr_path();
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s /tmp/xvr_std_test.xvr 2>&1", xvr_path);

    /* CI safety: enforce timeout to prevent hanging jobs */
    alarm(30);  /* seconds */

    FILE* proc = popen(cmd, "r");
    if (!proc) {
        alarm(0);
        fprintf(stderr, "[CI] popen failed for path: %s\n", xvr_path);
        unlink("/tmp/xvr_std_test.xvr");
        return -1;
    }

    size_t len = fread(output, 1, output_size - 1, proc);
    output[len] = '\0';

    int status = pclose(proc);
    alarm(0);  /* cancel any pending alarm */

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

    /* CI observability: explicit WIF* diagnostics */
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "[CI] Child killed by signal %d (timeout?)\n",
                WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    if (!WIFEXITED(status)) {
        fprintf(stderr, "[CI] Child did not exit normally, status=%d\n", status);
        return -1;
    }

    return status;
}

/* Legacy wrapper for existing tests */
static int capture_compile_run(const char* source, char* output, size_t output_size) {
    return safe_capture_run(source, output, output_size);
}

TEST_CASE("std::max integer greater", "[std][max]") {
    const char* source = "include std;\nvar r = std::max(10, 20);\nstd::print(\"{}\\n\", r);";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "20") == 0);
}

TEST_CASE("std::max integer less", "[std][max]") {
    const char* source = "include std;\nvar r = std::max(30, 20);\nstd::print(\"{}\\n\", r);";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "30") == 0);
}

TEST_CASE("std::max integer equal", "[std][max]") {
    const char* source = "include std;\nvar r = std::max(5, 5);\nstd::print(\"{}\\n\", r);";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "5") == 0);
}

TEST_CASE("std::max three args", "[std][max]") {
    const char* source = "include std;\nvar r = std::max(3, 1, 2);\nstd::print(\"{}\\n\", r);";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "3") == 0);
}

TEST_CASE("std::print integer", "[std][print]") {
    const char* source = "include std;\nstd::print(\"{}\\n\", 42);";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "42") == 0);
}

TEST_CASE("std::print negative integer", "[std][print]") {
    const char* source = "include std;\nstd::print(\"{}\\n\", -123);";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "-123") == 0);
}

TEST_CASE("std::print float", "[std][print]") {
    const char* source = "include std;\nstd::print(\"{}\\n\", 3.14);";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "3.140000") == 0);
}

TEST_CASE("std::print string", "[std][print]") {
    const char* source = "include std;\nstd::print(\"{}\\n\", \"hello\");";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "hello") == 0);
}

TEST_CASE("std::println basic", "[std][println]") {
    const char* source = "include std;\nstd::println(\"hello\");";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "hello") == 0);
}

TEST_CASE("std::println with variable", "[std][println]") {
    const char* source = "include std;\nvar x = 10;\nstd::println(\"value: {}\", x);";
    char output[4096] = {0};
    
    int status = capture_compile_run(source, output, sizeof(output));
    REQUIRE(status == 0);
    REQUIRE(strcmp(output, "value: 10") == 0);
}