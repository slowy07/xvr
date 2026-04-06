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

#include "xvr_asm_config.h"
#include "xvr_common.h"

typedef enum {
    XVR_EMIT_OBJECT = 0,
    XVR_EMIT_ASM = 1,
    XVR_EMIT_LLVM_IR = 2
} Xvr_EmitType;

Xvr_EmitType Xvr_EmitTypeFromString(const char* type_str) {
    if (!type_str) return XVR_EMIT_OBJECT;
    if (strcmp(type_str, "obj") == 0) return XVR_EMIT_OBJECT;
    if (strcmp(type_str, "asm") == 0) return XVR_EMIT_ASM;
    if (strcmp(type_str, "llvm-ir") == 0) return XVR_EMIT_LLVM_IR;
    return XVR_EMIT_OBJECT;
}

struct Xvr_LLVMTargetConfig {
    char* triple;
    char* cpu;
    char* features;
    char* reloc;
    char* code_model;
    Xvr_AsmSyntax asm_syntax;
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
    *field = Xvr_strdup(value);
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

bool Xvr_LLVMTargetConfigSetAsmSyntax(Xvr_LLVMTargetConfig* config,
                                      Xvr_AsmSyntax syntax) {
    if (!config) return false;
    config->asm_syntax = syntax;
    return true;
}

Xvr_AsmSyntax Xvr_LLVMTargetConfigGetAsmSyntax(
    const Xvr_LLVMTargetConfig* config) {
    if (!config) return XVR_ASM_SYNTAX_INTEL;
    return config->asm_syntax;
}

static bool convertAsmToIntelSyntax(const char* filename);
static char* convertAttToIntel(const char* input);
static char* extractOperandsSimple(const char* start);
static char* reorderOperandsSimple(const char* instr, const char* operands);
static char* convertMemOperandSimple(const char* start);

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
                LLVMDisposeMessage((char*)defaultTriple);
                return NULL;
            }
        }
        LLVMDisposeMessage((char*)defaultTriple);
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
    if (!tm || !mod_manager || !filename) {
        return false;
    }

    LLVMModuleRef mod = Xvr_LLVMModuleManagerGetModule(mod_manager);
    if (!mod) {
        return false;
    }

    Xvr_EmitType emit_type = (Xvr_EmitType)filetype;
    LLVMTargetMachineRef machine = tm->target_machine;

    char* err_msg = NULL;
    LLVMBool result;

    switch (emit_type) {
    case XVR_EMIT_ASM: {
        result = LLVMTargetMachineEmitToFile(machine, mod, filename,
                                             LLVMAssemblyFile, &err_msg);
        break;
    }
    case XVR_EMIT_LLVM_IR: {
        char* ir = LLVMPrintModuleToString(mod);
        if (!ir) {
            return false;
        }
        FILE* f = fopen(filename, "w");
        if (!f) {
            LLVMDisposeMessage(ir);
            return false;
        }
        fputs(ir, f);
        fclose(f);
        LLVMDisposeMessage(ir);
        return true;
    }
    case XVR_EMIT_OBJECT:
    default: {
        result = LLVMTargetMachineEmitToFile(machine, mod, filename,
                                             LLVMObjectFile, &err_msg);
        break;
    }
    }

    if (result) {
        if (err_msg) {
            fprintf(stderr, "error: failed to emit file: %s\n", err_msg);
            LLVMDisposeMessage(err_msg);
        }
        return false;
    }

    if (emit_type == XVR_EMIT_ASM) {
        Xvr_AsmSyntax syntax = Xvr_LLVMTargetConfigGetAsmSyntax(tm->config);
        if (syntax == XVR_ASM_SYNTAX_INTEL) {
            return convertAsmToIntelSyntax(filename);
        }
    }

    return true;
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

LLVMTargetMachineRef Xvr_LLVMTargetMachineGetLLVMTargetMachine(
    Xvr_LLVMTargetMachine* tm) {
    if (!tm) {
        return NULL;
    }
    return tm->target_machine;
}

static bool convertAsmToIntelSyntax(const char* filename) {
    if (!filename) return false;

    FILE* f = fopen(filename, "r");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return false;
    }

    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    char* converted = convertAttToIntel(buffer);
    free(buffer);

    if (!converted) return false;

    f = fopen(filename, "w");
    if (!f) {
        free(converted);
        return false;
    }

    fputs(converted, f);
    fclose(f);
    free(converted);

    return true;
}

static char* convertAttToIntel(const char* input) {
    if (!input) return NULL;

    size_t len = strlen(input);
    char* output = malloc(len * 2 + 1);
    if (!output) return NULL;

    const char* src = input;
    char* dst = output;
    bool in_directive = false;

    while (*src) {
        if (*src == '.') {
            in_directive = true;
            while (*src && *src != '\n') *dst++ = *src++;
            continue;
        }

        if (*src == '\n') {
            in_directive = false;
            *dst++ = *src++;
            continue;
        }

        if (*src == '\t' || *src == ' ') {
            if (in_directive) in_directive = false;
            *dst++ = *src++;
            continue;
        }

        if ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z')) {
            char instr[32] = {0};
            int i = 0;
            while ((*src >= 'a' && *src <= 'z') ||
                   (*src >= 'A' && *src <= 'Z') ||
                   (*src >= '0' && *src <= '9')) {
                if (i < 31) instr[i++] = *src;
                src++;
            }

            for (int j = 0; j < i; j++) *dst++ = instr[j];
            *dst++ = ' ';

            const char* ops_start = src;
            while (*ops_start == ' ' || *ops_start == '\t') ops_start++;

            char* ops = extractOperandsSimple(ops_start);
            if (ops && strlen(ops) > 0) {
                char* reordered = reorderOperandsSimple(instr, ops);
                if (reordered) {
                    size_t rl = strlen(reordered);
                    if (rl > 0) {
                        memcpy(dst, reordered, rl);
                        dst += rl;
                    }
                    free(reordered);
                } else if (ops) {
                    size_t ol = strlen(ops);
                    if (ol > 0) {
                        memcpy(dst, ops, ol);
                        dst += ol;
                    }
                }
                if (ops) free(ops);
            }

            while (*src && *src != '\n') src++;
            continue;
        }

        if (*src == '%') {
            src++;
            while (*src && ((*src >= 'a' && *src <= 'z') ||
                            (*src >= '0' && *src <= '9'))) {
                *dst++ = *src++;
            }
            continue;
        }

        if (*src == '$') {
            src++;
            while (*src && ((*src >= '0' && *src <= '9') ||
                            (*src >= 'a' && *src <= 'f') ||
                            (*src >= 'A' && *src <= 'F'))) {
                *dst++ = *src++;
            }
            continue;
        }

        if (*src == '(') {
            char* mem = convertMemOperandSimple(src);
            if (mem) {
                *dst++ = '[';
                size_t ml = strlen(mem);
                if (ml > 0) {
                    memcpy(dst, mem, ml);
                    dst += ml;
                }
                *dst++ = ']';
                free(mem);
            }
            while (*src && *src != ')') src++;
            if (*src == ')') src++;
            continue;
        }

        *dst++ = *src++;
    }

    *dst = '\0';
    return output;
}

static char* extractOperandsSimple(const char* start) {
    if (!start) return NULL;
    const char* end = start;
    while (*end && *end != '\n') end++;
    while (end > start && (end[-1] == ' ' || end[-1] == '\t')) end--;

    size_t len = end - start;
    if (len == 0) return NULL;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    if (len > 0) {
        memcpy(result, start, len);
    }
    result[len] = '\0';
    return result;
}

static char* reorderOperandsSimple(const char* instr, const char* operands) {
    if (!operands || !instr) return strdup(operands ? operands : "");

    char* copy = strdup(operands);
    if (!copy) return NULL;

    char* parts[8] = {0};
    int count = 0;

    char* token = strtok(copy, ",");
    while (token && count < 8) {
        while (*token == ' ' || *token == '\t') token++;
        size_t tl = strlen(token);
        while (tl > 0 && (token[tl - 1] == ' ' || token[tl - 1] == '\t'))
            token[--tl] = '\0';
        if (strlen(token) > 0) parts[count++] = strdup(token);
        token = strtok(NULL, ",");
    }
    free(copy);

    if (count == 0) return strdup(operands);
    if (count == 1) {
        char* result = strdup(parts[0]);
        for (int i = 0; i < count; i++) free(parts[i]);
        return result;
    }

    char* dst = malloc(512);
    if (!dst) {
        for (int i = 0; i < count; i++) free(parts[i]);
        return strdup(operands);
    }

    int is_single_operand =
        (strcmp(instr, "push") == 0 || strcmp(instr, "pushq") == 0 ||
         strcmp(instr, "pop") == 0 || strcmp(instr, "popq") == 0 ||
         strcmp(instr, "call") == 0 || strcmp(instr, "callq") == 0 ||
         strcmp(instr, "jmp") == 0 || strcmp(instr, "jmpq") == 0 ||
         strcmp(instr, "ret") == 0 || strcmp(instr, "retq") == 0);

    if (is_single_operand) {
        snprintf(dst, 512, "%s", parts[0]);
    } else {
        if (count >= 2) {
            char* first = parts[0];
            char* second = parts[1];
            int first_has_percent = (strchr(first, '%') != NULL);
            int second_has_percent = (strchr(second, '%') != NULL);
            int first_is_label =
                (first[0] == '.' || (first[0] >= 'a' && first[0] <= 'z') ||
                 (first[0] >= 'A' && first[0] <= 'Z'));
            int second_is_label =
                (second[0] == '.' || (second[0] >= 'a' && second[0] <= 'z') ||
                 (second[0] >= 'A' && second[0] <= 'Z'));

            if (first_has_percent && second_has_percent) {
                snprintf(dst, 512, "%s, %s", first, second);
            } else if (first_has_percent && !second_has_percent) {
                snprintf(dst, 512, "%s, %s", first, second);
            } else if (!first_has_percent && second_has_percent) {
                snprintf(dst, 512, "%s, %s", first, second);
            } else if (first_is_label || second_is_label) {
                snprintf(dst, 512, "%s, %s", second, first);
            } else {
                snprintf(dst, 512, "%s, %s", second, first);
            }
        } else {
            snprintf(dst, 512, "%s", parts[0]);
        }
    }

    for (int i = 0; i < count; i++) free(parts[i]);
    return dst;
}

static char* convertMemOperandSimple(const char* start) {
    if (!start || *start != '(') return NULL;

    const char* inner_start = start + 1;
    const char* inner_end = inner_start;
    while (*inner_end && *inner_end != ')') inner_end++;
    if (!*inner_end) return NULL;

    size_t inner_len = inner_end - inner_start;
    char* inner = malloc(inner_len + 1);
    if (!inner) return NULL;
    if (inner_len > 0) {
        memcpy(inner, inner_start, inner_len);
    }
    inner[inner_len] = '\0';

    char* offset = NULL;
    char* base = NULL;
    char* index = NULL;

    char* token = strtok(inner, "(,)");
    while (token) {
        while (*token == ' ' || *token == '\t') token++;
        size_t tl = strlen(token);
        while (tl > 0 && (token[tl - 1] == ' ' || token[tl - 1] == '\t'))
            token[--tl] = '\0';

        if (strchr(token, '%')) {
            if (!base)
                base = strdup(token + 1);
            else if (!index)
                index = strdup(token + 1);
        } else if (strlen(token) > 0) {
            if (!offset) offset = strdup(token);
        }
        token = strtok(NULL, "(,)");
    }
    free(inner);

    char* result = malloc(128);
    if (!result) {
        if (offset) free(offset);
        if (base) free(base);
        if (index) free(index);
        return NULL;
    }

    result[0] = '\0';
    char* rdst = result;

    if (offset) {
        strcpy(rdst, offset);
        rdst += strlen(offset);
    }
    if (base) {
        if (offset) *rdst++ = '+';
        strcpy(rdst, base);
        rdst += strlen(base);
    }
    if (index) {
        if (base || offset) *rdst++ = '+';
        strcpy(rdst, index);
        rdst += strlen(index);
        *rdst++ = '*';
        *rdst++ = '1';
        *rdst++ = '1';
    }

    *rdst = '\0';

    if (offset) free(offset);
    if (base) free(base);
    if (index) free(index);

    return result;
}
