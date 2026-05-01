# Stage0 Bootstrap Compiler - Test Suite

This directory contains unit tests for the stage0 bootstrap compiler.

## Test Structure

- `test_lexer_stage0.xvr` - Lexer unit tests
- `test_parser_stage0.xvr` - Parser unit tests  
- `test_codegen_stage0.xvr` - Codegen unit tests
- `test_stage0_all.xvr` - Integration tests
- `test_runner.xvr` - Test runner

## Running Tests

### Prerequisites

Build the main XVR compiler:
```bash
cmake --build build -j$(nproc)
```

### Run All Tests

```bash
# From project root
./stage0/run_tests.sh
```

### Run Individual Tests

```bash
# Lexer tests
./build/xvr stage0/tests/test_lexer_stage0.xvr -o stage0/tests/test_lexer
./stage0/tests/test_lexer

# Parser tests  
./build/xvr stage0/tests/test_parser_stage0.xvr -o stage0/tests/test_parser
./stage0/tests/test_parser

# Codegen tests
./build/xvr stage0/tests/test_codegen_stage0.xvr -o stage0/tests/test_codegen
./stage0/tests/test_codegen

# Integration tests
./build/xvr stage0/tests/test_stage0_all.xvr -o stage0/tests/test_all
./stage0/tests/test_all
```

## Test Coverage

- [ ] Token types and keywords
- [ ] Integer/float/string literals
- [ ] Identifiers and operators
- [ ] Variable declarations
- [ ] Control flow (if/while/for)
- [ ] Function definitions
- [ ] Struct definitions
- [ ] LLVM IR generation
- [ ] End-to-end compilation

## CI Pipeline

The stage0 test suite runs automatically on:
- Every push to `stage0` or `main` branch
- Every pull request to `stage0` or `main`

See `.github/workflows/stage0.yml` for the full CI configuration.