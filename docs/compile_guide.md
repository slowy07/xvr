# XVR Compilation Guide

## Table of Contents

1. [Quick Start](#quick-start)
2. [Compilation Flow](#compilation-flow)
3. [Build Commands](#build-commands)
4. [Emit Options](#emit-options)
5. [Intel Syntax](#intel-syntax)
6. [Running Compiled Programs](#running-compiled-programs)
7. [Troubleshooting](#troubleshooting)

---

## Quick Start

```bash
# Build the compiler
cd build && make -j4

# Compile and run
./xvr ../code/hello.xvr --run

# Compile to executable
./xvr ../code/hello.xvr -o hello

# Emit assembly (ATT syntax - default)
./xvr ../code/hello.xvr --emit asm -o hello.s

# Emit assembly (Intel syntax)
./xvr ../code/hello.xvr --emit asm --asm-syntax intel -o hello.s
```

---

## Compilation Flow

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        XVR Compilation Flow                                 │
└─────────────────────────────────────────────────────────────────────────────┘

   Source              Intermediate              Output
   ───────              ───────────             ──────
     │                    │                       │
     ▼                    ▼                       ▼
┌─────────┐         ┌─────────┐           ┌──────────────┐
│  .xvr   │────────▶│  LLVM   │──────────▶│  Executable  │
│  File   │         │   IR    │           │    (a.out)   │
└─────────┘         └─────────┘           └──────────────┘
                           │
                           ▼
                    ┌──────────────┐
                    │    .s / .o   │
                    │   (optional)│
                    └──────────────┘
```

### Detailed Pipeline

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Full Compilation Pipeline                           │
└─────────────────────────────────────────────────────────────────────────────┘

 .xvr Source
     │
     ▼
┌─────────────┐
│   Lexer     │  Tokenize source into tokens
│ xvr_lexer.c │  ─────────────────────────────────────
└──────┬──────┘
       │ Tokens
       ▼
┌─────────────┐
│   Parser    │  Build AST from tokens
│xvr_parser.c │  ─────────────────────────────────────
└──────┬──────┘
       │ AST Nodes
       ▼
┌─────────────┐
│  Semantic  │  Type checking, scope analysis
│ xvr_semantic│  ─────────────────────────────────────
└──────┬──────┘
       │ Validated AST
       ▼
┌─────────────┐
│    LLVM     │  Generate LLVM IR from AST
│Codegen      │  ─────────────────────────────────────
└──────┬──────┘
       │ LLVM IR
       ▼
┌─────────────┐
│  Optimizer  │  Optimize IR (with -O flags)
│  (optional) │  ─────────────────────────────────────
└──────┬──────┘
       │ Optimized IR
       ▼
┌─────────────┐
│   Target    │  Emit asm/object/executable
│  Emission   │  ─────────────────────────────────────
└─────────────┘
```

### Backend Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      LLVM Backend Components                                │
└─────────────────────────────────────────────────────────────────────────────┘

                        ┌─────────────────────┐
                        │  xvr_llvm_codegen.c │
                        │     (Coordinator)   │
                        └──────────┬──────────┘
                                   │
        ┌───────────────┬───────────┼───────────┬───────────────┐
        │               │           │           │               │
        ▼               ▼           ▼           ▼               ▼
  ┌──────────┐   ┌──────────┐ ┌──────────┐ ┌──────────┐  ┌──────────┐
  │ Context  │   │   Type   │ │  Module  │ │    IR    │  │  Target  │
  │ Manager  │   │  Mapper  │ │ Manager  │ │ Builder  │  │ Emission │
  └──────────┘   └──────────┘ └──────────┘ └──────────┘  └──────────┘
        │               │           │           │               │
        └───────────────┴───────────┴───────────┴───────────────┘
                                   │
                                   ▼
                          ┌─────────────────┐
                          │    Output       │
                          │  (asm/obj/exe)  │
                          └─────────────────┘
```

---

## Build Commands

### Basic Compilation

```bash
# Compile to executable (default)
./xvr source.xvr -o output

# Compile and run immediately
./xvr source.xvr --run
./xvr source.xvr -r

# Compile to executable, then run
./xvr source.xvr -o output && ./output
```

### Emit Options

```bash
# Emit executable (default)
./xvr source.xvr --emit exe -o output

# Emit object file
./xvr source.xvr --emit obj -o output.o

# Emit LLVM IR
./xvr source.xvr --emit ir -o output.ll

# Emit assembly (ATT syntax - default)
./xvr source.xvr --emit asm -o output.s

# Emit assembly (Intel syntax)
./xvr source.xvr --emit asm --asm-syntax intel -o output.s
```

### Build System

```bash
# Configure (with compile commands)
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j4

# Clean rebuild
make clean
make -j4
```

---

## Emit Options

### Emission Types

| Option | Description | Output |
|--------|-------------|--------|
| `--emit exe` | Executable | Native binary |
| `--emit obj` | Object file | `.o` file |
| `--emit asm` | Assembly | `.s` file |
| `--emit ir` | LLVM IR | `.ll` file |

### Assembly Syntax

| Syntax | Option | Example |
|--------|--------|---------|
| AT&T (default) | `--asm-syntax att` | `movq %rax, -8(%rbp)` |
| Intel | `--asm-syntax intel` | `movq QWORD PTR [rbp-8], rax` |

---

## Intel Syntax

### How It Works

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Intel Syntax Conversion Pipeline                         │
└─────────────────────────────────────────────────────────────────────────────┘

  LLVM Assembly (AT&T)              Intel Converter              Intel Assembly
  ─────────────────────              ──────────────              ───────────────
        │                               │                              │
        ▼                               ▼                              │
  ┌───────────────────┐        ┌─────────────────┐                     │
  │ Directive Handler │        │  convertAttTo   │                     │
  │ (.text, .data)    │───────▶│    Intel()      │                     │
  └───────────────────┘        └────────┬────────┘                     │
        │                               │                              │
        ▼                               ▼                              ▼
  ┌───────────────────┐        ┌─────────────────┐         ┌─────────────────┐
  │Instruction Parser │        │  Parse each     │         │  Intel Syntax   │
  │  (movq, addq)     │───────▶│  instruction    │────────▶│    Output       │
  └───────────────────┘        └────────┬────────┘         └─────────────────┘
        │                               │
        ▼                               ▼
  ┌───────────────────┐        ┌─────────────────┐
  │Operand Reorder    │        │Reorder: src→dest│
  │ (AT&T: src,dest)  │───────▶│(Intel: dest,src)│
  └───────────────────┘        └─────────────────┘
```

### Example Conversion

#### Input (XVR)
```xvr
var x: int32 = 42;
var y: int32 = x + 10;
std::print("{}", y);
```

#### AT&T Assembly (Default)
```asm
    .text
    .globl  _main
_main:
    pushq   %rbp
    movq    %rsp, %rbp
    subq    $16, %rsp
    movl    $42, -4(%rbp)
    movl    -4(%rbp), %eax
    addl    $10, %eax
    movl    %eax, -8(%rbp)
    leaq    L1(%rip), %rdi
    movl    -8(%rbp), %esi
    callq   _print_int
    addq    $16, %rsp
    popq    %rbp
    retq
```

#### Intel Syntax (--asm-syntax intel)
```asm
    .text
    .globl  _main
_main:
    pushq   rbp
    movq    rsp, rbp
    subq    16, rsp
    movl    DWORD PTR [rbp - 4], 42
    movl    DWORD PTR [rbp - 4], eax
    addl    eax, 10
    movl    eax, DWORD PTR [rbp - 8]
    leaq    L1(%rip), rdi
    movl    DWORD PTR [rbp - 8], esi
    callq   _print_int
    addq    16, rsp
    popq    rbp
    retq
```

### Key Differences

| Aspect | AT&T | Intel |
|--------|------|-------|
| Register | `%rax` | `rax` |
| Immediate | `$42` | `42` |
| Memory | `(%rbp)` | `[rbp]` |
| Offset | `-8(%rbp)` | `[rbp - 8]` |
| Order | `movq src, dest` | `movq dest, src` |

---

## Running Compiled Programs

### After Compilation

```bash
# Run the executable
./output

# Make executable and run
chmod +x output
./output

# Run with arguments
./output arg1 arg2

# Run with environment
VAR=value ./output
```

### Using --run Flag

```bash
# Compile and run in one step
./xvr source.xvr --run

# With arguments
./xvr source.xvr -- --arg1 arg2
```

### Debugging

```bash
# Show LLVM IR
./xvr source.xvr --emit ir -o output.ll
cat output.ll

# Show optimized IR
./xvr source.xvr -O2 --emit ir -o output.ll

# Show AST
./xvr source.xvr --dump-ast

# Show timing info
./xvr source.xvr --timing
```

---

## Troubleshooting

### Common Issues

#### 1. Compilation Errors

```bash
# Check for syntax errors
./xvr source.xvr -v  # Verbose mode
```

#### 2. Runtime Errors

```bash
# Run with debug info
./xvr source.xvr --run --debug

# Check for memory issues
valgrind ./output
```

#### 3. Intel Syntax Issues

```bash
# Compare ATT and Intel output
./xvr source.xvr --emit asm -o att.s
./xvr source.xvr --emit asm --asm-syntax intel -o intel.s
diff att.s intel.s
```

### Performance Optimization

```bash
# No optimization (fastest compile)
./xvr source.xvr -o output

# Basic optimization (-O1)
./xvr source.xvr -O1 -o output

# Standard optimization (-O2)
./xvr source.xvr -O2 -o output

# Aggressive optimization (-O3)
./xvr source.xvr -O3 -o output
```

### Build with Tests

```bash
# Run test suite
cd build
ctest
ctest --output-on-failure

# Run specific test
./xvr_test_all
```

---

## Quick Reference

```bash
# Basic
./xvr script.xvr -o script

# Run immediately  
./xvr script.xvr --run

# Assembly
./xvr script.xvr --emit asm --asm-syntax intel -o script.s

# Object file
./xvr script.xvr --emit obj -o script.o

# LLVM IR
./xvr script.xvr --emit ir -o script.ll

# With optimization
./xvr script.xvr -O2 -o script

# Verbose
./xvr script.xvr -v
```
