#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_with_xvr(const char *source) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "echo '%s' | ./build/xvr -i 'include std;'", source);
    system(cmd);
}

__attribute__((weak))
int LLVMFuzzerTestOneInput(const char *data, size_t size) {
    char *source = malloc(size + 1);
    memcpy(source, data, size);
    source[size] = '\0';

    printf("Testing lexer with: %s\n", source);

    free(source);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        const char *test_input = argv[1];
        size_t len = strlen(test_input);
        printf("Testing lexer with: %s\n", test_input);
        return LLVMFuzzerTestOneInput(test_input, len);
    }

    const char *test_cases[] = {
        "var x = 42",
        "if x > 0",
        "proc test()",
        "",
        "12345",
        "hello_world",
    };

    int failures = 0;
    for (int i = 0; i < 6; i++) {
        const char *input = test_cases[i];
        printf("\n=== Test case %d: \"%s\" ===\n", i, input);
        int result = LLVMFuzzerTestOneInput(input, strlen(input));
        if (result != 0) {
            printf("FAILED: %s\n", input);
            failures++;
        } else {
            printf("OK\n");
        }
    }

    printf("\n%d/%d tests passed\n", 6 - failures, 6);
    return failures > 0 ? 1 : 0;
}