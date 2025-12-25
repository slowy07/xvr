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
#include <string.h>

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

void Xvr_initCommandLine(int argc, const char* argv[]) {
    // default values
    Xvr_commandLine.error = false;
    Xvr_commandLine.help = false;
    Xvr_commandLine.version = false;
    Xvr_commandLine.binaryFile = NULL;
    Xvr_commandLine.sourceFile = NULL;
    Xvr_commandLine.compileFile = NULL;
    Xvr_commandLine.outFile = "out.xb";
    Xvr_commandLine.source = NULL;
    Xvr_commandLine.verbose = false;

    for (int i = 1; i < argc; i++) {  // start at 1 to skip the program name
        Xvr_commandLine.error =
            true;  // error state by default, set to false by successful flags

        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            Xvr_commandLine.help = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
            Xvr_commandLine.version = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
            Xvr_commandLine.verbose = true;
            Xvr_commandLine.error = false;
            continue;
        }

        if (argv[i][0] != '-') {  // if argv not start with '-' its a file
            Xvr_commandLine.sourceFile = (char*)argv[i];
            i++;
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

        if ((!strcmp(argv[i], "-c") || !strcmp(argv[i], "--compile")) &&
            i + 1 < argc) {
            Xvr_commandLine.compileFile = (char*)argv[i + 1];
            i++;
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

        // option without a flag + ending in .tb = binary input
        if (i < argc) {
            if (strncmp(&(argv[i][strlen(argv[i]) - 3]), ".xb", 3) == 0) {
                Xvr_commandLine.binaryFile = (char*)argv[i];
                Xvr_commandLine.error = false;
                continue;
            }
        }

        // don't keep reading in an error state
        return;
    }
}

void Xvr_usageCommandLine(int argc, const char* argv[]) {
    printf(
        "usage: %s [file.xb | -h | -v | [-d] | -i source | -c  file "
        "[-o outfile]]]\n\n",
        argv[0]);
}

void Xvr_helpCommandLine(int argc, const char* argv[]) {
    Xvr_usageCommandLine(argc, argv);

    printf(
        "file.xb\t\tBinary input file in xb format, must be version "
        "%d.%d.%d\n\n",
        XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_PATCH);

    printf("-h\t\t --help\t\tShow this help\n");
    printf("-v\t\t --verrsion\t\tShow version and information\n");
    printf("-d\t\t --debug\t\tBe versbose when operating\n");

    printf(
        "-i\t\t --input source\t\tParse, compile and execute the given string "
        "of source code\n");

    printf(
        "-c\t\t --compile filename\t\tParse and compile the specified source "
        "file into output file\n");

    printf(
        "-o\t\t --output outfile\t\tName of the output file built with "
        "--compile (default output.xb)\n");
}

void Xvr_copyrightCommandLine(int argc, const char* argv[]) {
    printf("The Xvr Programming Language\n");
    printf("Interpreter version %d.%d.%d (built date %s)\n\n",
           XVR_VERSION_MAJOR, XVR_VERSION_MINOR, XVR_VERSION_MINOR,
           XVR_VERSION_BUILD);
    printf("Copyright (c) Arfy Slowy - MIT License\n");
}

#endif /* ifndef XVR_EXPORT */
