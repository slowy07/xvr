# XVR AOT Compiler

The XVR language uses an AOT (Ahead-of-Time) compiler that generates native executables via LLVM IR.

## Compilation Pipeline

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  .xvr File  │───▶│    Lexer    │───▶│   Parser    │───▶│  AST Nodes  │───▶│    LLVM     │
│  (Source)   │    │  (Tokens)   │    │  (Errors)   │    │   (Tree)    │    │     IR      │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
                                                                                    │
                                                                                    ▼
                                                                           ┌─────────────────┐
                                                                           │  LLVM Optimizer │
                                                                           │   (Optional)    │
                                                                           └─────────────────┘
                                                                                    │
                                                                                    ▼
      ┌──────────────────────────────────────────────────────────────────────────────────┐
      │                                                                                  │
      ▼                                                                                  ▼
┌─────────────┐                                                          ┌─────────────────┐
│  Executable │◀─── Link ───┐                                   ┌───────▶│    Object File  │
│   (a.out)   │             │                                   │        │    (.o/.obj)    │
└─────────────┘             │        ┌─────────────────┐        │        └─────────────────┘
                            └───────▶│    LLVM MCJIT   │────────┘
                                     │   or JIT (dev)  │
                                     └─────────────────┘
```

## Overview

The XVR compiler translates `.xvr` source files into:
- Native executables (via LLVM IR → object file → linked binary)
- LLVM IR (for debugging/inspection)
- Object files (for custom linking)

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                    XVR Compiler                                         │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────────────────────────┐  │
│  │   Lexer     │───▶│   Parser    │───▶│              LLVM Backend                   │  │
│  │ xvr_lexer.c │    │xvr_parser.c │    │                                             │  │
│  └─────────────┘    └─────────────┘    │  ┌─────────────────────────────────────────┐│  │
│                                        │  │         xvr_llvm_codegen.c              ││  │
│                                        │  │              (Coordinator)              ││  │
│                                        │  └──────────────────┬──────────────────────┘│  │
│                                        │                     │                       │  │
│                                        │    ┌────────────────┼────────────────────┐  │  │
│                                        │    │                │                    │  │  │
│                                        │    ▼                ▼                    |  ▼  │  
│                                        │  ┌──────────┐ ┌──────────┐ ┌──────────────┐ │  │
│                                        │  │Context   │ │Type      │ │Module        │ │  │
│                                        │  │Manager   │ │Mapper    │ │Manager       │ │  │
│                                        │  │.c        │ │.c        │ │.c            │ │  │
│                                        │  └──────────┘ └──────────┘ └──────────────┘ │  │
│                                        │  ┌──────────┐ ┌──────────┐ ┌──────────────┐ │  │
│                                        │  │IR        │ │Expression │ │Function     │ │  │
│                                        │  │Builder   │ │Emitter   │ │Emitter       │ │  │
│                                        │  │.c        │ │.c        │ │.c            │ │  │
│                                        │  └──────────┘ └──────────┘ └──────────────┘ │  │
│                                        │  ┌──────────┐ ┌──────────┐ ┌──────────────┐ │  │
│                                        │  │Control   │ │Optimizer │ │Target        │ │  │
│                                        │  │Flow      │ │.c        │ │.c            │ │  │
│                                        │  │.c        │ │          │ │              │ │  │
│                                        │  └──────────┘ └──────────┘ └──────────────┘ │  │
│                                        │                                             │  │
│                                        └─────────────────────────────────────────────┘  │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

## Backend Module Flow

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                           LLVM Backend Data Flow                                       │
└────────────────────────────────────────────────────────────────────────────────────────┘

  AST Node              Codegen Stage           LLVM IR               Output
  ────────              ─────────────           ───────               ──────
  
  ┌──────────┐         ┌──────────────┐       ┌───────────┐
  │ VAR_DECL │────────▶│ xvr_llvm_    │──────▶│ %x =      │
  │ var x=42 │         │ codegen.c    │       │ alloca i32│
  └──────────┘         └──────────────┘       └───────────┘
                                    │
                                    ▼
                         ┌──────────────────┐
                         │ xvr_llvm_type_   │
                         │ mapper.c         │────────▶ i32, i8*, float, etc.
                         └──────────────────┘
                                    │
                                    ▼
                         ┌──────────────────┐
                         │ xvr_llvm_ir_     │
                         │ builder.c        │────────▶ LLVMBuildAlloca, LLVMBuildStore
                         └──────────────────┘

  ┌──────────┐         ┌──────────────┐       ┌───────────┐
  │  BINARY   │────────▶│ xvr_llvm_    │──────▶│ %add =    │
  │ x + y     │         │ expression   │       │ add i32   │
  └──────────┘         │ emitter.c    │       │ %x, %y    │
                       └──────────────┘       └───────────┘

  ┌──────────┐         ┌──────────────┐       ┌───────────┐
  │  IF/     │────────▶│ xvr_llvm_    │──────▶│ br i1 %   │
  │  WHILE   │         │ control_flow │       │ cond,     │
  │          │         │ .c           │       │ label,    │
  └──────────┘         └──────────────┘       │ label     │
                                              └───────────┘
```

## Source File Organization

```
src/backend/
├── xvr_llvm_codegen.h/.c         # Main coordinator, entry points
├── xvr_llvm_context.h/.c         # LLVM context management
├── xvr_llvm_type_mapper.h/.c    # Type mapping (XVR → LLVM types)
├── xvr_llvm_module_manager.h/.c # Module creation & IR printing
├── xvr_llvm_ir_builder.h/.c    # IR building (alloca, store, load, etc.)
├── xvr_llvm_expression_emitter.h/.c # Expressions, arrays, indexing
├── xvr_llvm_function_emitter.h/.c   # Function definitions
├── xvr_llvm_control_flow.h/.c      # If/while/for generation
├── xvr_llvm_optimizer.h/.c       # Optimization passes
├── xvr_llvm_target.h/.c          # Target machine configuration
└── xvr_format_string.h/.c        # Format string {} parser
```

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

### If Statements

XVR supports traditional if-else statements and **expression-based if** (Rust-like):

#### Statement Form

```xvr
var score: int32 = 85;

if (score >= 90) {
    print("Grade: A");
} else if (score >= 80) {
    print("Grade: B");
} else {
    print("Grade: C or lower");
}
```

#### Expression Form (Rust-like)

The `if` can be used as an expression that returns a value:

```xvr
var score: int32 = 85;

var grade: string = if (score >= 90) {
    "A"
} else if (score >= 80) {
    "B"
} else {
    "C or lower"
};

print(grade);
```

**Note:** Expression-based `if` requires explicit type annotation on the variable.

### While Loops

```xvr
var i = 0;
while (i < 10) {
    print("{}", i);
    i = i + 1;
}
```

### Break and Continue

Control loop execution with `break` and `continue`:

```xvr
var i = 0;
while (i < 100) {
    i = i + 1;
    if (i == 50) {
        break;  // Exit loop when i reaches 50
    }
    if (i % 2 == 0) {
        continue;  // Skip even numbers
    }
    print("{}", i);  // Only prints odd numbers
}
```

**Conditions must be boolean** - the compiler validates this and provides helpful hints:

```xvr
var x = 5;
if (x) { }  // ERROR: condition must be boolean
// help: use a comparison operator (e.g., 'x > 0') or wrap the condition with 'bool()'
```

#### Expression Form (Rust-like)

The `if` can be used as an expression that returns a value:

```xvr
var score: int32 = 85;

var grade: string = if (score >= 90) {
    "A"
} else if (score >= 80) {
    "B"
} else {
    "C or lower"
};

print(grade);
```

**Note:** Expression-based `if` requires explicit type annotation on the variable.

### While Loops

```xvr
var i = 0;
while (i < 10) {
    print("{}", i);
    i = i + 1;
}
```

### Break and Continue

Control loop execution with `break` and `continue`:

```xvr
var i = 0;
while (i < 100) {
    i = i + 1;
    if (i == 50) {
        break;  // Exit loop when i reaches 50
    }
    if (i % 2 == 0) {
        continue;  // Skip even numbers
    }
    print("{}", i);  // Only prints odd numbers
}
```

**Conditions must be boolean** - the compiler validates this and provides helpful hints:

```xvr
var x = 5;
if (x) { }  // ERROR: condition must be boolean
// help: use a comparison operator (e.g., 'x > 0') or wrap the condition with 'bool()'
```

### Print with Format Strings

XVR uses `{}` placeholders:

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

### While Loops

```xvr
var i = 0;
while (i < 10) {
    print("{}", i);
    i = i + 1;
}
```

### Break and Continue

Control loop execution with `break` and `continue`:

```xvr
var i = 0;
while (i < 100) {
    i = i + 1;
    if (i == 50) {
        break;  // Exit loop when i reaches 50
    }
    if (i % 2 == 0) {
        continue;  // Skip even numbers
    }
    print("{}", i);  // Only prints odd numbers
}
```

**Conditions must be boolean** - the compiler validates this and provides helpful hints:

```xvr
var x = 5;
while (x) { }  // ERROR: condition must be boolean
// help: use a comparison operator (e.g., 'x > 0') or wrap the condition with 'bool()'
```

### Static Arrays

```xvr
var arr = [1, 2, 3, 4, 5];
print(arr[0]);  // prints 1

// Array assignment
arr[1] = 20;
print(arr[1]);  // prints 20
```

## Format String Parser

The `xvr_format_string.c` module handles `{}` interpolation:

```
Format String Parsing Flow:
┌─────────────────┐
│  "Hello {}!"    │  (input)
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────┐
│  Parse for {} placeholders          │
│  Count arguments                    │
└────────┬────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│  Infer types from LLVM values       │
│  integer → %d                       │
│  float   → %lf                      │
│  string  → %s                       │
└────────┬────────────────────────────┘
         │
         ▼
┌─────────────────┐
│  "Hello %s!"    │  (printf format)
└─────────────────┘
```

### Type Mapping

| XVR Type   | LLVM Type        | printf  |
|------------|------------------|---------|
| `string`   | `i8*`            | `%s`    |
| `integer`  | `i32`            | `%d`    |
| `float`    | `float` → `double` | `%lf` |
| `boolean` | `i1`             | `%s`    |

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

## Error Handling

The XVR compiler provides clear, actionable error messages with helpful hints.

### Type Mismatch

XVR validates explicit type annotations:

```xvr
var x: int = 1.5;    // error: type mismatch: cannot convert from 'float' to 'int'
var x: string = 123;  // error: type mismatch: cannot convert from 'int' to 'string'
var x: bool = 1;      // error: type mismatch: cannot convert from 'int' to 'bool'
```

### Non-Boolean Conditions

Control flow conditions must be boolean:

```xvr
var x = 5;
if (x) { }  // ERROR
```

Error output:
```
error: condition of if statement must be boolean, got 'i32'
help: use a comparison operator (e.g., 'x > 0') or wrap the condition with 'bool()'
```

### Break/Continue Outside Loop

```xvr
if (true) {
    break;  // ERROR
}
```

Error output:
```
error: break statement must be inside a loop
help: place the 'break' statement inside a 'while' or 'for' loop
```

### Maximum Loop Nesting

Loops can be nested up to 64 levels deep:

```xvr
error: maximum loop nesting depth (64) exceeded
help: simplify nested loop structure
```

## Building

### Requirements

- LLVM 21+ (with C API headers)
- C compiler with C18 support

### Build

```bash
make
```

Output: `out/xvr`

## Differences from Interpreter

- **AOT only**: No interpreter, no REPL
- **Format strings**: Uses `{}` instead of printf `%` syntax
- **Variables**: Use `var` keyword (not `let`)
- **Semicolons**: Optional (like Rust/C++)

## Future Enhancements

- Function declarations
- Struct types
- Better optimization passes
- Multiple return values
