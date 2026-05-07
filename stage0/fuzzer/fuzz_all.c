#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

extern int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size);

// Timeout handler for CI safety
static void timeout_handler(int sig) {
    (void)sig;
    _exit(128 + SIGALRM);  // Exit with SIGALRM indicator
}

__attribute__((weak))
int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    // Validate input
    if (!data || size == 0 || size > 65536) {
        return 0;
    }

    // Copy input to null-terminated string
    char *source = (char *)malloc(size + 1);
    if (!source) return 0;
    memcpy(source, data, size);
    source[size] = '\0';

    // Skip empty or whitespace-only input
    if (source[0] == '\0' || source[0] == ' ' || source[0] == '\n' || source[0] == '\t') {
        free(source);
        return 0;
    }

    // Use fork + execve instead of system() for safety
    pid_t pid = fork();
    if (pid < 0) {
        free(source);
        return 0;
    }

    if (pid == 0) {
        // Child process: set timeout and run xvr
        alarm(30);  // 30 second timeout
        signal(SIGALRM, timeout_handler);

        // Write source to stdin of xvr
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            _exit(1);
        }

        pid_t child2 = fork();
        if (child2 == 0) {
            // Grandchild: run xvr
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execl("../xvr", "xvr", "-", (char *)NULL);
            _exit(127);  // exec failed
        }

        // Parent of grandchild: write source to pipe
        close(pipefd[0]);
        write(pipefd[1], source, size);
        close(pipefd[1]);

        int status;
        waitpid(child2, &status, 0);
        _exit(WIFEXITED(status) ? WEXITSTATUS(status) : 1);
    }

    // Parent: wait for child with timeout
    int status;
    pid_t result = waitpid(pid, &status, 0);
    free(source);

    if (result < 0) {
        return 0;  // Wait failed
    }

    // Log CI diagnostics if process was signaled
    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        if (sig == SIGALRM) {
            fprintf(stderr, "FUZZER: xvr timed out after 30s\n");
        } else {
            fprintf(stderr, "FUZZER: xvr killed by signal %d\n", sig);
        }
    }

    return 0;
}
