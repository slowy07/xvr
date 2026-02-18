/**
 * @file xvr_print_handler.c
 * @brief Implementation of print handler interface
 *
 * @see xvr_print_handler.h
 */

#include "xvr_print_handler.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Initializes a print handler with specified output function and newline
 * behavior
 *
 * Sets both the output function pointer and newline preference in a single
 * call. This is the preferred way to create a fully-configured handler.
 *
 * @param[out] handler      Pointer to handler structure to initialize
 * @param[in]  output      Function pointer for output destination
 * @param[in]  enableNewline  Boolean flag for newline appending
 *
 * IMPLEMENTATION NOTES
 * -------------------
 * - Direct assignment - no validation performed
 * - Caller must ensure handler and output are valid pointers
 * - Setting output to NULL will cause Xvr_printHandlerPrint to do nothing
 */
void Xvr_printHandlerInit(Xvr_PrintHandler* handler, Xvr_PrintOutputFn output,
                          bool enableNewline) {
    handler->output = output;
    handler->enableNewline = enableNewline;
}

/**
 * @brief Changes only the output function of a handler
 *
 * Modifies the output function while preserving the current newline setting.
 * Useful for redirecting output at runtime without affecting formatting.
 *
 * @param[in,out] handler  Handler to modify
 * @param[in]     output  New output function pointer
 *
 * IMPLEMENTATION NOTES
 * -------------------
 * - Only modifies the output field
 * - enableNewline remains unchanged
 * - Can be used to implement output redirection (e.g., to file or network)
 */
void Xvr_printHandlerSetOutput(Xvr_PrintHandler* handler,
                               Xvr_PrintOutputFn output) {
    handler->output = output;
}

/**
 * @brief Changes only the newline behavior of a handler
 *
 * Modifies the newline setting while preserving the current output function.
 * Useful for runtime changes based on configuration or command-line flags.
 *
 * @param[in,out] handler      Handler to modify
 * @param[in]     enableNewline  New newline preference
 *
 * IMPLEMENTATION NOTES
 * -------------------
 * - Only modifies the enableNewline field
 * - Output function remains unchanged
 * - Common use: toggling based on CLI flags like -n
 */
void Xvr_printHandlerSetNewline(Xvr_PrintHandler* handler, bool enableNewline) {
    handler->enableNewline = enableNewline;
}

/**
 * @brief Outputs a message using the handler's configuration
 *
 * This is the main printing function that applies both the output destination
 * and formatting rules defined in the handler.
 *
 * @param[in] handler  Handler containing output configuration
 * @param[in] message  Null-terminated string to output
 *
 * BEHAVIOR
 * --------
 * 1. NULL check: If handler->output is NULL, returns immediately
 *    (allows disabled handlers without crashing)
 *
 * 2. Newline formatting:
 *    - If enableNewline is true: formats as "message\n" using snprintf
 *    - If enableNewline is false: passes message directly to output
 *
 * 3. Output: Calls the configured output function with (formatted) message
 *
 * IMPLEMENTATION NOTES
 * -------------------
 * - Uses fixed-size buffer (1024 bytes) for newline formatting
 * - Messages longer than 1023 chars will be truncated with null terminator
 * - Thread-unsafe: if handlers are shared across threads, add synchronization
 *
 * DESIGN DECISION
 * ---------------
 * Using snprintf instead of strcat or direct concatenation:
 * - Prevents buffer overflow
 * - Handles edge cases (empty string, very long messages)
 * - Adds null terminator automatically
 * - Slight performance cost acceptable for print output
 */
void Xvr_printHandlerPrint(const Xvr_PrintHandler* handler,
                           const char* message) {
    /* Guard clause: do nothing if no output function configured */
    if (handler->output == NULL) {
        return;
    }

    /* Apply newline formatting if enabled */
    if (handler->enableNewline) {
        /* Fixed-size buffer - truncation possible for very long messages */
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "%s\n", message);
        handler->output(buffer);
    } else {
        /* Pass through without modification */
        handler->output(message);
    }
}

/**
 * @brief Predefined output function for standard output
 *
 * Simple wrapper around printf for stdout without newline.
 * The newline is handled by Xvr_printHandlerPrint based on handler config.
 *
 * @param[in] message Null-terminated string to print to stdout
 *
 * IMPLEMENTATION NOTES
 * -------------------
 * - Uses printf with "%s" format (no extra formatting)
 * - Message should NOT include newline (handler adds if configured)
 * - Equivalent to: printf("%s", message)
 */
void Xvr_printHandlerStdout(const char* message) { printf("%s", message); }

/**
 * @brief Predefined output function for standard error
 *
 * Wrapper around fprintf to stderr. Useful for error and assertion handlers
 * where output should be separated from normal print output.
 *
 * @param[in] message Null-terminated string to print to stderr
 *
 * IMPLEMENTATION NOTES
 * -------------------
 * - Uses fprintf(stderr, ...) for error output
 * - Separates errors from normal stdout output
 * - Allows shell redirection: ./xvr program.xvr 2>errors.log
 */
void Xvr_printHandlerStderr(const char* message) {
    fprintf(stderr, "%s", message);
}

/**
 * @brief Predefined output function for file output
 *
 * Generic file output function that writes to any open FILE pointer.
 * More commonly used through a custom wrapper that captures the FILE*.
 *
 * @param[in] file    FILE pointer obtained from fopen() (must be valid)
 * @param[in] message Null-terminated string to write to file
 *
 * WARNING
 * -------
 * This function signature differs from Xvr_PrintOutputFn because it takes
 * an extra FILE* parameter. To use with Xvr_PrintHandler, create a wrapper:
 *
 * @code
 * typedef struct {
 *     FILE* file;
 * } FileContext;
 *
 * void fileWrapper(const char* msg) {
 *     // Would need context to know which file - use custom wrapper instead
 * }
 * @endcode
 *
 * For file output, it's typically better to create a custom wrapper that
 * captures the FILE* in a context struct or uses a global variable.
 *
 * IMPLEMENTATION NOTES
 * -------------------
 * - Null-check on file prevents crashes with invalid FILE pointers
 * - Does NOT flush - caller should flush if needed
 * - Does NOT close the file - caller retains ownership
 */
void Xvr_printHandlerFile(FILE* file, const char* message) {
    if (file != NULL) {
        fprintf(file, "%s", message);
    }
}
