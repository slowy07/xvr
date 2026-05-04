# Module Resolver API

The XVR Module Resolver provides a flexible system for locating and loading XVR modules from multiple search paths.

## Overview

The module resolver searches for modules in a configurable list of directories, supporting:
- Standard library paths
- Environment variable configuration
- Source-relative discovery
- Custom search paths

## API Reference

### Xvr_ModuleResolverCreate

Creates a new module resolver instance.

```c
Xvr_ModuleResolver* Xvr_ModuleResolverCreate(const char* stdlib_path);
```

**Parameters:**
- `stdlib_path`: Initial search path (typically the standard library directory)

**Returns:** New resolver instance, or NULL on failure

**Example:**
```c
Xvr_ModuleResolver* resolver = Xvr_ModuleResolverCreate("./lib/std");
```

### Xvr_ModuleResolverDestroy

Frees a module resolver instance.

```c
void Xvr_ModuleResolverDestroy(Xvr_ModuleResolver* resolver);
```

### Xvr_ModuleResolverAddSearchPath

Adds a new search path to the resolver.

```c
bool Xvr_ModuleResolverAddSearchPath(Xvr_ModuleResolver* resolver, const char* path);
```

**Parameters:**
- `resolver`: Resolver instance
- `path`: Directory path to add

**Returns:** true on success, false on failure

**Note:** Maximum paths limited by `XVR_MAX_SEARCH_PATHS` (default: 16)

**Example:**
```c
Xvr_ModuleResolverAddSearchPath(resolver, "/usr/local/lib/xvr");
Xvr_ModuleResolverAddSearchPath(resolver, "../lib");
```

### Xvr_ModuleResolverSetStdlibPath

Sets or updates the primary standard library path.

```c
void Xvr_ModuleResolverSetStdlibPath(Xvr_ModuleResolver* resolver, const char* path);
```

### Xvr_ModuleResolverResolve

Resolves a module name to a file path.

```c
bool Xvr_ModuleResolverResolve(Xvr_ModuleResolver* resolver,
                               const char* module_name,
                               char** out_path);
```

**Parameters:**
- `resolver`: Resolver instance
- `module_name`: Module name (e.g., "std", "mymodule")
- `out_path`: Output buffer for resolved path

**Returns:** true if module found, false otherwise

### Xvr_ModuleResolverLoadModule

Loads and parses a module file.

```c
bool Xvr_ModuleResolverLoadModule(Xvr_ModuleResolver* resolver,
                                  const char* module_path,
                                  Xvr_ASTNode*** out_nodes,
                                  int* out_count);
```

## Search Order

The resolver searches paths in this order:
1. Standard library path (set via `Xvr_ModuleResolverSetStdlibPath`)
2. Environment variable `XVR_LIB_PATH` (if set)
3. Additional paths added via `Xvr_ModuleResolverAddSearchPath`
4. Default paths: `./lib/std`, `../lib/std`, `./lib`, `../lib`

## Integration with Compiler

### Setting Include Directory

```c
bool Xvr_LLVMCodegenSetIncludeDir(Xvr_LLVMCodegen* codegen,
                                   const char* include_dir);
```

This convenience function adds a search path to the codegen's module resolver.

**Example:**
```c
Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("my_module");
Xvr_LLVMCodegenSetIncludeDir(codegen, "/usr/local/xvr/lib");
```

## Error Handling

All functions return bool for success/failure. Use `Xvr_LLVMContextGetErrorMessage()` to retrieve error details after a failure.

## Thread Safety

The module resolver is NOT thread-safe. Use separate instances per thread or external synchronization.

## Example: Custom Module Resolution

```c
// Create resolver with custom stdlib path
Xvr_ModuleResolver* resolver = Xvr_ModuleResolverCreate("/opt/xvr/lib/std");

// Add additional search paths
Xvr_ModuleResolverAddSearchPath(resolver, "/usr/local/lib/xvr");
Xvr_ModuleResolverAddSearchPath(resolver, "./modules");

// Resolve a module
char* path = NULL;
if (Xvr_ModuleResolverResolve(resolver, "mymodule", &path)) {
    printf("Found module at: %s\n", path);
    free(path);
}

// Cleanup
Xvr_ModuleResolverDestroy(resolver);
```
