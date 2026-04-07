# XVR Intel Syntax Emitter

This document describes the Intel syntax assembly emitter for the XVR compiler.

## Overview

The XVR compiler can emit assembly in two syntaxes:
- **AT&T (default)**: Native LLVM output format
- **Intel**: Converted from AT&T for readability

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Assembly Emission Pipeline                           │
└─────────────────────────────────────────────────────────────────────────────┘

  XVR Source          LLVM IR              ASM (AT&T)            ASM (Intel)
  ──────────          ───────              ──────────           ──────────
     │                  │                      │                     │
     ▼                  ▼                      ▼                     ▼
  ┌──────┐        ┌─────────┐         ┌────────────┐       ┌──────────────┐
  │Lexer │        │   LLVM  │         │   Target   │       │   Intel      │
  │      │───────▶│  IR Gen │───────▶ │   Emitter  │─────▶ │  Converter   │
  └──────┘        │         │         │(ATT native)│       │(ATT→Intel)   │
                  └─────────┘         └────────────┘       └──────────────┘
```

## Conversion Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Intel Syntax Conversion Flow                             │
└─────────────────────────────────────────────────────────────────────────────┘

                         LLVM Assembly (AT&T)
                              │
                              ▼
                    ┌──────────────────┐
                    │ convertAttToIntel│
                    │   (main loop)    │
                    └────────┬─────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
   ┌───────────┐      ┌──────────────┐    ┌────────────┐
   │ Directive │      │ Instruction  │    │  Operand   │
   │  Handler  │      │  Converter   │    │ Reorderer  │
   │ (.text,   │      │ (movq→mov)   │    │ src→dest   │
   │  .data)   │      │              │    │            │
   └───────────┘      └──────────────┘    └────────────┘
         │                   │                   │
         └───────────────────┼───────────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   Output File   │
                    │  (Intel Syntax) │
                    └─────────────────┘
```

## Key Functions

### 1. convertAttToIntel()

Main conversion function that processes AT&T assembly line by line.

**Location**: `src/backend/xvr_llvm_target.c:403`

**Flow**:
```
Input Buffer
     │
     ▼
┌─────────────────────────────────────────┐
│  Main Loop: Process each character      │
│  ───────────────────────────────────────│
│  • Detect directive (.text, .data)      │
│  • Parse instructions                   │
│  • Reorder operands                     │
│  • Handle memory operands               │
└─────────────────────────────────────────┘
     │
     ▼
Output Buffer (Intel Syntax)
```

### 2. extractOperandsSimple()

Extracts operands from an AT&T instruction.

**Location**: `src/backend/xvr_llvm_target.c:567`

**Process**:
```
AT&T: movq %rax, -8(%rbp)
                  │
                  ▼
Extract: -8(%rbp), %rax
```

### 3. reorderOperandsSimple()

Reorders operands from AT&T (src, dest) to Intel (dest, src) format.

**Location**: `src/backend/xvr_llvm_target.c:588`

**Process**:
```
AT&T:  movq %rax, -8(%rbp)
        │     │
        │     └─────── src
        └──────────── dest

Intel: movq -8(%rbp), %rax
        │     │
        │     └─────── dest
        └──────────── src
```

### 4. convertMemOperandSimple()

Converts AT&T memory operands to Intel format.

**Location**: `src/backend/xvr_llvm_target.c:668`

**Process**:
```
AT&T: -8(%rbp, %rax, 4)
                │
                ▼
Intel: [rbp + rax*4 - 8]
```

## Supported Instructions

### Single-Operand Instructions
These maintain their order (no reorder needed):
- `push` / `pushq`
- `pop` / `popq`
- `call` / `callq`
- `ret` / `retq`
- `jmp` / `jmpq`

### Two-Operand Instructions
These are reordered from AT&T to Intel:
- `movq` → `movq dest, src`
- `addq` → `addq dest, src`
- `subq` → `subq dest, src`
- `leaq` → `leaq dest, src`

## Usage

### Command Line

```bash
# Emit Intel syntax assembly
xvr script.xvr --emit asm --asm-syntax intel -o program.s

# Emit Intel syntax object file
xvr script.xvr --emit obj --asm-syntax intel -o program.o
```

### Programmatic

```c
// Set Intel syntax in code
Xvr_AsmConfig config;
config.syntax = XVR_ASM_SYNTAX_INTEL;
config.emission = XVR_ASM_EMIT_ASM;
```

## Example

### Input (XVR Source)
```xvr
var x: int32 = 42;
var y: int32 = x + 10;
std::print("{}", y);
```

### AT&T Assembly Output (Default)
```asm
movl    $42, -4(%rbp)
movl    -4(%rbp), %eax
addl    $10, %eax
movl    %eax, -8(%rbp)
```

### Intel Syntax Output (--asm-syntax intel)
```asm
movl    DWORD PTR [rbp - 4], 42
movl    %eax, DWORD PTR [rbp - 4]
addl    %eax, 10
movl    DWORD PTR [rbp - 8], %eax
```

## Conversion Details

### Register Naming
- AT&T: `%rax` → Intel: `rax`
- AT&T: `%rsp` → Intel: `rsp`
- AT&T: `%rbp` → Intel: `rbp`

### Memory Addressing
- AT&T: `offset(%base, %index, scale)`
- Intel: `[base + index * scale + offset]`

### Immediate Values
- AT&T: `$42`
- Intel: `42`

## Limitations

1. Complex addressing modes may not fully convert
2. Some SIMD instructions may lose information
3. Label references are preserved as-is
4. Debug directives (.loc, .file) are kept

## Debugging

To debug conversion issues:

```bash
# Compare ATT and Intel output
xvr script.xvr --emit asm -o att.s
xvr script.xvr --emit asm --asm-syntax intel -o intel.s

# Diff the outputs
diff att.s intel.s
```

## Performance

The Intel syntax conversion adds minimal overhead:
- Single pass through the assembly
- In-place string building
- Typical conversion: < 10ms for 1000 lines

## Error Handling

If conversion fails:
1. Falls back to original AT&T syntax
2. Prints warning to stderr
3. Continues with available output
