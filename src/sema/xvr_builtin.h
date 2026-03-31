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

#ifndef XVR_BUILTIN_H
#define XVR_BUILTIN_H

#include <llvm-c/Core.h>
#include <stdbool.h>
#include <stdint.h>

#include "xvr_ast_node.h"
#include "xvr_type.h"

typedef enum {
    XVR_BUILTIN_NONE,
    XVR_BUILTIN_SIZEOF,
    XVR_BUILTIN_LEN,
    XVR_BUILTIN_PANIC,
} Xvr_BuiltinType;

typedef struct Xvr_BuiltinRegistry Xvr_BuiltinRegistry;
typedef struct Xvr_BuiltinInfo Xvr_BuiltinInfo;

typedef LLVMValueRef (*Xvr_BuiltinHandler)(void* context,
                                           LLVMBuilderRef builder,
                                           Xvr_ASTNode* call_node,
                                           Xvr_ASTNode** arguments,
                                           int arg_count);

struct Xvr_BuiltinInfo {
    const char* name;
    Xvr_BuiltinType type;
    Xvr_BuiltinHandler handler;
    const char* llvm_name;
    int min_args;
    int max_args;
};

Xvr_BuiltinRegistry* Xvr_BuiltinRegistryCreate(void);
void Xvr_BuiltinRegistryDestroy(Xvr_BuiltinRegistry* registry);

bool Xvr_BuiltinRegistryRegister(Xvr_BuiltinRegistry* registry,
                                 const char* name, Xvr_BuiltinType type,
                                 Xvr_BuiltinHandler handler,
                                 const char* llvm_name, int min_args,
                                 int max_args);

Xvr_BuiltinInfo* Xvr_BuiltinRegistryLookup(Xvr_BuiltinRegistry* registry,
                                           const char* name);

bool Xvr_BuiltinRegistryIsBuiltin(Xvr_BuiltinRegistry* registry,
                                  const char* name);

void Xvr_BuiltinRegistryInitDefaults(Xvr_BuiltinRegistry* registry);

typedef struct Xvr_ModuleResolver Xvr_ModuleResolver;

Xvr_ModuleResolver* Xvr_ModuleResolverCreate(const char* stdlib_path);
void Xvr_ModuleResolverDestroy(Xvr_ModuleResolver* resolver);

bool Xvr_ModuleResolverResolve(Xvr_ModuleResolver* resolver,
                               const char* module_name, char** out_path);

bool Xvr_ModuleResolverLoadModule(Xvr_ModuleResolver* resolver,
                                  const char* module_path,
                                  Xvr_ASTNode*** out_nodes, int* out_count);

const char* Xvr_ModuleResolverGetStdlibPath(Xvr_ModuleResolver* resolver);
void Xvr_ModuleResolverSetStdlibPath(Xvr_ModuleResolver* resolver,
                                     const char* path);

#endif
