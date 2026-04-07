# XVR Intel Syntax Emitter

This document describes the Intel syntax assembly emitter for the XVR compiler.

## Overview

The XVR compiler can emit assembly in two syntaxes:
- **AT&T (default)**: Native LLVM output format
- **Intel**: Native Intel syntax via `llc --x86-asm-syntax=intel`

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
  │Lexer │        │   LLVM  │         │   Target   │       │     llc      │
  │      │───────▶│  IR Gen │───────▶ │   Emitter  │─────▶ │(native intel)│
  └──────┘        │         │         │(LLVMTarget │       └──────────────┘
                  └─────────┘         │  Machine)  │
                                      └────────────┘
```

## Implementation Details

### Emission Flow

1. **XVR Source** → Parser → AST → LLVM IR
2. **LLVM IR** → `LLVMTargetMachineEmitToFile()` (AT&T)
3. **For Intel syntax**: Invoke `llc --x86-asm-syntax=intel`

### Key Code Locations

| File | Purpose |
|------|---------|
| `src/xvr_common.c:175-189` | CLI argument parsing and validation |
| `src/backend/xvr_llvm_target.c:238-280` | Assembly emission with Intel syntax detection |

### CLI Arguments

```bash
# Emit Intel syntax assembly
xvr script.xvr --emit asm --asm-syntax intel -o program.s

# Emit AT&T syntax assembly (default)
xvr script.xvr --emit asm -o program.s

# Explicit AT&T syntax
xvr script.xvr --emit asm --asm-syntax att -o program.s
```

### Validation

Invalid `--asm-syntax` values are rejected with a clear error:

```
error: invalid asm-syntax 'invalid', must be 'intel' or 'att'
```

## Usage Examples

### Basic Compilation

```bash
# Compile to Intel syntax assembly
xvr program.xvr --emit asm --asm-syntax intel -o program.s

# Compile with gcc (requires -no-pie for PIE issues)
gcc -no-pie program.s -o program -lm -lpthread -lxml2
./program
```

### Multiple Files

```bash
# Compile library code
xvr library.xvr --emit asm --asm-syntax intel -o library.s

# Compile main program
xvr main.xvr --emit asm --asm-syntax intel -o main.s

# Link together
gcc -no-pie main.s library.s -o app -lm -lpthread -lxml2
```

### Example Output

**Input (XVR Source):**
```xvr
std::println("Hello, World!");
```

**Generated Intel Syntax Assembly:**
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

## Features

### Native Intel Syntax

- Uses `llc --x86-asm-syntax=intel` for proper Intel syntax
- Includes `.intel_syntax noprefix` directive
- Compatible with standard toolchains (gcc, clang)

### Proper Register Naming

- Intel format: `rax`, `rsp`, `rbp` (without `%` prefix)
- Proper instruction suffixes (e.g., `mov`, not `movq`)

### Memory Operands

- Intel format: `[rbp - 8]` instead of `-8(%rbp)`
- Proper RIP-relative addressing: `offset .Lfmt_str[rip]`

## Compilation Notes

### PIE Compatibility

When linking with gcc, you may need `-no-pie` flag:

```bash
gcc -no-pie program.s -o program -lm -lpthread -lxml2
```

This is required because the generated assembly uses non-PIE compatible relocations.

### Linking

The XVR runtime requires these libraries:
- `-lm` (math)
- `-lpthread` (threading)
- `-lxml2` (XML support)

## Debugging

To debug or compare outputs:

```bash
# Generate both syntaxes
xvr script.xvr --emit asm -o att.s
xvr script.xvr --emit asm --asm-syntax intel -o intel.s

# Compare
diff att.s intel.s
```

## Limitations

1. Requires `llc` to be available in PATH
2. Some relocation types may require `-no-pie` flag
3. Debug info (.loc directives) not preserved in Intel mode

## Error Handling

If `llc` fails:
- Error message printed to stderr
- Returns failure code
- No partial output written

If assembly syntax is invalid:
- Clear error message with valid options
- Exits with error code
