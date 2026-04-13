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

#include "xvr_asm_config.h"

#include <stdlib.h>
#include <string.h>

struct Xvr_AsmConfig {
    Xvr_AsmSyntax syntax;
    bool verbose;
};

Xvr_AsmConfig* Xvr_AsmConfigCreate(void) {
    Xvr_AsmConfig* config = (Xvr_AsmConfig*)calloc(1, sizeof(Xvr_AsmConfig));
    if (config) {
        config->syntax = XVR_ASM_SYNTAX_INTEL;
        config->verbose = false;
    }
    return config;
}

void Xvr_AsmConfigDestroy(Xvr_AsmConfig* config) {
    if (config) {
        free(config);
    }
}

void Xvr_AsmConfigSetSyntax(Xvr_AsmConfig* config, Xvr_AsmSyntax syntax) {
    if (config) {
        config->syntax = syntax;
    }
}

Xvr_AsmSyntax Xvr_AsmConfigGetSyntax(const Xvr_AsmConfig* config) {
    if (!config) {
        return XVR_ASM_SYNTAX_INTEL;
    }
    return config->syntax;
}

void Xvr_AsmConfigSetVerbose(Xvr_AsmConfig* config, bool verbose) {
    if (config) {
        config->verbose = verbose;
    }
}

bool Xvr_AsmConfigIsVerbose(const Xvr_AsmConfig* config) {
    if (!config) {
        return false;
    }
    return config->verbose;
}

const char* Xvr_AsmSyntaxToString(Xvr_AsmSyntax syntax) {
    switch (syntax) {
    case XVR_ASM_SYNTAX_INTEL:
        return "intel";
    case XVR_ASM_SYNTAX_ATT:
        return "att";
    default:
        return "intel";
    }
}

Xvr_AsmSyntax Xvr_AsmSyntaxFromString(const char* str) {
    if (!str) {
        return XVR_ASM_SYNTAX_INTEL;
    }
    if (strcmp(str, "intel") == 0) {
        return XVR_ASM_SYNTAX_INTEL;
    }
    if (strcmp(str, "att") == 0) {
        return XVR_ASM_SYNTAX_ATT;
    }
    return XVR_ASM_SYNTAX_INTEL;
}
