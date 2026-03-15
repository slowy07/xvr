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

#ifndef XVR_LLVM_TARGET_H
#define XVR_LLVM_TARGET_H

#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <stdbool.h>
#include <stddef.h>

#include "xvr_llvm_module_manager.h"

typedef struct Xvr_LLVMTargetConfig Xvr_LLVMTargetConfig;

Xvr_LLVMTargetConfig* Xvr_LLVMTargetConfigCreate(void);
void Xvr_LLVMTargetConfigDestroy(Xvr_LLVMTargetConfig* config);

bool Xvr_LLVMTargetConfigSetTriple(Xvr_LLVMTargetConfig* config,
                                   const char* triple);
bool Xvr_LLVMTargetConfigSetCPU(Xvr_LLVMTargetConfig* config, const char* cpu);
bool Xvr_LLVMTargetConfigSetFeatures(Xvr_LLVMTargetConfig* config,
                                     const char* features);
bool Xvr_LLVMTargetConfigSetReloc(Xvr_LLVMTargetConfig* config,
                                  const char* reloc);
bool Xvr_LLVMTargetConfigSetCodeModel(Xvr_LLVMTargetConfig* config,
                                      const char* model);

typedef struct Xvr_LLVMTargetMachine Xvr_LLVMTargetMachine;

Xvr_LLVMTargetMachine* Xvr_LLVMTargetMachineCreate(
    Xvr_LLVMTargetConfig* config);
void Xvr_LLVMTargetMachineDestroy(Xvr_LLVMTargetMachine* tm);

bool Xvr_LLVMTargetMachineEmitToFile(Xvr_LLVMTargetMachine* tm,
                                     Xvr_LLVMModuleManager* module,
                                     const char* filename, int filetype);

void* Xvr_LLVMTargetMachineEmitToMemory(Xvr_LLVMTargetMachine* tm,
                                        Xvr_LLVMModuleManager* module,
                                        size_t* out_size);

const char* Xvr_LLVMTargetMachineGetDefaultTargetTriple(void);
const char* Xvr_LLVMTargetMachineGetDefaultCPU(void);

#endif
