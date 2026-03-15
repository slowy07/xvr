# Contributing to XVR

Thank you for contributing to XVR Programming Language!

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
├── src/              # Core source files
│   ├── backend/      # LLVM AOT backend
│   └── *.c/*.h       # Parser, lexer, AST, etc.
├── compiler/         # Compiler executable
├── test/             # Unit tests
├── docs/             # Documentation
└── out/              # Build output
```

## Building

```sh
# Build everything
make

# Build just the compiler
make -C compiler

# Run tests
make -C test

# Clean build artifacts
make clean
```

## Testing

All tests are in the `test/` directory:

```sh
cd test
make
../out/test_all
```

### Test Structure

- `test_all.c` - Main test runner
- `test_*.c` - Individual test modules

### Adding Tests

1. Add your test function to the appropriate `test_*.c` file
2. Register it in `test_all.c`
3. Rebuild and run tests

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
4. **Test** your changes with `make -C test`
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

## Reporting Bugs

Include:
- Description of the bug
- Steps to reproduce
- Expected vs actual behavior
- Environment (OS, LLVM version)

## Code Review Criteria

- Code follows style guidelines
- Tests pass (`make -C test`)
- No memory leaks
- No compiler warnings
- Documentation updated if needed

## Topics for Contribution

Look for `// BUG`, `// TODO`, or `// FIXME` comments in the code for starting points.

---

> [!NOTE]
> This project uses LLVM for AOT compilation. The interpreter runtime has been removed - all code compiles to native executables via LLVM.
