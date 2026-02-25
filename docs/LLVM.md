# LLVM IR Backend Implementation

The xvr language now supports compilation to LLVM IR (Intermediate Representation), enabling native code generation, JIT compilation, and various optimization capabilities.

## Overview

The LLVM backend translates xvr's AST (Abstract Syntax Tree) into LLVM IR, which can then be:
- Compiled to native machine code
- JIT-compiled for immediate execution
- Optimized using LLVM's optimization passes
- Written to bitcode files for later use

## Architecture

The backend is implemented as a set of modular components in `src/backend/`:

```
src/backend/
├── xvr_llvm_context.h/.c         # LLVM context management
├── xvr_llvm_type_mapper.h/.c      # Type mapping (xvr → LLVM)
├── xvr_llvm_module_manager.h/.c   # Module management
├── xvr_llvm_ir_builder.h/.c       # IR building abstraction
├── xvr_llvm_expression_emitter.h/.c # Expression to IR
├── xvr_llvm_function_emitter.h/.c  # Function emission
├── xvr_llvm_control_flow.h/.c      # If/while/for generation
├── xvr_llvm_optimizer.h/.c         # Optimization pipeline
├── xvr_llvm_target.h/.c            # Target configuration
└── xvr_llvm_codegen.h/.c          # Main coordinator
```

## Components

### 1. Context Manager (`xvr_llvm_context`)

Manages the LLVM context, which is the container for all LLVM types and values.

```c
Xvr_LLVMContext* ctx = Xvr_LLVMContextCreate();
// Use context for type creation, etc.
Xvr_LLVMContextDestroy(ctx);
```

### 2. Type Mapper (`xvr_llvm_type_mapper`)

Maps xvr's runtime types to LLVM types:

| xvr Type | LLVM Type |
|----------|-----------|
| `null` | `i8*` (pointer) |
| `boolean` | `i1` |
| `integer` | `i64` |
| `float` | `float` (32-bit) |
| `string` | `i8*` (C string) |
| `array` | `i8*` (opaque pointer) |
| `dictionary` | `i8*` (opaque pointer) |
| `function` | `i8*` (function pointer) |

```c
Xvr_LLVMTypeMapper* mapper = Xvr_LLVMTypeMapperCreate(ctx);
LLVMTypeRef int_type = Xvr_LLVMTypeMapperGetIntType(mapper);
LLVMTypeRef float_type = Xvr_LLVMTypeMapperGetFloatType(mapper);
```

### 3. Module Manager (`xvr_llvm_module_manager`)

Creates and manages LLVM modules, which contain functions and global variables.

```c
Xvr_LLVMModuleManager* mgr = Xvr_LLVMModuleManagerCreate(ctx, "my_module");
LLVMModuleRef module = Xvr_LLVMModuleManagerGetModule(mgr);
```

### 4. IR Builder (`xvr_llvm_ir_builder`)

Provides a high-level API for building LLVM IR instructions:

```c
Xvr_LLVMIRBuilder* builder = Xvr_LLVMIRBuilderCreate(ctx, module);

// Create basic block
LLVMBasicBlockRef block = Xvr_LLVMIRBuilderCreateBlockInFunction(builder, fn, "entry");
Xvr_LLVMIRBuilderSetInsertPoint(builder, block);

// Build instructions
Xvr_LLVMIRBuilderCreateRet(builder, return_value);
Xvr_LLVMIRBuilderCreateAdd(builder, lhs, rhs, "add_tmp");
```

### 5. Expression Emitter (`xvr_llvm_expression_emitter`)

Translates xvr expressions into LLVM IR:

- **Literals**: `null`, `boolean`, `integer`, `float`, `string`
- **Binary operations**: `+`, `-`, `*`, `/`, `%`, `<`, `>`, `<=`, `>=`, `==`, `!=`, `&&`, `||`
- **Unary operations**: `-` (negate), `!` (invert)
- **Function calls**

```c
LLVMValueRef value = Xvr_LLVMExpressionEmitterEmit(emitter, expr_node);
```

### 6. Function Emitter (`xvr_llvm_function_emitter`)

Emits LLVM functions from xvr function declarations:

```c
bool success = Xvr_LLVMFunctionEmitterEmit(emitter, fn_decl_node);
```

### 7. Control Flow Generator (`xvr_llvm_control_flow`)

Handles control flow structures:

- **If/Else**: Conditional branches
- **While loops**: Loop with condition check
- **For loops**: Init → condition → update pattern

### 8. Optimizer (`xvr_llvm_optimizer`)

Applies LLVM optimization passes:

```c
Xvr_LLVMOptimizer* opt = Xvr_LLVMOptimizerCreate();
Xvr_LLVMOptimizerSetLevel(opt, XVR_LLVM_OPT_O2);
Xvr_LLVMOptimizerAddStandardPasses(opt);
Xvr_LLVMOptimizerOptimize(opt, module);
```

Optimization levels:
- `XVR_LLVM_OPT_NONE` - No optimization
- `XVR_LLVM_OPT_O1` - Basic optimization
- `XVR_LLVM_OPT_O2` - Standard optimization
- `XVR_LLVM_OPT_O3` - Aggressive optimization

### 9. Target Configuration (`xvr_llvm_target`)

Configures target architecture:

```c
Xvr_LLVMTargetConfig* config = Xvr_LLVMTargetConfigCreate();
Xvr_LLVMTargetConfigSetTriple(config, "x86_64-pc-linux-gnu");
Xvr_LLVMTargetConfigSetCPU(config, "generic");
Xvr_LLVMTargetConfigSetFeatures(config, "+sse,+sse2");
```

### 10. Codegen Coordinator (`xvr_llvm_codegen`)

Main entry point that coordinates all components:

```c
Xvr_LLVMCodegen* codegen = Xvr_LLVMCodegenCreate("my_module");

// Configure
Xvr_LLVMCodegenSetOptimizationLevel(codegen, XVR_LLVM_OPT_O2);
Xvr_LLVMCodegenSetTargetTriple(codegen, "x86_64-pc-linux-gnu");

// Emit IR from AST
Xvr_LLVMCodegenEmitAST(codegen, ast);

// Get output
char* ir = Xvr_LLVMCodegenPrintIR(codegen, &len);

// Write bitcode
Xvr_LLVMCodegenWriteBitcode(codegen, "output.bc");

Xvr_LLVMCodegenDestroy(codegen);
```

## CLI Integration

The `-l` or `--llvm` flag dumps LLVM IR instead of executing:

```bash
./xvr -l script.xvr    # Dump LLVM IR
./xvr script.xvr       # Execute normally
./xvr -i "println(1)"  # Execute inline code
./xvr -l -i "1+1"      # Dump IR for inline code
```

## Building with LLVM

### Requirements

- LLVM 21+ (with C API headers)
- C compiler with C18 support

### Build

```bash
make
```

The build system automatically detects LLVM. To specify a specific version:

```bash
make LLVM_CONFIG=llvm-config-21
```

## Output Formats

### Human-Readable IR

```bash
$ ./xvr -l test.xvr
; ModuleID = 'xvr_module'
source_filename = "xvr_module"

define i32 @fibo() {
entry:
  ret i32 0
}
```

### Bitcode

```bash
$ ./xvr -l -o output.bc test.xvr
```

### Object File

```bash
$ ./xvr -c -o output.o test.xvr
```

## Memory Management

The backend follows these ownership rules:

- **Created objects**: Caller owns, must call destroy function
- **LLVMValueRef/LLVMModuleRef**: Owned by LLVM module, don't free manually
- **Strings**: Caller owns, must free returned strings

## Testing

Run the LLVM backend tests:

```bash
make test
```

Tests are located in `test/test_llvm_backend.c` and cover:
- Context creation/destruction
- Type mapping
- Module management
- IR building
- Optimization
- Target configuration
- Full codegen pipeline

## Platform Support

- **Linux**: Full support with LLVM 21
- **Windows (MSYS2/MinGW)**: Supported, requires `mingw-w64-x86_64-llvm`
- **macOS**: Should work with LLVM from Homebrew or official packages

## Future Enhancements

- Full function return value support
- Struct types for compound types
- Better optimization for xvr-specific patterns
- Native code compilation (via LLC)
- JIT execution integration
