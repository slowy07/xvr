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

#ifndef XVR_COMMON_H
#define XVR_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @def XVR_API
 * @brief control symbol visibilty / declaration linkage fro the XVR library
 *
 * using to annotate public function and variable that part of the ABI library
 *  - Linux & MacOs: default to `extern` (ELF / Dyld visibilty handle via
 * `-fvisibility`)
 *  - Windows:
 *    - `XVR_EXPORT` : mark symbol for export in DLL (used when buold the libs)
 *    - `XVR_IMPORT` : mark symbol as imported from the DLL (used when link
 * againts the libs)
 *    - else : fallback to `extern` (static linking unspecified)
 *
 * @note on windows `__declspec(dllexport) ` / `__declspec(dllimport)` are
 * require for proper the dll linking on unix-like system, symbol visibilty is
 * typically controlled via compiler flags
 */
#if defined(__linux__)
#    define XVR_API extern
#elif defined(_WIN32) || defined(_WIN64)
#    if defined(XVR_EXPORT)
#        define XVR_API __declspec(dllexport)
#    elif defined(XVR_IMPORT)
#        define XVR_API __declspec(dllimport)
#    else
#        define XVR_API extern
#    endif
#elif defined(__APPLE__)
#    define XVR_API extern
#else
// generic solution
#    define XVR_API extern
#endif

/**
 * @def XVR_BITNESS
 * @brief specifiers target architercture native pointer size (in bits)
 *
 * using for cache-line alignment hints, memry layout decision, or
 * architecture-aware optim value are:
 *    - 32: 32bit arch (x86, ARMv7)
 *    - 64: 64bit arch (x86_64, ARM64, RISC-V64)
 *    - -1: maybe using custom arch
 *
 * logic:
 *  - Linux, MacOs: rely on `__LP64__` (LP64 data model: long and pointer)
 *  - Windows: `_WIN64` implies 64-bit; otherwise 32bit
 *  - NetBSD: XVR_BITNESS
 */
#if defined(__linux__)
#    if defined(__LP64__)
#        define XVR_BITNESS 64
#    else
#        define XVR_BITNESS 32
#    endif
#elif defined(__NetBSD__)
#    if defined(__LP64__)
#        define XVR_BITNESS 64
#    else
#        define XVR_XVR_BITNESS 32
#    endif  //     if defined(__LP64__)
#elif defined(_WIN32) || defined(_WIN64)
#    if defined(_WIN64)
#        define XVR_BITNESS 64
#    else
#        define XVR_BITNESS 32
#    endif
#elif defined(__APPLE__)
#    if defined(__LP64__)
#        define XVR_BITNESS 64
#    else
#        define XVR_BITNESS 32
#    endif
#else
// generic solution
#    define XVR_BITNESS -1
#endif

/**
 * @brief build timestamp (compile-time constant)
 *
 * Format will be: "MMM DD YYYY HH:MM:SS"
 * generate via standard predifined macros
 *
 */
#define XVR_VERSION_BUILD __DATE__ " " __TIME__

#ifndef XVR_EXPORT

/**
 * @struct Xvr_CommandLine
 * @brief parsed command-line stat for the XVR tool
 *
 * all members are populated by `Xvr_initCommandLine()`
 * boolean flags use `true / false (<stdbool.h>)`
 *
 * string filed (char*) are:
 *  - ownerd by the runtime (do not free)
 *  - NULL if not provided
 *  - point into `argv` or internal static buffers (valid for program lifetime)
 *
 * @note Design favors simplicty and zero-copy parsing (no dynamic allocation in
 * parser) for production, consider arena allocation or deep-copying if argv may
 * be mutated or free
 */
typedef struct {
    bool error;
    bool help;
    bool version;
    char* binaryFile;
    char* sourceFile;
    char* compileFile;
    char* outFile;
    char* source;
    bool enablePrintNewline;
    bool verbose;
} Xvr_CommandLine;

/**
 * @var Xvr_commandLine
 * @brief global singleton storing the parsed command-line state
 *
 * initialized by `Xvr_initCommandLine()`
 * use only after `main()` calls `Xvr_initCommandLine(argc, argv)`
 *
 * @warning not thread-safe, this intented for single-threaded CLI tools
 */
extern Xvr_CommandLine Xvr_commandLine;

/**
 * @brief parses command-line arguments and populates `Xvr_commandLine`
 *
 * @param[in] argc argument count (from main)
 * @param[in] argv argument vector (from main)
 *
 * - reset `Xvr_commandLine` to zero-initialized state first
 *   set error = true and prints usage on unknown flags or missing required args
 */
void Xvr_initCommandLine(int argc, const char* argv[]);

/**
 * @brief prints brief usage summary
 *
 * typically called after parsing error on `-h` / `--help`
 * output goes to `stderr`
 */
void Xvr_usageCommandLine(int argc, const char* argv[]);

/**
 * @brief prints copyright notice and license info
 */
void Xvr_copyrightCommandLine(int argc, const char* argv[]);

/**
 * @brief prints full helpm essage (usage + options + example + copyright)
 */
void Xvr_helpCommandLine(int argc, const char* argv[]);

#endif  // !XVR_EXPORT

/**
 * @defgroup version XVR versioning constant
 * @brief Embedded bytecode and runtime version metadata
 */
#define XVR_VERSION_MAJOR 0
#define XVR_VERSION_MINOR 1
#define XVR_VERSION_PATCH 4
#endif  // !XVR_COMMON_H
