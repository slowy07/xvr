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
                            └───────▶│    Linker       │────────┘
                                     │   (system ld)   │
                                     └─────────────────┘

Module Loading Flow:
┌─────────────┐    ┌──────────────────┐    ┌─────────────┐    ┌──────────────────┐
│ include std;│───▶│ ModuleResolver   │───▶│ Load .xvr   │───▶│ Merge AST Nodes  │
│  (Import)   │    │ (path resolution)│    │ (parse)     │    │ (into codegen)   │
└─────────────┘    └──────────────────┘    └─────────────┘    └──────────────────┘
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
│  ┌─────────────────────────────────────────────────────────────────────────────────┐    │
│  │                              Frontend                                           │    │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────────────────┐ │     │
│  │  │   Lexer     │───▶│   Parser    │───▶│           Semantic Analysis          │ │    │
│  │  │ xvr_lexer.c │    │xvr_parser.c │    │  ┌─────────────────────────────────┐ │ │    │
│  │  └─────────────┘    └─────────────┘    │  │     BuiltinRegistry            │ │ │     │
│  │                                        │  │  (sizeof, len, panic)          │ │ │     │
│  │                                        │  └─────────────────────────────────┘ │ │    │
│  │                                        │  ┌─────────────────────────────────┐ │ │    │
│  │                                        │  │     ModuleResolver             │ │ │     │
│  │                                        │  │  (include std; module loading)│ │ │      │
│  │                                        │  └─────────────────────────────────┘ │ │    │
│  │                                        └─────────────────────────────────────┘ │     │
│  └─────────────────────────────────────────────────────────────────────────────────┘    │
│                                         │                                               │
│                                         ▼                                               │
│  ┌─────────────────────────────────────────────────────────────────────────────────────────┐
│  │                                    LLVM Backend                                      │
│  │                                                                                         │
│  │  ┌─────────────────────────────────────────┐                                          │
│  │  │         xvr_llvm_codegen.c               │                                          │
│  │  │              (Coordinator)               │                                          │
│  │  └──────────────────┬──────────────────────┘                                          │
│  │                     │                                                                │
│  │    ┌────────────────┼────────────────┐                                                │
│  │    ▼                ▼                ▼                                                │
│  │ ┌──────────┐ ┌──────────┐ ┌──────────────┐                                             │
│  │ │Context   │ │Type      │ │Module        │                                             │
│  │ │Manager   │ │Mapper    │ │Manager       │                                             │
│  │ │.c        │ │.c        │ │.c            │                                             │
│  │ └──────────┘ └──────────┘ └──────────────┘                                             │
│  │ ┌──────────┐ ┌──────────┐ ┌──────────────┐                                             │
│  │ │IR        │ │Expression │ │Function     │                                             │
│  │ │Builder   │ │Emitter   │ │Emitter       │                                             │
│  │ │.c        │ │.c        │ │.c            │                                             │
│  │ └──────────┘ └──────────┘ └──────────────┘                                             │
│  │ ┌──────────┐ ┌──────────┐ ┌──────────────┐                                             │
│  │ │Control   │ │Optimizer │ │Target        │                                             │
│  │ │Flow      │ │.c        │ │.c            │                                             │
│  │ │.c        │ │          │ │              │                                             │
│  │ └──────────┘ └──────────┘ └──────────────┘                                             │
│  └─────────────────────────────────────────────────────────────────────────────────────────┘
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
  │  BINARY  │────────▶│ xvr_llvm_    │──────▶│ %add =    │
  │ x + y    │         │ expression   │       │ add i32   │
  └──────────┘         │ emitter.c    │       │ %x, %y    │
                       └──────────────┘       └───────────┘

  ┌──────────┐         ┌──────────────┐       ┌───────────┐
  │  IF/     │────────▶│ xvr_llvm_    │──────▶│ br i1 %   │
  │  WHILE   │         │ control_flow │       │ cond,     │
  │          │         │ .c           │       │ label,    │
  └──────────┘         └──────────────┘       │ label     │
                                              └───────────┘

Module Loading Flow:
  ┌────────────┐         ┌──────────────────┐       ┌────────────┐
  │ XVR_AST_   │────────▶│ ModuleResolver   │──────▶│ Parse .xvr │
  │ NODE_IMPORT│         │ Resolve(module)  │       │ file       │
  └────────────┘         └──────────────────┘       └─────┬──────┘
                                                          │
                                                          ▼
                                               ┌──────────────────┐
                                               │ Merge AST into   │
                                               │ main program     │
                                               └──────────────────┘
```

## Source File Organization

```
src/
├── backend/                      # LLVM code generation
│   ├── xvr_llvm_codegen.h/.c         # Main coordinator, entry points
│   ├── xvr_llvm_context.h/.c         # LLVM context management
│   ├── xvr_llvm_type_mapper.h/.c    # Type mapping (XVR → LLVM types)
│   ├── xvr_llvm_module_manager.h/.c # Module creation & IR printing
│   ├── xvr_llvm_ir_builder.h/.c    # IR building (alloca, store, load, etc.)
│   ├── xvr_llvm_expression_emitter.h/.c # Expressions, arrays, indexing
│   ├── xvr_llvm_function_emitter.h/.c   # Function definitions
│   ├── xvr_llvm_control_flow.h/.c      # If/while/for generation
│   ├── xvr_llvm_optimizer.h/.c       # Optimization passes
│   ├── xvr_llvm_target.h/.c          # Target machine configuration
│   └── xvr_format_string.h/.c        # Format string {} parser
│
└── sema/                          # Semantic analysis & builtins
    └── xvr_builtin.h/.c           # Builtin registry & module resolver
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


#### Statement Form

```xvr
var score: int32 = 85;

if (score >= 90) {
    std::print("Grade: A");
} else if (score >= 80) {
    std::print("Grade: B");
} else {
    std::print("Grade: C or lower");
}
```


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

std::print(grade);
```

**Note:** Expression-based `if` requires explicit type annotation on the variable.

### While Loops

```xvr
var i = 0;
while (i < 10) {
    std::print("{}", i);
    i = i + 1;
}
```

### For Loops

The `for` loop is ideal when you know the number of iterations:

```xvr
// Basic for loop
for (var i = 0; i < 5; i++) {
    std::print("{}", i);  // 0, 1, 2, 3, 4
}

// With std::print
include std;
for (var i = 0; i < 20; i++) {
    std::print("{}\n", i);
}
```

**Syntax:**
```xvr
for (init; condition; increment) {
    // body
}
```

| Part | Description |
|------|-------------|
| `init` | Variable initialization (e.g., `var i = 0`) |
| `condition` | Boolean expression evaluated before each iteration |
| `increment` | Executed after each iteration (e.g., `i++`, `i--`) |

### Increment/Decrement Operators

| Operator | Description |
|----------|-------------|
| `++` | Increment by 1 |
| `--` | Decrement by 1 |

```xvr
// Count up
for (var i = 0; i < 5; i++) { }  // 0, 1, 2, 3, 4

// Count down
for (var i = 5; i > 0; i--) { }  // 5, 4, 3, 2, 1
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
    std::print("{}", i);  // Only prints odd numbers
}
```

**Conditions must be boolean** - the compiler validates this and provides helpful hints:

```xvr
var x = 5;
if (x) { }  // ERROR: condition must be boolean

### Print with Format Strings

XVR uses `{}` placeholders:

```xvr
var name = "world";
var num = 42;

std::print("Hello, {}!", name);      // Hello, world!
std::print("Value: {}", num);         // Value: 42
std::print("{} + {} = {}", 1, 2, 3); // 1 + 2 = 3

// Direct array printing
var arr = [1, 2, 3];
std::print(arr);                      // prints: 1 2 3
```

### While Loops

```xvr
var i = 0;
while (i < 10) {
    std::print("{}", i);
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
    std::print("{}", i);  // Only prints odd numbers
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
std::print(arr[0]);  // prints 1

// Array assignment
arr[1] = 20;
std::print(arr[1]);  // prints 20
```

### String Concatenation

XVR supports compile-time and runtime string concatenation:

```xvr
// Compile-time concatenation (both operands are string literals)
var msg = "Hello, " + "World!";  // "Hello, World!" at compile time

// Runtime concatenation (at least one operand is runtime)
var name = "John";
var greeting = "Hello, " + name;  // uses string_concat runtime proc
```

**Optimization:** When both operands are string literals, the compiler constant-folds them into a single string at compile time. When at least one operand is a runtime value (like a variable or function parameter), it falls back to the `string_concat` runtime procedure.

## Builtin Registry System

XVR implements a builtin function registry for compiler-level functions:

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                          BuiltinRegistry & ModuleResolver                               │
└────────────────────────────────────────────────────────────────────────────────────────┘

                         ┌─────────────────────┐
                         │  BuiltinRegistry    │
                         ├─────────────────────┤
                         │ sizeof(T)           │────┐
                         │ len(x)              │    │
                         │ panic(msg)          │    │
                         └─────────────────────┘    │
                                                   ▼
                                      ┌─────────────────────┐
                                      │  Codegen Handler    │
                                      │  (callback for each)│
                                      └─────────────────────┘

                         ┌─────────────────────┐
                         │  ModuleResolver     │
                         ├─────────────────────┤
                         │ stdlib_path: ./lib/std/
                         └─────────────────────┘
                                │
           ┌────────────────────┼────────────────────┐
           │                    │                    │
           ▼                    ▼                    ▼
    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
    │ include std; │    │ include io;  │    │include math; │
    │   (import)   │    │   (import)   │    │   (import)   │
    └──────┬───────┘    └──────┬───────┘    └──────┬───────┘
           │                   │                   │
           ▼                   ▼                   ▼
    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
    │ lib/std/io   │    │ lib/std/io   │    │ lib/std/math │
    │   .xvr      │    │   .xvr       │    │   .xvr       │
    └──────────────┘    └──────────────┘    └──────────────┘
```

### Module Resolution Flow

```
Source Code                      Compiler                    Filesystem
────────────                     ────────                    ─────────

include std;            ─────▶  ModuleResolver
   │                          Resolve("std")
   │                          └─ path: ./lib/std/std.xvr
   │                                │
   │                                ▼
   │                          read_file()  ─────────────────────▶ ./lib/std/std.xvr
   │                                │
   │                                ▼ 
   │                          parse_to_ast()
   │                                │
   │                                ▼ 
   │                          merge AST nodes into 
   │                          main compilation
   │                                │
   ▼                                ▼
┌──────────────┐              ┌──────────────┐
│ std::println │    ─────▶    │ Compiled     │
│   "Hello"    │              │   Output     │
└──────────────┘              └──────────────┘
```

#### BuiltinRegistry API

```c
// Create/destroy registry
Xvr_BuiltinRegistry* Xvr_BuiltinRegistryCreate(void);
void Xvr_BuiltinRegistryDestroy(Xvr_BuiltinRegistry* registry);

// Register a builtin function
bool Xvr_BuiltinRegistryRegister(Xvr_BuiltinRegistry* registry,
    const char* name, Xvr_BuiltinType type,
    Xvr_BuiltinHandler handler, const char* llvm_name,
    int min_args, int max_args);

// Lookup builtin
Xvr_BuiltinInfo* Xvr_BuiltinRegistryLookup(Xvr_BuiltinRegistry* registry,
    const char* name);
```

#### ModuleResolver API

```c
// Create/destroy resolver
Xvr_ModuleResolver* Xvr_ModuleResolverCreate(const char* stdlib_path);
void Xvr_ModuleResolverDestroy(Xvr_ModuleResolver* resolver);

// Resolve module name to file path
bool Xvr_ModuleResolverResolve(Xvr_ModuleResolver* resolver,
    const char* module_name, char** out_path);

// Load module AST nodes
bool Xvr_ModuleResolverLoadModule(Xvr_ModuleResolver* resolver,
    const char* module_path, Xvr_ASTNode*** out_nodes, int* out_count);
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
- **Semicolons**: Optional
- **Procedures**: Use `proc` keyword with optional return type annotation

## Future Enhancements

- Struct types
- Better optimization passes
- Multiple return values
- Additional builtin functions (len, panic)
- Module caching for faster compilation
- Standard library expansion (math, string, memory, fs modules)
