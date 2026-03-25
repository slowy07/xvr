# XVR Compiler - Test Coverage Report

## Test Suites

### 1. Unit Tests (`make test`)
- `test_all` - Combined test suite
- `test_ast_node` - AST node tests
- `test_lexer` - Lexer tests
- `test_parser` - Parser tests
- `test_compiler` - Compiler tests
- `test_literal` - Literal value tests
- `test_memory` - Memory management tests
- `test_scope` - Scope tests
- `test_llvm_backend` - LLVM backend tests
- `test_std` - Standard library tests

### 2. Fuzzing Tests (`fuzzer/fuzz_test_suite.sh`)
- 72+ edge case tests covering:
  - Empty/malformed input handling
  - String parsing edge cases
  - Parenthesized expressions
  - Type mismatches
  - Array operations
  - Function call arguments
  - Control flow edge cases
  - String operations
  - Boolean operations
  - Array literals
  - Ternary expressions
  - Loop edge cases

### 3. Stress Tests (`fuzzer/stress_test.sh`)
- 720+ random input tests covering:
  - Nested parentheses
  - Deep nesting
  - Multiple function definitions
  - Nested function calls
  - Boolean operations
  - String concatenation
  - Array operations
  - Ternary expressions
  - Loop constructs
  - Type casts
  - Compound assignments
  - Edge cases (division by zero, out of bounds, etc.)

### 4. Regression Tests (`test/regression_tests.sh`)
- 18 tests for bugs that have been fixed:
  - Function call arguments at module level
  - Runtime string concatenation
  - GROUPING expression unwrapping
  - std::print with non-literal arguments
  - Compile-time constant folding
  - Function parameter allocation
  - Format strings
  - Nested function calls
  - Ternary expressions
  - Boolean expressions
  - Self-referencing variable assignment
  - Array indexing

### 5. Corpus Tests
- Tests all files in `fuzzer/corpus/` directory
- Tests both valid and intentionally invalid inputs

## Bug Categories Tested

### Parsing
- [x] Nested parentheses
- [x] Unbalanced parentheses
- [x] Unterminated strings
- [x] Special characters in strings
- [x] Unicode strings
- [x] Empty inputs

### Type System
- [x] Type mismatches
- [x] Implicit conversions
- [x] Array type compatibility
- [x] String vs number operations

### Code Generation
- [x] Function call arguments
- [x] Return value handling
- [x] Variable allocation
- [x] String concatenation
- [x] Array indexing

### Runtime Safety
- [x] Division by zero
- [x] Array out of bounds
- [x] Undefined variables
- [x] Undefined functions

### Control Flow
- [x] Nested conditionals
- [x] Loop breaks/continues
- [x] Early returns
- [x] Missing returns

## Test Results

| Test Suite | Passed | Failed | Crashes |
|------------|--------|--------|---------|
| Unit Tests | ✓ | 0 | 0 |
| Fuzzing Tests | 72 | 0 | 0 |
| Stress Tests | 720 | 0 | 0 |
| Regression Tests | 18 | 0 | 0 |
| Corpus Tests | ✓ | varies | 0 |

## Known Limitations

1. **Runtime errors** (division by zero, array out of bounds) are not caught at compile time but may cause runtime crashes
2. **Undefined behavior** (accessing uninitialized variables) is not detected
3. **Memory leaks** are not detected without ASAN/MSAN

## Adding New Tests

### Regression Tests
Add new tests to `test/regression_tests.sh`:
```bash
run_test "test_name" 'xvr code here'
```

### Fuzzing Tests
Add new tests to `fuzzer/fuzz_test_suite.sh`:
```bash
run_test "test_name" "input code"
```

### Stress Tests
Add new patterns to `fuzzer/stress_test.sh`:
```bash
patterns=(
    "new pattern"
)
```

## CI Integration

All tests are automatically run in GitHub Actions:
- `.github/workflows/ci.yml` - Basic CI
- `.github/workflows/fuzzer.yml` - Comprehensive fuzzing CI
