# Contributing to XVR

Thank you for contributing to XVR Programming Language!

## Quick Start

```sh
# Clone and build
git clone https://github.com/anomalyco/xvr.git
cd xvr
make

# Run the compiler
./out/xvr your_program.xvr
```

## Code Style

- **Indentation**: Use **4 spaces** for all indentation
- **Line Length**: Keep lines under 100 characters when possible
- **Editor Config**: Configure your editor to insert 4 spaces when pressing `tab`

### Editor Examples

**VS Code** (`.vscode/settings.json`):
```json
{
    "editor.insertSpaces": true,
    "editor.tabSize": 4,
    "editor.trimAutoWhitespace": true,
    "files.insertFinalNewline": true,
    "files.trimTrailingWhitespace": true
}
```

**Vim** (`.vimrc`):
```vimrc
set tabstop=4
set shiftwidth=4
set expandtab
```

## Project Structure

```
xvr/
├── src/                    # Core source files
│   ├── backend/            # LLVM AOT backend
│   │   ├── xvr_llvm_codegen.c       # Main code generator
│   │   ├── xvr_llvm_control_flow.c # If/while/for handling
│   │   ├── xvr_llvm_expression_emitter.c # Expression codegen
│   │   └── *.c/*.h                  # LLVM infrastructure
│   ├── xvr_*.c              # Parser, lexer, AST, etc.
│   └── xvr_common.h        # Version info, common definitions
├── compiler/               # Compiler executable source
├── test/                   # Unit tests
├── fuzzer/                 # Fuzz testing scripts
│   ├── fuzz_test_suite.sh  # Edge case tests
│   ├── stress_test.sh      # Random input tests
│   └── corpus/            # Valid test inputs
├── code/                   # Example XVR programs
├── docs/                   # Documentation
└── out/                    # Build output (generated)
```

## Building

```sh
# Build everything (compiler, library, tests)
make

# Build just the compiler
make -C compiler

# Clean build artifacts
make clean
```

### Debug Build Options

```sh
# Enable debug mode with assertions
make DEBUG=1

# Enable AddressSanitizer (detect memory errors)
make DEBUG=1 SANITIZE=address

# Enable UBSan (undefined behavior)
make DEBUG=1 SANITIZE=undefined

# Enable both ASan and UBSan
make DEBUG=1 SANITIZE=address SANITIZE=undefined

# Enable profiling support
make PROFILE=1

# Show all build commands
make VERBOSE=1
```

## Testing

### Running Tests

```sh
# Run unit tests
make -C test

# Run fuzzer test suite (finds edge case bugs)
bash fuzzer/fuzz_test_suite.sh

# Run stress tests (500+ random inputs)
bash fuzzer/stress_test.sh
```

### Test Structure

- `test/test_all.c` - Main test runner
- `test/test_*.c` - Individual test modules (lexer, parser, stdlib, etc.)
- `fuzzer/fuzz_test_suite.sh` - Edge case fuzzer
- `fuzzer/stress_test.sh` - Random stress testing
- `fuzzer/corpus/` - Valid test inputs for fuzzing

### Adding Tests

1. Add your test function to the appropriate `test_*.c` file
2. Register it in `test_all.c`
3. Rebuild and run tests

### Fuzz Testing

The fuzzer tests edge cases and potential bug triggers:

- Array out-of-bounds access
- Division by zero
- Type mismatches
- Infinite recursion
- Undefined variables
- And more...

Run the full fuzzer suite:
```sh
bash fuzzer/fuzz_test_suite.sh
```

### Code Examples

The `code/` directory contains example XVR programs demonstrating features:

```sh
# Run an example
./out/xvr code/conditional.xvr
./out/xvr code/loop_while.xvr
```

## Error Handling

Use the color-coded error macros:

```c
#include "xvr_console_colors.h"

fprintf(stderr, XVR_CC_ERROR "Error: %s\n" XVR_CC_RESET, message);
fprintf(stderr, XVR_CC_NOTICE "Notice: %s\n" XVR_CC_RESET, message);
```

## Making Changes

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/your-feature`)
3. **Make** your changes following the code style
4. **Test** your changes:
   ```sh
   make                           # Build
   bash fuzzer/fuzz_test_suite.sh # Run fuzzer
   bash fuzzer/stress_test.sh     # Run stress tests
   ```
5. **Commit** with clear messages
6. **Push** to your fork
7. **Submit** a pull request

### Commit Messages

Use clear, descriptive commit messages:
- `fix: resolve segfault in codegen`
- `add: type mismatch error detection`
- `update: Windows installation docs`
- `docs: update documentations`

## Adding New Features

### New AST Nodes

1. Add enum value to `xvr_ast_node.h` (`Xvr_ASTNodeType`)
2. Add struct definition in `xvr_ast_node.h`
3. Add case to `Xvr_createASTNode()` in `xvr_ast_node.c`
4. Add case to `Xvr_freeASTNode()` in `xvr_ast_node.c`
5. Add parser handling in `xvr_parser.c`
6. Add codegen in LLVM backend if needed

### New Backend Operations

1. Add opcode to `xvr_opcodes.h`
2. Add parser handling
3. Add LLVM codegen in `src/backend/`

### Print/Println Behavior

When adding print functionality, note the language design:
- Only strings can be printed directly: `std::print("hello")`
- All other types require format specifiers: `std::print("{}", 42)`
- Arrays cannot be printed directly (use indexing or loops)

```xvr
// Valid
std::print("hello\n");
std::print("value: {}\n", 42);

// Invalid - will cause compile error
std::print(42);
std::print([1, 2, 3]);
```

## Reporting Bugs

Include:
- Description of the bug
- Steps to reproduce
- Expected vs actual behavior
- Environment (OS, LLVM version)
- Input file if applicable

## Code Review Criteria

- Fuzzer tests pass (`bash fuzzer/fuzz_test_suite.sh`)
- Stress tests pass (`bash fuzzer/stress_test.sh`)
- No memory leaks (run with `SANITIZE=address`)
- No compiler warnings
- Documentation updated if needed

## Topics for Contribution

Look for `// BUG`, `// TODO`, or `// FIXME` comments in the code for starting points.

---

> [!NOTE]
> This project uses LLVM for AOT compilation. The interpreter runtime has been removed - all code compiles to native executables via LLVM.
