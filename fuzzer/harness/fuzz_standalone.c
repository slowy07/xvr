#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

static int test_count = 0;
static int error_count = 0;
static FILE* crash_log = NULL;

void log_crash(const char* input, size_t size, const char* error_type) {
    if (!crash_log) {
        crash_log = fopen("fuzzer/crashes/crash_log.txt", "a");
    }
    if (crash_log) {
        fprintf(crash_log, "=== Crash #%d (%s) ===\n", error_count + 1,
                error_type);
        fprintf(crash_log, "Input (%zu bytes): ", size);
        for (size_t i = 0; i < size && i < 256; i++) {
            fprintf(crash_log, "%02x ", input[i]);
        }
        if (size > 256) fprintf(crash_log, "... (truncated)");
        fprintf(crash_log, "\nText: ");
        for (size_t i = 0; i < size && i < 256; i++) {
            fprintf(crash_log, "%c",
                    (input[i] >= 32 && input[i] < 127) ? input[i] : '.');
        }
        fprintf(crash_log, "\n\n");
        fflush(crash_log);
    }
}

int main(int argc, char** argv) {
    srand(time(NULL));

    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("XVR Fuzzer - Standalone test harness\n");
        printf("Usage: %s [iterations]\n", argv[0]);
        return 0;
    }

    int iterations = 10000;
    if (argc > 1) {
        iterations = atoi(argv[1]);
    }

    printf("Starting fuzzing with %d iterations...\n", iterations);

    for (int i = 0; i < iterations; i++) {
        test_count++;

        size_t size = (rand() % 200) + 1;
        uint8_t* data = (uint8_t*)malloc(size);
        if (!data) continue;

        for (size_t j = 0; j < size; j++) {
            data[j] = (uint8_t)(rand() % 256);
        }

        LLVMFuzzerTestOneInput(data, size);
        free(data);

        if (i % 1000 == 0 && i > 0) {
            printf("Progress: %d/%d (%.1f%%)\n", i, iterations,
                   (float)i / iterations * 100);
        }
    }

    printf("\nFuzzing complete: %d tests, %d errors detected\n", test_count,
           error_count);
    if (crash_log) fclose(crash_log);
    return 0;
}
