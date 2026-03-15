# XVR AOT Compiler

The XVR language now uses an AOT (Ahead-of-Time) compiler that generates native executables via LLVM IR.

## Overview

The XVR compiler translates `.xvr` source files into:
- Native executables (via LLVM IR → object file → linked binary)
- LLVM IR (for debugging/inspection)
- Object files (for custom linking)

## Usage

```bash
# Compile and run
xvr script.xvr

# Compile to executable (default: a.out)
xvr script.xvr -o myprogram

# Compile to object file
xvr script.xvr -c -o program.o

# Dump LLVM IR
xvr script.xvr -l

# Show help
xvr -h

# Show version
xvr -v
```

## Language Features

### Variables

```xvr
var x = 42;
var name = "hello";
var pi = 3.14;
```

### Print with Format Strings

XVR uses `{}` placeholders

```xvr
var name = "world";
var num = 42;

print("Hello, {}!", name);      // Hello, world!
print("Value: {}", num);         // Value: 42
print("{} + {} = {}", 1, 2, 3); // 1 + 2 = 3

// Direct array printing
var arr = [1, 2, 3];
print(arr);                      // prints: 1 2 3
```

**Type inference:**
- `{}` with integer → `%d`
- `{}` with float → `%lf`
- `{}` with string → `%s`
- `{:p}` → pointer address

### Security

Only **literal strings** are parsed as format strings. User-controlled strings are passed directly to printf:

```xvr
var userInput = getInput();
print(userInput);  // Passed as-is, not interpreted as format
```

### While Loops

```xvr
var i = 0;
while (i < 10) {
    print("{}", i);
    i = i + 1;
}
```

### Compound Assignment

```xvr
var x = 10;
x += 5;  // x = 15
x -= 3;  // x = 12
x *= 2;  // x = 24
x /= 4;  // x = 6
x %= 5;  // x = 1
```

### Static Arrays

XVR supports static arrays with compile-time known sizes:

```xvr
var arr = [1, 2, 3, 4, 5];
print(arr[0]);  // prints 1
print(arr[2]);  // prints 3

// Array assignment
arr[1] = 20;
print(arr[1]);  // prints 20

// Array indexing with variables
var i = 0;
while (i < 5) {
    print(arr[i]);
    i += 1;
}

// Direct array printing
print(arr);  // prints: 1 2 3 4 5

// 2D arrays
var matrix = [
    [1, 2, 3],
    [4, 5, 6]
];
matrix[1][2] = 50;
```

**Features:**
- Array literals: `[val1, val2, ...]`
- Array indexing: `arr[index]` (zero-based)
- Array assignment: `arr[index] = value`
- Direct printing: `print(arr)` shows all elements
- Variable indices in loops
- 2D array literals

**Implementation:**
- Stored as `[N x i32]` in LLVM IR (stack-allocated)
- Uses GEP (GetElementPtr) for indexing
- Array print loops through elements

**Limitations:*
- Empty arrays (`[]`) not yet fully supported
- 2D array printing shows only first dimension elements

## Architecture

```
src/backend/
├── xvr_llvm_context.h/.c         # LLVM context management
├── xvr_llvm_type_mapper.h/.c    # Type mapping (xvr → LLVM)
├── xvr_llvm_module_manager.h/.c # Module management
├── xvr_llvm_ir_builder.h/.c    # IR building abstraction
├── xvr_llvm_expression_emitter.h/.c # Expression to IR (includes arrays)
├── xvr_llvm_function_emitter.h/.c  # Function emission
├── xvr_llvm_control_flow.h/.c   # If/while/for generation
├── xvr_llvm_optimizer.h/.c     # Optimization pipeline
├── xvr_llvm_target.h/.c        # Target configuration
├── xvr_llvm_codegen.h/.c       # Main coordinator
├── xvr_format_string.h/.c      # Format string parser
└── xvr_llvm_ir_builder.c        # CreateSRem for modulo
```

**Key files for array implementation:**
- `xvr_llvm_codegen.c` - Array literal handling in VAR_DECL, array type detection
- `xvr_llvm_expression_emitter.c` - Array indexing, assignment, and printing

## Format String Parser

The `xvr_format_string.c` module handles `{}` interpolation:

1. Parse format string for `{}` placeholders
2. Emit arguments and infer their types from LLVM values
3. Build printf format string with correct specifiers

### Type Mapping

| XVR Type | LLVM Type | printf |
|-----------|-----------|--------|
| `string` | `i8*` | `%s` |
| `integer` | `i32` | `%d` |
| `float` | `float` → `double` | `%lf` |
| `boolean` | `i1` | `%s` |

## Building

### Requirements

- LLVM 21+ (with C API headers)
- C compiler with C18 support

### Build

```bash
make
```

Output: `out/xvr`

### Linking

Object files can be linked with a simple runtime:

```c
// runtime.c
#include <stdio.h>
#include <stdarg.h>
int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vprintf(fmt, args);
    va_end(args);
    return result;
}
```

```bash
gcc -c runtime.c -o runtime.o
gcc program.o runtime.o -o program
```

## Output Examples

### LLVM IR

```bash
$ xvr test.xvr -l
; ModuleID = 'test'
source_filename = "test"

@fmt_str = private unnamed_addr constant [11 x i8] c"Hello, %s!\00", align 1

define i32 @main() {
entry:
  %name = alloca ptr, align 8
  store ptr @str_literal, ptr %name, align 8
  %name1 = load ptr, ptr %name, align 8
  %printf_call = call i32 (ptr, ...) @printf(ptr @fmt_str, ptr %name1)
  ret i32 0
}

declare i32 @printf(ptr, ...)
```

### Object File

```bash
$ xvr test.xvr -c -o test.o
$ file test.o
test.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
```

## Differences from Interpreter

- **AOT only**: No interpreter, no REPL
- **No `.xb` files**: Binary bytecode support removed
- **Format strings**: Uses `{}` instead of printf `%` syntax
- **Variables**: Use `var` keyword (not `let`)

## Error Handling

### Unused Variables

XVR detects unused variables at compile time:

```xvr
var x = 1;  // error: unused variable 'x'
```

Output:
```
[31merror[0m: unused variable 'x'
  --> line 1
[38;2;140;207;126mhelp[0m: variable 'x' is declared but never used
[38;2;140;207;126mhelp[0m: remove the unused variable or use it in an expression
```

### Type Mismatch

XVR validates that explicit type annotations match the inferred type from the value:

```xvr
var x: int = 1.5;    // error: type mismatch: cannot convert from 'float' to 'int'
var x: string = 123; // error: type mismatch: cannot convert from 'int' to 'string'
var x: bool = 1;    // error: type mismatch: cannot convert from 'int' to 'bool'
```

Output:
```
[31merror[0m: type mismatch: cannot convert from 'float' to 'int'
[38;2;140;207;126mhelp[0m: Check your code for type errors or unsupported features
```

**Allowed implicit conversions:**
- Integer types to float/float64 (e.g., `var x: float = 42;`)
- Between different integer sizes (e.g., `var x: int64 = 42;`)
- Exact type matches

**Not allowed:**
- Float to integer
- String to/from numeric types
- Integer to boolean

## Future Enhancements

- Function declarations
- Struct types
- Better optimization passes
- Multiple return values
