#ifdef XVR_FUZZER_LIBFUZzer
#    define _GNU_SOURCE
#    include <stddef.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

extern int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
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

    if (written != (ssize_t)size && written != -1) {
        unlink(template);
        return 0;
    }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "./out/xvr %s 2>&1 >/dev/null", template);

    int status = system(cmd);
    unlink(template);

    if (status != 0 && status != 256) {
    }

    return 0;
}
#else

#    include <fcntl.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open input file");
        return 1;
    }

    char buffer[1024 * 1024];
    ssize_t total = 0;
    ssize_t n;

    while ((n = read(fd, buffer + total, sizeof(buffer) - total - 1)) > 0) {
        total += n;
        if (total >= sizeof(buffer) - 1) {
            break;
        }
    }
    close(fd);

    buffer[total] = '\0';

    char template[] = "/tmp/xvr_fuzz_XXXXXX";
    int outfd = mkstemp(template);
    if (outfd == -1) {
        return 1;
    }
    write(outfd, buffer, total);
    close(outfd);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "./out/xvr %s 2>&1 >/dev/null", template);

    int status = system(cmd);
    unlink(template);

    if (status != 0 && status != 256) {
    }

    return 0;
}
#endif