#include "xvr_builtin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "xvr_literal.h"

#ifdef XVR_EXPORT_LLVM
#    include "xvr_lexer.h"
#    include "xvr_parser.h"
#endif

#define XVR_BUILTIN_MAX 64

struct Xvr_BuiltinRegistry {
    Xvr_BuiltinInfo* builtins[XVR_BUILTIN_MAX];
    int count;
    LLVMContextRef llvm_context;
};

struct Xvr_ModuleResolver {
    char* stdlib_path;
};

static LLVMValueRef handle_sizeof(void* context, LLVMBuilderRef builder,
                                  Xvr_ASTNode* call_node,
                                  Xvr_ASTNode** arguments, int arg_count) {
    (void)builder;
    (void)call_node;
    (void)context;

    if (arg_count != 1) {
        return NULL;
    }

    Xvr_ASTNode* type_arg = arguments[0];
    if (!type_arg) {
        return NULL;
    }

    if (type_arg->type == XVR_AST_NODE_LITERAL) {
        Xvr_Literal lit = type_arg->atomic.literal;
        if (lit.type == XVR_LITERAL_TYPE) {
            Xvr_LiteralType type_val = XVR_AS_TYPE(lit).typeOf;
            int size_bits = 0;
            switch (type_val) {
            case XVR_LITERAL_VOID:
                size_bits = 0;
                break;
            case XVR_LITERAL_BOOLEAN:
            case XVR_LITERAL_INT8:
            case XVR_LITERAL_UINT8:
                size_bits = 8;
                break;
            case XVR_LITERAL_INT16:
            case XVR_LITERAL_UINT16:
                size_bits = 16;
                break;
            case XVR_LITERAL_INTEGER:
            case XVR_LITERAL_INT32:
            case XVR_LITERAL_UINT32:
            case XVR_LITERAL_FLOAT32:
                size_bits = 32;
                break;
            case XVR_LITERAL_INT64:
            case XVR_LITERAL_UINT64:
            case XVR_LITERAL_FLOAT64:
                size_bits = 64;
                break;
            case XVR_LITERAL_FLOAT16:
                size_bits = 16;
                break;
            default:
                size_bits = 0;
                break;
            }
            LLVMContextRef ctx = LLVMGetGlobalContext();
            LLVMValueRef result =
                LLVMConstInt(LLVMInt32TypeInContext(ctx), size_bits, false);
            return result;
        }
    }

    return NULL;
}

static LLVMValueRef handle_len(void* context, LLVMBuilderRef builder,
                               Xvr_ASTNode* call_node, Xvr_ASTNode** arguments,
                               int arg_count) {
    (void)context;
    (void)builder;
    (void)call_node;
    (void)arguments;

    if (arg_count != 1) {
        return NULL;
    }

    return NULL;
}

static LLVMValueRef handle_panic(void* context, LLVMBuilderRef builder,
                                 Xvr_ASTNode* call_node,
                                 Xvr_ASTNode** arguments, int arg_count) {
    (void)context;
    (void)builder;
    (void)call_node;
    (void)arguments;

    if (arg_count != 1) {
        return NULL;
    }

    return NULL;
}

Xvr_BuiltinRegistry* Xvr_BuiltinRegistryCreate(void) {
    Xvr_BuiltinRegistry* registry = calloc(1, sizeof(Xvr_BuiltinRegistry));
    if (!registry) {
        return NULL;
    }

    registry->llvm_context = LLVMContextCreate();
    Xvr_BuiltinRegistryInitDefaults(registry);

    return registry;
}

void Xvr_BuiltinRegistryDestroy(Xvr_BuiltinRegistry* registry) {
    if (!registry) {
        return;
    }

    if (registry->llvm_context) {
        LLVMContextDispose(registry->llvm_context);
    }

    for (int i = 0; i < registry->count; i++) {
        free((void*)registry->builtins[i]->name);
        free((void*)registry->builtins[i]->llvm_name);
        free(registry->builtins[i]);
    }

    free(registry);
}

bool Xvr_BuiltinRegistryRegister(Xvr_BuiltinRegistry* registry,
                                 const char* name, Xvr_BuiltinType type,
                                 Xvr_BuiltinHandler handler,
                                 const char* llvm_name, int min_args,
                                 int max_args) {
    if (!registry || !name || !handler) {
        return false;
    }

    if (registry->count >= XVR_BUILTIN_MAX) {
        return false;
    }

    Xvr_BuiltinInfo* info = calloc(1, sizeof(Xvr_BuiltinInfo));
    if (!info) {
        return false;
    }

    info->name = strdup(name);
    info->type = type;
    info->handler = handler;
    info->llvm_name = llvm_name ? strdup(llvm_name) : NULL;
    info->min_args = min_args;
    info->max_args = max_args;

    registry->builtins[registry->count++] = info;

    return true;
}

Xvr_BuiltinInfo* Xvr_BuiltinRegistryLookup(Xvr_BuiltinRegistry* registry,
                                           const char* name) {
    if (!registry || !name) {
        return NULL;
    }

    for (int i = 0; i < registry->count; i++) {
        if (registry->builtins[i] &&
            strcmp(registry->builtins[i]->name, name) == 0) {
            return registry->builtins[i];
        }
    }

    return NULL;
}

bool Xvr_BuiltinRegistryIsBuiltin(Xvr_BuiltinRegistry* registry,
                                  const char* name) {
    return Xvr_BuiltinRegistryLookup(registry, name) != NULL;
}

void Xvr_BuiltinRegistryInitDefaults(Xvr_BuiltinRegistry* registry) {
    Xvr_BuiltinRegistryRegister(registry, "sizeof", XVR_BUILTIN_SIZEOF,
                                handle_sizeof, NULL, 1, 1);

    Xvr_BuiltinRegistryRegister(registry, "len", XVR_BUILTIN_LEN, handle_len,
                                NULL, 1, 1);

    Xvr_BuiltinRegistryRegister(registry, "panic", XVR_BUILTIN_PANIC,
                                handle_panic, NULL, 1, 1);
}

Xvr_ModuleResolver* Xvr_ModuleResolverCreate(const char* stdlib_path) {
    Xvr_ModuleResolver* resolver = calloc(1, sizeof(Xvr_ModuleResolver));
    if (!resolver) {
        return NULL;
    }

    if (stdlib_path) {
        resolver->stdlib_path = strdup(stdlib_path);
    } else {
        resolver->stdlib_path = strdup("./lib/std");
    }

    return resolver;
}

void Xvr_ModuleResolverDestroy(Xvr_ModuleResolver* resolver) {
    if (!resolver) {
        return;
    }

    free(resolver->stdlib_path);
    free(resolver);
}

bool Xvr_ModuleResolverResolve(Xvr_ModuleResolver* resolver,
                               const char* module_name, char** out_path) {
    if (!resolver || !module_name || !out_path) {
        return false;
    }

    for (size_t i = 0; i < strlen(module_name); i++) {
        if (module_name[i] == '/' || module_name[i] == '\\' ||
            module_name[i] == '.' && (i == 0 || module_name[i - 1] == '/')) {
            return false;
        }
    }

    size_t path_len = strlen(resolver->stdlib_path) + strlen(module_name) + 16;
    char* path = malloc(path_len);
    if (!path) {
        return false;
    }

    snprintf(path, path_len, "%s/%s.xvr", resolver->stdlib_path, module_name);

    *out_path = path;
    return true;
}

static const unsigned char* read_file(const char* path, size_t* size) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len < 0) {
        fclose(f);
        return NULL;
    }

    unsigned char* buffer = malloc(len + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buffer, 1, len, f);
    buffer[read] = '\0';
    fclose(f);

    *size = read;
    return buffer;
}

bool Xvr_ModuleResolverLoadModule(Xvr_ModuleResolver* resolver,
                                  const char* module_path,
                                  Xvr_ASTNode*** out_nodes, int* out_count) {
#ifdef XVR_EXPORT_LLVM
    if (!resolver || !module_path || !out_nodes || !out_count) {
        return false;
    }

    FILE* test = fopen(module_path, "r");
    if (!test) {
        return false;
    }
    fclose(test);

    size_t file_size = 0;
    const unsigned char* source = read_file(module_path, &file_size);
    if (!source) {
        return false;
    }

    Xvr_Lexer lexer;
    Xvr_Parser parser;

    Xvr_initLexer(&lexer, (const char*)source);
    Xvr_initParser(&parser, &lexer);

    Xvr_ASTNode** nodes = NULL;
    int node_count = 0;
    *out_nodes = NULL;
    int node_capacity = 0;

    Xvr_ASTNode* node = Xvr_scanParser(&parser);
    while (node != NULL) {
        if (node->type == XVR_AST_NODE_ERROR) {
            Xvr_freeASTNode(node);
            for (int i = 0; i < node_count; i++) {
                Xvr_freeASTNode(nodes[i]);
            }
            free(nodes);
            Xvr_freeParser(&parser);
            free((void*)source);
            *out_count = 0;
            return false;
        }

        if (node_count >= node_capacity) {
            node_capacity = node_capacity < 8 ? 8 : node_capacity * 2;
            nodes = realloc(nodes, sizeof(Xvr_ASTNode*) * node_capacity);
        }
        nodes[node_count++] = node;
        node = Xvr_scanParser(&parser);
    }

    Xvr_freeParser(&parser);
    free((void*)source);

    *out_nodes = nodes;
    *out_count = node_count;
    return true;
#else
    (void)resolver;
    (void)module_path;
    (void)out_nodes;
    (void)out_count;
    return false;
#endif
}

const char* Xvr_ModuleResolverGetStdlibPath(Xvr_ModuleResolver* resolver) {
    return resolver ? resolver->stdlib_path : NULL;
}

void Xvr_ModuleResolverSetStdlibPath(Xvr_ModuleResolver* resolver,
                                     const char* path) {
    if (!resolver) {
        return;
    }

    free(resolver->stdlib_path);
    resolver->stdlib_path = path ? strdup(path) : strdup("./lib/std");
}
