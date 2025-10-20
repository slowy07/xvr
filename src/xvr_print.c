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

#include "xvr_print.h"

#include <stdio.h>

static void outDefault(const char* msg) { fprintf(stdout, "%s", msg); }

static void errDefault(const char* msg) { fprintf(stderr, "%s", msg); }

static void assertDefault(const char* msg) { fprintf(stderr, "%s\n", msg); }

static Xvr_callbackType printCallback = outDefault;
static Xvr_callbackType errorCallback = errDefault;
static Xvr_callbackType assertCallback = assertDefault;

void Xvr_print(const char* msg) { printCallback(msg); }

void Xvr_error(const char* msg) { assertCallback(msg); }

void Xvr_assertFailure(const char* msg) { assertCallback(msg); }

void Xvr_setPrintCallback(Xvr_callbackType cb) { printCallback = cb; }

void Xvr_setErrorCallback(Xvr_callbackType cb) { errorCallback = cb; }

void Xvr_setAssertFailureCallback(Xvr_callbackType cb) { assertCallback = cb; }

void Xvr_resetPrintCallback(void) { printCallback = outDefault; }

void Xvr_resetErrorCallback(void) { errorCallback = errDefault; }

void Xvr_resetAssertFailureCallback(void) { assertCallback = assertDefault; }
