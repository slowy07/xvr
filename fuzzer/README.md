# Fuzzing the XVR Compiler

This directory contains fuzzing infrastructure for the XVR compiler using AFL (American Fuzzy Lop) and libFuzzer.

## Quick Start

### Option 1: libFuzzer (Recommended)

libFuzzer is built into clang and doesn't require additional installation:

```bash
cd fuzzer
./build.sh libfuzzer
./harness/fuzz_libfuzzer corpus/
```

### Option 2: AFL/AFL++

AFL requires instrumenting the compiler:

```bash
cd fuzzer
./build.sh afl
cd build
afl-fuzz -i corpus -o output -M main -- ./out/xvr @@
```

## Requirements

### libFuzzer
- clang (or GCC with libFuzzer support)
- LLVM (for building XVR)

### AFL
- AFL++ (https://github.com/AFLplusplus/AFLplusplus)
- LLVM

## Installation

### Install AFL++

```bash
cd ~/Downloads
wget https://github.com/AFLplusplus/AFLplusplus/archive/stable.tar.gz
tar -xf stable.tar.gz
cd AFLplusplus-stable
make distrib
sudo make install
```

## Directory Structure

```
fuzzer/
├── build/              # Build configurations
│   ├── Makefile.afl    # AFL build rules
│   └── Makefile.libfuzzer
├── corpus/             # Initial test cases
│   ├── 00_minimal.xvr
│   ├── 01_arithmetic.xvr
│   └── ...
├── harness/            # Fuzzing harnesses
│   ├── fuzz_main.c    # AFL harness
│   └── fuzz_libfuzzer.c
├── build.sh           # Build script
└── README.md          # This file
```

## Test Corpus

The corpus contains minimal XVR programs covering:
- Basic variables
- Arithmetic operations
- Strings and arrays
- Control flow (if/else, while, for)
- Functions
- Print functions (print, println with format strings)

## Tips for Effective Fuzzing

1. **Expand the corpus**: Add more test cases to `corpus/` covering edge cases
2. **Use dictionaries**: Add common patterns to help fuzzer explore new code paths
3. **Monitor crashes**: Check `output/crashes/` for discovered bugs
4. **Parallel fuzzing**: Use multiple instances with AFL++ for faster coverage

## Security Considerations

- Run fuzzers in isolated environments
- Don't use sudo for fuzzing sessions
- Monitor system resources (memory, CPU)
- Review crashes manually to filter false positives