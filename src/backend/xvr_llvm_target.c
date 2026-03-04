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

#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    LLVMTargetMachineRef target_machine;
    LLVMTargetRef target;
};

Xvr_LLVMTargetMachine* Xvr_LLVMTargetMachineCreate(
    Xvr_LLVMTargetConfig* config) {
    if (!config) {
        return NULL;
    }

    LLVMTargetRef target;
    char* error = NULL;

    if (!config->triple) {
        LLVMInitializeAllTargetInfos();
        LLVMInitializeAllTargets();
        LLVMInitializeAllTargetMCs();
        LLVMInitializeAllAsmPrinters();
        LLVMInitializeAllAsmParsers();

        const char* defaultTriple = LLVMGetDefaultTargetTriple();
        if (!LLVMGetTargetFromTriple(defaultTriple, &target, &error)) {
            free(error);
            error = NULL;
        } else {
            free(error);
            error = NULL;
            if (!LLVMGetTargetFromTriple("x86_64-pc-linux-gnu", &target,
                                         &error)) {
                free(error);
            } else {
                free(error);
                return NULL;
            }
        }
    } else {
        LLVMInitializeAllTargetInfos();
        LLVMInitializeAllTargets();
        LLVMInitializeAllTargetMCs();
        LLVMInitializeAllAsmPrinters();
        LLVMInitializeAllAsmParsers();

        if (!LLVMGetTargetFromTriple(config->triple, &target, &error)) {
            free(error);
            return NULL;
        }
    }

    const char* cpu = config->cpu ? config->cpu : "generic";
    const char* features = config->features ? config->features : "";
    const char* reloc = config->reloc ? config->reloc : "default";
    const char* model = config->code_model ? config->code_model : "jitdefault";

    LLVMRelocMode reloc_mode = LLVMRelocDefault;
    if (strcmp(reloc, "PIC") == 0) {
        reloc_mode = LLVMRelocPIC;
    } else if (strcmp(reloc, "Static") == 0) {
        reloc_mode = LLVMRelocStatic;
    }

    LLVMCodeModel code_model = LLVMCodeModelJITDefault;
    if (strcmp(model, "small") == 0) {
        code_model = LLVMCodeModelSmall;
    } else if (strcmp(model, "kernel") == 0) {
        code_model = LLVMCodeModelKernel;
    } else if (strcmp(model, "medium") == 0) {
        code_model = LLVMCodeModelMedium;
    } else if (strcmp(model, "large") == 0) {
        code_model = LLVMCodeModelLarge;
    }

    LLVMTargetMachineRef tm = LLVMCreateTargetMachine(
        target, config->triple ? config->triple : "x86_64-pc-linux-gnu", cpu,
        features, LLVMCodeGenLevelDefault, reloc_mode, code_model);

    if (!tm) {
        return NULL;
    }

    Xvr_LLVMTargetMachine* result = calloc(1, sizeof(Xvr_LLVMTargetMachine));
    if (!result) {
        LLVMDisposeTargetMachine(tm);
        return NULL;
    }

    result->config = config;
    result->target_machine = tm;
    result->target = target;

    return result;
}

void Xvr_LLVMTargetMachineDestroy(Xvr_LLVMTargetMachine* tm) {
    if (!tm) {
        return;
    }
    if (tm->target_machine) {
        LLVMDisposeTargetMachine(tm->target_machine);
    }
    Xvr_LLVMTargetConfigDestroy(tm->config);
    free(tm);
}

bool Xvr_LLVMTargetMachineEmitToFile(Xvr_LLVMTargetMachine* tm,
                                     Xvr_LLVMModuleManager* mod_manager,
                                     const char* filename, int filetype) {
    (void)tm;
    (void)filetype;
    if (!mod_manager || !filename) {
        return false;
    }

    LLVMModuleRef mod = Xvr_LLVMModuleManagerGetModule(mod_manager);
    if (!mod) {
        return false;
    }

    LLVMSetTarget(mod, "x86_64-pc-linux-gnu");

    size_t ir_len = 0;
    char* ir = LLVMPrintModuleToString(mod);
    if (!ir) {
        return false;
    }

    FILE* f = fopen("/tmp/xvr_ir.ll", "w");
    if (!f) {
        LLVMDisposeMessage(ir);
        return false;
    }
    fputs(ir, f);
    fclose(f);
    LLVMDisposeMessage(ir);

    char* cmd;
    if (asprintf(&cmd,
                 "clang -target x86_64-pc-linux-gnu -c /tmp/xvr_ir.ll -o %s",
                 filename) == -1) {
        return false;
    }
    int rc = system(cmd);
    free(cmd);

    return rc == 0;
}

void* Xvr_LLVMTargetMachineEmitToMemory(Xvr_LLVMTargetMachine* tm,
                                        Xvr_LLVMModuleManager* module,
                                        size_t* out_size) {
    if (!tm || !module || !out_size) {
        return NULL;
    }

    LLVMModuleRef mod = Xvr_LLVMModuleManagerGetModule(module);
    if (!mod) {
        return NULL;
    }

    LLVMTargetMachineRef machine = tm->target_machine;

    char* error = NULL;
    LLVMMemoryBufferRef buffer = NULL;

    bool success = LLVMTargetMachineEmitToMemoryBuffer(
        machine, mod, LLVMObjectFile, &error, &buffer);

    if (error) {
        free(error);
    }

    if (!success || !buffer) {
        return NULL;
    }

    *out_size = LLVMGetBufferSize(buffer);
    void* data = malloc(*out_size);
    if (data) {
        memcpy(data, LLVMGetBufferStart(buffer), *out_size);
    }

    LLVMDisposeMemoryBuffer(buffer);

    return data;
}

const char* Xvr_LLVMTargetMachineGetDefaultTargetTriple(void) {
    return "x86_64-pc-linux-gnu";
}

const char* Xvr_LLVMTargetMachineGetDefaultCPU(void) { return "generic"; }
