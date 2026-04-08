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

#ifndef XVR_ASM_CONFIG_H
#define XVR_ASM_CONFIG_H

#include <stdbool.h>

typedef enum Xvr_AsmSyntax {
    XVR_ASM_SYNTAX_INTEL,
    XVR_ASM_SYNTAX_ATT
} Xvr_AsmSyntax;

typedef struct Xvr_AsmConfig Xvr_AsmConfig;

Xvr_AsmConfig* Xvr_AsmConfigCreate(void);
void Xvr_AsmConfigDestroy(Xvr_AsmConfig* config);

void Xvr_AsmConfigSetSyntax(Xvr_AsmConfig* config, Xvr_AsmSyntax syntax);
Xvr_AsmSyntax Xvr_AsmConfigGetSyntax(const Xvr_AsmConfig* config);

void Xvr_AsmConfigSetVerbose(Xvr_AsmConfig* config, bool verbose);
bool Xvr_AsmConfigIsVerbose(const Xvr_AsmConfig* config);

const char* Xvr_AsmSyntaxToString(Xvr_AsmSyntax syntax);
Xvr_AsmSyntax Xvr_AsmSyntaxFromString(const char* str);

#endif
