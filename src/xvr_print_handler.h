/**
MIT License

Copyright (c) 2025 arfy slowy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef XVR_PRINT_HANDLER_H
#define XVR_PRINT_HANDLER_H

#include <stdbool.h>
#include <stdio.h>

/**
 * @typedef Xvr_PrintOutputFn
 * @brief Function pointer type for print output operations
 *
 * This is the core abstraction for output destinations. Any function matching
 * this signature can be used as an output handler. Common examples include:
 * - printf-style functions (printf, fprintf)
 * - Custom logging functions
 * - Network send functions
 * - GUI display functions
 *
 * @param message Null-terminated string to output
 *
 * @note The function should NOT append its own newline - that is controlled
 *       by the enableNewline field in Xvr_PrintHandler
 */
typedef void (*Xvr_PrintOutputFn)(const char*);

/**
 * @struct Xvr_PrintHandler
 * @brief Encapsulates print output configuration
 *
 * This struct holds both the output function and formatting preferences.
 * It provides a single unit that can be passed around and configured,
 * making it easy to share output configuration between components.
 *
 * output:
 *     Function pointer that performs the actual output. This is called
 *     with the complete formatted string (including newline if enabled).
 *     Can be any function that matches Xvr_PrintOutputFn signature.
 *
 * enableNewline:
 *     Boolean flag controlling whether a newline is appended to output.
 *     - true:  Append "\n" to every message before calling output
 *     - false: Pass message directly to output without modification
 */
typedef struct {
    Xvr_PrintOutputFn output;
    bool enableNewline;
} Xvr_PrintHandler;

/**
 * @brief Initializes a print handler with specified output function and newline
 * behavior
 *
 * This is the primary initialization function for Xvr_PrintHandler.
 * It sets both the output function and newline preference in one call.
 *
 * @param[out] handler      Pointer to handler to initialize (must not be NULL)
 * @param[in]  output      Function to call for output (must not be NULL)
 * @param[in]  enableNewline  true to append newline, false to pass through
 * as-is
 *
 * @note If output is NULL, Xvr_printHandlerPrint will do nothing.
 *       This allows for silent/disabled handlers.
 */
void Xvr_printHandlerInit(Xvr_PrintHandler* handler, Xvr_PrintOutputFn output,
                          bool enableNewline);

/**
 * @brief Changes the output function of an existing handler
 *
 * Use this to change where output goes without changing newline behavior.
 * This is useful for redirecting output at runtime.
 *
 * @param[in,out] handler  Handler to modify (must not be NULL)
 * @param[in]     output  New output function (must not be NULL)
 *
 * @note The enableNewline setting is preserved when changing output
 */
void Xvr_printHandlerSetOutput(Xvr_PrintHandler* handler,
                               Xvr_PrintOutputFn output);

/**
 * @brief Changes the newline behavior of an existing handler
 *
 * Use this to enable or disable newlines without changing the output function.
 * This allows dynamic control over formatting (e.g., based on command-line
 * flags).
 *
 * @param[in,out] handler      Handler to modify (must not be NULL)
 * @param[in]     enableNewline  true to append newline, false to pass through
 * as-is
 *
 * @note The output function is preserved when changing newline setting
 */
void Xvr_printHandlerSetNewline(Xvr_PrintHandler* handler, bool enableNewline);

/**
 * @brief Outputs a message through the handler
 *
 * This is the main function for producing output through a handler.
 * It applies the newline setting and calls the output function.
 *
 * @param[in] handler  Handler to use for output (must not be NULL)
 * @param[in] message  Null-terminated string to output
 *
 * BEHAVIOR
 * --------
 * - If handler->output is NULL, this function does nothing
 * - If handler->enableNewline is true, appends "\n" to message
 * - Calls handler->output with the formatted string
 *
 * @note This function uses an internal buffer (1024 bytes) for newline
 *       formatting. Messages longer than this will be truncated.
 */
void Xvr_printHandlerPrint(const Xvr_PrintHandler* handler,
                           const char* message);

/**
 * @brief Predefined output function for stdout
 *
 * Use this as the output function when you want print output to go to
 * standard output (console). This is the default for print statements.
 *
 * @param message Null-terminated string to output to stdout
 *
 * @note Does NOT append a newline - that is handled by Xvr_printHandlerPrint
 *       based on the enableNewline setting
 */
void Xvr_printHandlerStdout(const char* message);

/**
 * @brief Predefined output function for stderr
 *
 * Use this as the output function when you want output to go to
 * standard error. This is useful for error handlers and assertion handlers.
 *
 * @param message Null-terminated string to output to stderr
 *
 * @note Does NOT append a newline - that is handled by Xvr_printHandlerPrint
 *       based on the enableNewline setting
 */
void Xvr_printHandlerStderr(const char* message);

/**
 * @brief Predefined output function for file output
 *
 * Use this as the output function when you want output to go to a specific
 * file. This is useful for logging to files or capturing output.
 *
 * @param file   FILE pointer to write to (e.g., from fopen)
 * @param message Null-terminated string to output
 *
 * @note The file pointer must remain valid for the lifetime of the handler
 *       or until the output function is changed
 *
 */
void Xvr_printHandlerFile(FILE* file, const char* message);

#endif  // XVR_PRINT_HANDLER_H
