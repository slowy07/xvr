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

#include "xvr_llvm_target.h"

#include <stdlib.h>
#include <string.h>

static char* Xvr_private_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

struct Xvr_LLVMTargetConfig {
    char* triple;
    char* cpu;
    char* features;
    char* reloc;
    char* code_model;
};

Xvr_LLVMTargetConfig* Xvr_LLVMTargetConfigCreate(void) {
    Xvr_LLVMTargetConfig* config = calloc(1, sizeof(Xvr_LLVMTargetConfig));
    if (!config) {
        return NULL;
    }
    return config;
}

void Xvr_LLVMTargetConfigDestroy(Xvr_LLVMTargetConfig* config) {
    if (!config) {
        return;
    }
    free(config->triple);
    free(config->cpu);
    free(config->features);
    free(config->reloc);
    free(config->code_model);
    free(config);
}

static bool set_string_field(char** field, const char* value) {
    if (!field || !value) {
        return false;
    }
    free(*field);
    *field = Xvr_private_strdup(value);
    return (*field != NULL);
}

bool Xvr_LLVMTargetConfigSetTriple(Xvr_LLVMTargetConfig* config,
                                   const char* triple) {
    return set_string_field(&config->triple, triple);
}

bool Xvr_LLVMTargetConfigSetCPU(Xvr_LLVMTargetConfig* config, const char* cpu) {
    return set_string_field(&config->cpu, cpu);
}

bool Xvr_LLVMTargetConfigSetFeatures(Xvr_LLVMTargetConfig* config,
                                     const char* features) {
    return set_string_field(&config->features, features);
}

bool Xvr_LLVMTargetConfigSetReloc(Xvr_LLVMTargetConfig* config,
                                  const char* reloc) {
    return set_string_field(&config->reloc, reloc);
}

bool Xvr_LLVMTargetConfigSetCodeModel(Xvr_LLVMTargetConfig* config,
                                      const char* model) {
    return set_string_field(&config->code_model, model);
}

struct Xvr_LLVMTargetMachine {
    Xvr_LLVMTargetConfig* config;
    void* target_machine;
};

Xvr_LLVMTargetMachine* Xvr_LLVMTargetMachineCreate(
    Xvr_LLVMTargetConfig* config) {
    if (!config) {
        return NULL;
    }

    Xvr_LLVMTargetMachine* tm = calloc(1, sizeof(Xvr_LLVMTargetMachine));
    if (!tm) {
        return NULL;
    }

    tm->config = config;
    return tm;
}

void Xvr_LLVMTargetMachineDestroy(Xvr_LLVMTargetMachine* tm) {
    if (!tm) {
        return;
    }
    Xvr_LLVMTargetConfigDestroy(tm->config);
    free(tm);
}

bool Xvr_LLVMTargetMachineEmitToFile(Xvr_LLVMTargetMachine* tm,
                                     Xvr_LLVMModuleManager* module,
                                     const char* filename, int filetype) {
    (void)tm;
    (void)module;
    (void)filename;
    (void)filetype;
    return false;
}

void* Xvr_LLVMTargetMachineEmitToMemory(Xvr_LLVMTargetMachine* tm,
                                        Xvr_LLVMModuleManager* module,
                                        size_t* out_size) {
    (void)tm;
    (void)module;
    (void)out_size;
    return NULL;
}

const char* Xvr_LLVMTargetMachineGetDefaultTargetTriple(void) {
    return "x86_64-pc-linux-gnu";
}

const char* Xvr_LLVMTargetMachineGetDefaultCPU(void) { return "generic"; }
