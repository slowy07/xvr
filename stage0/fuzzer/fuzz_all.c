#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size);

__attribute__((weak))
int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    char *source = malloc(size + 1);
    if (!source) return 0;
    memcpy(source, data, size);
    source[size] = '\0';

    if (size == 0 || source[0] == '\0') {
        free(source);
        return 0;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "echo '%s' | ../xvr - 2>&1", source);
    int result = system(cmd);
    (void)result;

    free(source);
    return 0;
}