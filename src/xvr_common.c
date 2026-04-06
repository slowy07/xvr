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

#include "xvr_common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_string_utils.h"

char* Xvr_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

#define STATIC_ASSERT(test_for_true) \
    static_assert((test_for_true), "(" #test_for_true ") failed")

STATIC_ASSERT(sizeof(char) == 1);
STATIC_ASSERT(sizeof(short) == 2);
STATIC_ASSERT(sizeof(int) == 4);
STATIC_ASSERT(sizeof(float) == 4);
STATIC_ASSERT(sizeof(unsigned char) == 1);
STATIC_ASSERT(sizeof(unsigned short) == 2);
STATIC_ASSERT(sizeof(unsigned int) == 4);

#ifndef XVR_EXPORT

Xvr_CommandLine Xvr_commandLine;

Xvr_CommandLine Xvr_commandLine = {.error = false,
                                   .help = false,
                                   .version = false,
                                   .sourceFile = NULL,
                                   .compileFile = NULL,
                                   .outFile = NULL,
                                   .source = NULL,
                                   .initialfile = NULL,
                                   .enablePrintNewline = true,
                                   .verbose = false,
                                   .dumpTokens = false,
                                   .dumpAST = false,
                                   .dumpLLVM = false,
                                   .compileOnly = false,
                                   .compileAndRun = true,
                                   .showTiming = false,
                                   .emitType = NULL,
                                   .asmSyntax = "att",
                                   .optimizationLevel = 0};

void Xvr_initCommandLine(int argc, const char* argv[]) {
    for (int i = 1; i < argc; i++) {  // start at 1 to skip the program name
        Xvr_commandLine.error =
            true;  // error state by default, set to false by successful flags

        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            Xvr_commandLine.help = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "--version")) {
            Xvr_commandLine.version = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
            Xvr_commandLine.verbose = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
            Xvr_commandLine.verbose = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--llvm")) {
            Xvr_commandLine.dumpLLVM = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if ((!strcmp(argv[i], "-i") || !strcmp(argv[i], "--input")) &&
            i + 1 < argc) {
            Xvr_commandLine.source = (char*)argv[i + 1];
            i++;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--compile")) {
            Xvr_commandLine.compileOnly = true;
            if (i + 1 < argc) {
                size_t len = xvr_safe_strlen_bounded(argv[i + 1], 256);
                if (argv[i + 1][0] != '-' && !(len >= 4)) {
                    Xvr_commandLine.outFile = (char*)argv[i + 1];
                    i++;
                }
            }
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "-r")) {
            Xvr_commandLine.compileAndRun = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if ((!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) &&
            i + 1 < argc) {
            Xvr_commandLine.outFile = (char*)argv[i + 1];
            i++;
            Xvr_commandLine.error = false;
            continue;
        }

        if ((!strcmp(argv[i], "-t") || !strcmp(argv[i], "--initial")) &&
            i + 1 < argc) {
            Xvr_commandLine.initialfile = (char*)argv[i + 1];
            i++;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "-n")) {
            Xvr_commandLine.enablePrintNewline = false;
            Xvr_commandLine.error = false;
            continue;
        }

        if ((!strcmp(argv[i], "-e") || !strcmp(argv[i], "--emit")) &&
            i + 1 < argc) {
            Xvr_commandLine.emitType = (char*)argv[i + 1];
            i++;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "--asm-syntax") && i + 1 < argc) {
            Xvr_commandLine.asmSyntax = (char*)argv[i + 1];
            i++;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "-S")) {
            Xvr_commandLine.dumpLLVM = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (xvr_safe_strlen_bounded(argv[i], 256) >= 2 && argv[i][0] == '-' &&
            argv[i][1] == 'O') {
            int optLevel = atoi(argv[i] + 2);
            if (optLevel < 0 || optLevel > 3) {
                fprintf(stderr, "error: optimization level must be 0-3\n");
                Xvr_commandLine.error = true;
                return;
            }
            Xvr_commandLine.optimizationLevel = optLevel;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "--dump-tokens") || !strcmp(argv[i], "-Z")) {
            Xvr_commandLine.dumpTokens = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "--dump-ast")) {
            Xvr_commandLine.dumpAST = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "--timing")) {
            Xvr_commandLine.showTiming = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (i < argc) {
            size_t len = xvr_safe_strlen_bounded(argv[i], 256);
            if (len >= 4) {
                int is_xvr = 1;
                for (int j = 0; j < 4; j++) {
                    if (argv[i][len - 4 + j] != ".xvr"[j]) {
                        is_xvr = 0;
                        break;
                    }
                }
                if (is_xvr) {
                    Xvr_commandLine.sourceFile = (char*)argv[i];
                    Xvr_commandLine.error = false;
                    continue;
                }
            }
        }

        // don't keep reading in an error state
        return;
    }
}

void Xvr_usageCommandLine(int argc, const char* argv[]) {
    (void)argc;
    printf("usage: %s [options] [file.xvr] [-o output]\n\n", argv[0]);
    printf("Try '%s --help' for more information.\n", argv[0]);
}

void Xvr_helpCommandLine(int argc, const char* argv[]) {
    (void)argc;
    (void)argv;

    printf("XVR Programming Language Compiler v%d.%d.%d\n\n", XVR_VERSION_MAJOR,
           XVR_VERSION_MINOR, XVR_VERSION_PATCH);
    printf(
        "Ahead-of-time compiler for .xvr source files to native "
        "executables\n\n");

    printf("USAGE:\n");
    printf(
        "  xvr [flags] <source.xvr>      Compile and run (default: creates "
        "'source' executable)\n");
    printf("  xvr [flags] <source.xvr> -o <output>   Compile to executable\n");
    printf(
        "  xvr [flags] <source.xvr> -c [output]  Compile to object file "
        "(default: source.o)\n");
    printf(
        "  xvr [flags] <source.xvr> --emit asm       Output assembly (default: "
        "source.s)\n");
    printf(
        "  xvr [flags] <source.xvr> --emit llvm-ir  Output LLVM IR (default: "
        "source.ll)\n");
    printf(
        "  xvr [flags] <source.xvr> -l             Output LLVM IR to stdout\n");
    printf("  xvr -i '<code>'                Compile and run inline code\n\n");

    printf("OPTIONS:\n");
    printf("  -h, --help               Display this help message and exit\n");
    printf("  -v, --version            Display version information and exit\n");
    printf("  -o, --output <file>      Output file name\n");
    printf("  -c, --compile [file]     Compile to object file (.o)\n");
    printf("  -S, -l, --llvm           Output LLVM IR to stdout\n");
    printf(
        "  -r                        Compile and immediately run (default)\n");
    printf("  -d, --debug               Enable verbose debug output\n");
    printf("  -v, --verbose             Show detailed compilation info\n");
    printf(
        "  -i, --input <code>       Compile and run inline XVR code string\n");
    printf(
        "  -e, --emit <type>        Emit specific output (llvm-ir, asm, "
        "obj)\n");
    printf("  --asm-syntax <intel|att> Set assembly syntax (default: att)\n");
    printf("  -t, --initial <file>     Set entry source file\n");
    printf(
        "  -n                        Disable trailing newline in print "
        "statements\n");
    printf("  -O<0|1|2|3>              Optimization level (default: -O0)\n");
    printf("  -Z, --dump-tokens        Dump all lexer tokens to stderr\n");
    printf("  --dump-ast               Dump parsed AST to stderr\n");
    printf("  --timing                 Show compilation timing breakdown\n\n");

    printf("OUTPUT TYPES:\n");
    printf("  -e asm                   Emit assembly (.s file)\n");
    printf("  -e llvm-ir              Emit LLVM IR (.ll file)\n");
    printf("  -e obj                  Emit object file (.o file)\n\n");

    printf("DEFAULT OUTPUT FILES:\n");
    printf("  (no flag)               Executable (source name without .xvr)\n");
    printf("  -c                       Object file (.o)\n");
    printf("  --emit asm               Assembly (.s)\n");
    printf("  --emit llvm-ir           LLVM IR (.ll)\n\n");

    printf("OPTIMIZATION LEVELS:\n");
    printf("  -O0                      No optimization (fastest compile)\n");
    printf("  -O1                      Basic optimizations\n");
    printf("  -O2                      Balanced optimizations (default)\n");
    printf("  -O3                      Aggressive optimizations\n\n");

    printf("ARGUMENTS:\n");
    printf(
        "  [flags]                   Optional flags (can be placed before or "
        "after file)\n");
    printf("  <source.xvr>              XVR source file to compile\n");
    printf(
        "  <output>                  Output executable or object file name\n");
    printf("  <code>                     Inline XVR source code string\n\n");

    printf("EXAMPLES:\n");
    printf("  Compile and run a source file:\n");
    printf("    $ xvr hello.xvr\n\n");
    printf("  Compile with optimization (flag before file):\n");
    printf("    $ xvr -O2 hello.xvr\n");
    printf("    $ xvr -O3 -d hello.xvr\n\n");
    printf("  Compile to executable with custom name:\n");
    printf("    $ xvr hello.xvr -o myprogram\n");
    printf("    $ ./myprogram\n\n");
    printf("  Compile to object file for later linking:\n");
    printf("    $ xvr hello.xvr -c hello.o\n\n");
    printf("  Output LLVM IR for inspection:\n");
    printf("    $ xvr hello.xvr -l\n");
    printf("    $ xvr -S hello.xvr\n\n");
    printf("  Verbose compilation with timing:\n");
    printf("    $ xvr -v --timing hello.xvr\n");
    printf("    $ xvr --timing -O2 hello.xvr\n\n");
    printf("  Compile with optimization and verbose:\n");
    printf("    $ xvr -O2 -v hello.xvr -o hello\n\n");
    printf("  Compile inline code and run:\n");
    printf("    $ xvr -i 'print(\"Hello World\")'\n\n");
    printf("  Show compiler version:\n");
    printf("    $ xvr --version\n\n");
    printf("  Dump all tokens for debugging:\n");
    printf("    $ xvr -Z hello.xvr\n");
    printf("    $ xvr --dump-tokens hello.xvr\n\n");
    printf("  Dump AST for debugging:\n");
    printf("    $ xvr --dump-ast hello.xvr\n\n");
    printf("  Show compilation timing:\n");
    printf("    $ xvr --timing hello.xvr\n\n");

    printf("EMIT TYPES:\n");
    printf("  llvm-ir    Emit LLVM IR (human-readable .ll file)\n");
    printf("  asm        Emit native assembly (.s file)\n");
    printf("  obj        Emit object file (.o file)\n\n");

    printf("REPORT BUGS TO:\n");
    printf("  https://github.com/anomalyco/xvrlang/issues\n\n");

    Xvr_copyrightCommandLine(0, NULL);
}

void Xvr_copyrightCommandLine(int argc, const char* argv[]) {
    (void)argc;
    (void)argv;
    printf("The Xvr Programming Language\n");
    printf("Compiler version %d.%d.%d (built date %s)\n\n", XVR_VERSION_MAJOR,
           XVR_VERSION_MINOR, XVR_VERSION_PATCH, XVR_VERSION_BUILD);
    printf("Copyright (c) Arfy Slowy - MIT License\n");
}

#endif /* ifndef XVR_EXPORT */
