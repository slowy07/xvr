# XVR Compilation Guide

## Table of Contents

1. [Quick Start](#quick-start)
2. [CMake Build](#cmake-build)
3. [Compilation Flow](#compilation-flow)
4. [Build Commands](#build-commands)
5. [Emit Options](#emit-options)
6. [Intel Syntax](#intel-syntax)
7. [Running Compiled Programs](#running-compiled-programs)
8. [Testing](#testing)
9. [Troubleshooting](#troubleshooting)

---

## Quick Start

```bash
# Configure and build (compiler only - fast)
cmake -S . -B build -DXVR_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4

# Or build with tests
cmake -S . -B build -DXVR_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4

# Compile and run
./build/xvr code/hello.xvr -r

# Compile to executable
./build/xvr code/hello.xvr -o hello

# Emit assembly (ATT syntax - default)
./build/xvr code/hello.xvr --emit asm -o hello.s

# Emit assembly (Intel syntax)
./build/xvr code/hello.xvr --emit asm --asm-syntax intel -o hello.s
```

---

## CMake Build

### Build Without Tests (Recommended for Development)

```bash
# Configure - skips Catch2 FetchContent (much faster)
cmake -S . -B build -DXVR_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j$(nproc)
```

### Build With Tests

```bash
# Configure with tests enabled (will download Catch2)
cmake -S . -B build -DXVR_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j$(nproc)

# Run tests
./build/xvr_test_all
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build type: Debug, Release | Debug |
| `XVR_BUILD_TESTS` | Build test suite | OFF |
| `XVR_BUILD_SHARED` | Build shared library | ON |
| `XVR_BUILD_STATIC` | Build static library | ON |
| `XVR_SANITIZE` | Sanitizer: address, undefined, thread, all | (none) |

### Release Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Build with Sanitizers

```bash
cmake -S . -B build -DXVR_SANITIZE=address -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
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

The Intel syntax emission uses LLVM's `llc` compiler with the `--x86-asm-syntax=intel` flag to generate native Intel syntax assembly.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Intel Syntax Emission Pipeline                           │
└─────────────────────────────────────────────────────────────────────────────┘

  XVR Source          LLVM IR              llc (Intel)              Intel Assembly
  ──────────          ───────              ──────────              ─────────────
     │                  │                      │                        │
     ▼                  ▼                      ▼                        ▼
  ┌──────┐        ┌─────────┐         ┌──────────────┐       ┌──────────────┐
  │Lexer │        │   LLVM  │         │     llc      │       │    Native    │
  │      │───────▶│  IR Gen │───────▶ │--x86-asm-    │──────▶│    Intel     │
  └──────┘        │         │         │ syntax=intel │       │    Output    │
                  └─────────┘         └──────────────┘       └──────────────┘
```

### Key Features

- **Native Intel Syntax**: Uses `llc` with `--x86-asm-syntax=intel`
- **Proper Directive**: Includes `.intel_syntax noprefix`
- **Toolchain Compatible**: Works with standard gcc/clang

### Example Conversion

#### Input (XVR)
```xvr
std::println("Hello, World!");
```

#### Intel Syntax (--asm-syntax intel)
```asm
	.intel_syntax noprefix
	.file	"test"
	.text
	.globl	main
	.p2align	4
	.type	main,@function
main:
	.cfi_startproc
# %bb.0:
	push	rax
	.cfi_def_cfa_offset 16
	mov	edi, offset .Lfmt_str
	xor	eax, eax
	call	printf@PLT
	xor	eax, eax
	pop	rcx
	.cfi_def_cfa_offset 8
	ret
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
	.type	.Lfmt_str,@object
	.section	.rodata.str1.1,"aMS",@progbits,1
.Lfmt_str:
	.asciz	"Hello, World!\n"
	.size	.Lfmt_str, 15
	.section	".note.GNU-stack","",@progbits
```

### Key Differences

| Aspect | AT&T | Intel |
|--------|------|-------|
| Register | `%rax` | `rax` |
| Immediate | `$42` | `42` |
| Memory | `(%rbp)` | `[rbp]` |
| Offset | `-8(%rbp)` | `[rbp - 8]` |
| Order | `movq src, dest` | `movq dest, src` |
| Directive | (none) | `.intel_syntax noprefix` |

### Compilation

To compile Intel syntax assembly with gcc:

```bash
# Generate Intel assembly
xvr source.xvr --emit asm --asm-syntax intel -o source.s

# Compile with gcc (requires -no-pie for PIE compatibility)
gcc -no-pie source.s -o source -lm -lpthread -lxml2

# Run
./source
```

> **Note**: The `-no-pie` flag is required because the generated assembly uses relocations that are incompatible with PIE (Position Independent Executable) by default.

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

## Testing

### Run Unit Tests

```bash
# First ensure tests are built with XVR_BUILD_TESTS=ON
cmake -S . -B build -DXVR_BUILD_TESTS=ON
cmake --build build -j$(nproc)

# Create symlink for tests
ln -sf build/xvr ./xvr
ln -sf build/xvr_test_all ./xvr_test_all

# Run all tests
./xvr_test_all

# Run with verbose output
./xvr_test_all -s --verbosity high

# Run specific test tags
./xvr_test_all "[lexer]"
./xvr_test_all "[parser]"
./xvr_test_all "[llvm]"
./xvr_test_all "[std]"
```

### Run Fuzzer Tests

```bash
# Build compiler first
cmake -S . -B build -DXVR_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run fuzzer corpus
cd fuzzer && ./run_fuzzer.sh
```

### Test Tags

| Tag | Description |
|-----|-------------|
| `[unit]` | Unit tests |
| `[lexer]` | Lexer tests |
| `[parser]` | Parser tests |
| `[llvm]` | LLVM backend tests |
| `[std]` | Standard library tests |

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
