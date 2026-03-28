# Contributing to XVR

Thank you for contributing to XVR Programming Language!

## Quick Start

```sh
# Clone and build
git clone https://github.com/anomalyco/xvr.git
cd xvr
mkdir build && cd build
cmake .. && cmake --build .

# Run the compiler
./build/xvr your_program.xvr
```

## Code Style

- **Indentation**: Use **4 spaces** for all indentation
- **Line Length**: Keep lines under 100 characters when possible
- **Formatter**: Use `clang-format` for automatic formatting
- **Pre-commit Hook**: Automatically formats staged files on commit

### Automatic Formatting

The project includes a pre-commit hook that automatically formats C files:

```sh
# Install clang-format (if not already installed)
# Ubuntu/Debian:
sudo apt install clang-format

# macOS:
brew install clang-format

# The pre-commit hook runs automatically on git commit
# Unformatted files will be auto-formatted and re-staged
```

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
│   ├── CMakeLists.txt      # Library build config
│   ├── backend/            # LLVM AOT backend
│   │   ├── xvr_llvm_codegen.c       # Main code generator
│   │   ├── xvr_llvm_control_flow.c # If/while/for handling
│   │   ├── xvr_llvm_expression_emitter.c # Expression codegen
│   │   └── *.c/*.h                  # LLVM infrastructure
│   ├── xvr_*.c              # Parser, lexer, AST, etc.
│   └── xvr_common.h        # Version info, common definitions
├── compiler/               # Compiler executable source
│   └── CMakeLists.txt      # Compiler build config
├── test/                   # Unit tests
│   └── CMakeLists.txt      # Test build config
├── fuzzer/                 # Fuzz testing scripts
│   ├── fuzz_test_suite.sh  # Edge case tests
│   ├── stress_test.sh      # Random input tests
│   └── corpus/            # Valid test inputs
├── code/                   # Example XVR programs
├── docs/                   # Documentation
├── CMakeLists.txt          # Root build config
├── .clang-format           # Code formatting rules
├── .git/hooks/pre-commit   # Auto-format on commit
└── build/                  # Build output (generated)
```

## Building

```sh
# Create build directory
mkdir build && cd build

# Configure (Debug build by default)
cmake ..

# Build everything (compiler, library, tests)
cmake --build .

# Clean build artifacts
rm -rf build
```

### Build Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Debug, Release | Debug | Build type |
| `XVR_BUILD_TESTS` | ON, OFF | ON | Build test suite |
| `XVR_BUILD_SHARED` | ON, OFF | ON | Build shared library |
| `XVR_BUILD_STATIC` | ON, OFF | ON | Build static library |
| `XVR_SANITIZE` | address, undefined, thread, all | - | Enable sanitizers |

### Build Examples

```sh
# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Enable debug mode with assertions
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Enable AddressSanitizer (detect memory errors)
cmake -DXVR_SANITIZE=address ..

# Enable UBSan (undefined behavior)
cmake -DXVR_SANITIZE=undefined ..

# Enable both ASan and UBSan
cmake -DXVR_SANITIZE=all ..

# Build without tests
cmake -DXVR_BUILD_TESTS=OFF ..
```

## Testing

### Running Tests

```sh
# Run unit tests (from build directory)
ctest --output-on-failure

# Run all tests verbosely
ctest -V

# Run specific test
./build/xvr_test_all

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
./build/xvr code/conditional.xvr
./build/xvr code/loop_while.xvr
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
   rm -rf build && mkdir build && cd build
   cmake .. && cmake --build .    # Build
   ctest --output-on-failure     # Run tests
   bash ../fuzzer/fuzz_test_suite.sh # Run fuzzer
   bash ../fuzzer/stress_test.sh     # Run stress tests
   ```
5. **Commit** - The pre-commit hook will auto-format your staged files
6. **Push** to your fork
7. **Submit** a pull request

> **Note**: The pre-commit hook automatically formats C files with `clang-format`. If your files are reformatted, simply commit again after the hook re-stages them.

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
