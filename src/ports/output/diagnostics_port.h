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

#ifndef XVR_DIAGNOSTICS_PORT_H
#define XVR_DIAGNOSTICS_PORT_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    XVR_DIAG_ERROR,
    XVR_DIAG_WARNING,
    XVR_DIAG_NOTE
} Xvr_DiagnosticType;

typedef struct Xvr_Diagnostic {
    Xvr_DiagnosticType type;
    const char* message;
    const char* file;
    int line;
    int column;
} Xvr_Diagnostic;

typedef struct Xvr_DiagnosticList {
    Xvr_Diagnostic* diagnostics;
    size_t count;
    size_t capacity;
} Xvr_DiagnosticList;

typedef struct Xvr_DiagnosticsPort Xvr_DiagnosticsPort;
typedef struct Xvr_DiagnosticsPortVTable {
    void (*add_diagnostic)(Xvr_DiagnosticsPort*, Xvr_Diagnostic);
    void (*clear)(Xvr_DiagnosticsPort*);
    bool (*has_errors)(const Xvr_DiagnosticsPort*);
    Xvr_DiagnosticList (*get_all)(const Xvr_DiagnosticsPort*);
} Xvr_DiagnosticsPortVTable;

struct Xvr_DiagnosticsPort {
    const Xvr_DiagnosticsPortVTable* vtable;
};

Xvr_DiagnosticList Xvr_DiagnosticListCreate(void);
void Xvr_DiagnosticListDestroy(Xvr_DiagnosticList* list);

#endif
