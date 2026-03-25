#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0 || size > 1024 * 1024) {
        return 0;
    }

    char template[] = "/tmp/xvr_fuzz_XXXXXX";
    int fd = mkstemp(template);
    if (fd == -1) {
        return 0;
    }

    ssize_t written = write(fd, data, size);
    close(fd);

    if (written == -1 || (size_t)written != size) {
        unlink(template);
        return 0;
    }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "../out/xvr %s 2>&1 >/dev/null", template);

    int status = system(cmd);
    unlink(template);

    return 0;
}