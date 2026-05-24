# Stage0 Bootstrap Compiler - Test Suite

This directory contains unit tests for the stage0 bootstrap compiler.

## Test Structure

- `test_helpers.xvr` — Shared assertion utilities (`assertEq`, `assertStr`, `assertTrue`)
- `test_lexer_stage0.xvr` — Lexer unit tests (token types, keywords, literals, operators)
- `test_lexer_ops.xvr` — Multi-character operator tests (`==`, `!=`, `<=`, `>=`, `&&`, `||`, `->`, `..`)
- `test_parser_stage0.xvr` — Parser unit tests (var decl, if, while, return)
- `test_parser_expr.xvr` — Expression parser tests (binary ops, precedence)
- `test_codegen_stage0.xvr` — Codegen unit tests (function, if, while IR generation)
- `test_struct.xvr` — Struct definition and codegen tests
- `test_stage0_all.xvr` — Integration tests (lex → parse → codegen pipeline)
- `test_runner.xvr` — Test runner metadata

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

### Run Self-Host Test

```bash
# From stage0 directory
./test_self_host.sh
```

This verifies the full toolchain: build → run → IR validation.

## Test Coverage

- [x] Token types and keyword recognition
- [x] Integer/float/string literals
- [x] Identifiers and operators (single and multi-char)
- [x] Variable declarations
- [x] Control flow (if/while/return)
- [x] Function definitions (codegen)
- [x] Struct definitions (codegen)
- [x] Expression parsing (binary ops, precedence)
- [x] LLVM IR generation (module, function, if/while)
- [x] End-to-end compilation pipeline (lex → parse → codegen)
- [ ] Self-host roundtrip (stage0 compiling itself) — WIP, blocked by LLVM codegen segfault

## Known Issues

- Runtime segfault in compiled test binaries (pre-existing LLVM codegen bug, tracked in `test_self_host.sh`)

## CI Pipeline

The stage0 test suite runs automatically on:
- Every push to `stage0` or `main` branch
- Every pull request to `stage0` or `main`

See `.github/workflows/stage0.yml` for the full CI configuration.
