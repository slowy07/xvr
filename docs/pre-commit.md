# XVR Pre-commit Hook Documentation

A comprehensive pre-commit hook for C/C++ code quality enforcement.

## Features

### Automatic Formatting
- **clang-format**: Automatically formats `.c` and `.h` files to maintain consistent code style
- **Idempotent**: Re-running produces the same result
- **Environment-agnostic**: Uses LLVM's standard formatting rules

### Validation Checks
- **Trailing whitespace**: Detects and removes trailing spaces/tabs
- **Newline at EOF**: Ensures files end with a proper newline
- **Binary file detection**: Warns when binary files are staged

### Performance
- **Staged files only**: Only processes files that are staged for commit
- **Fast execution**: No full-repo scans
- **Parallel processing**: Efficient file handling

## Installation

### Automatic (if pre-commit tool is available)
```sh
# If using pre-commit framework:
pre-commit install
```

### Manual
```sh
# The hook is already installed at .git/hooks/pre-commit
# Just ensure it's executable:
chmod +x .git/hooks/pre-commit
```

## Usage

The hook runs automatically when you commit:

```sh
# Stage your files
git add .

# Commit (hook runs automatically)
git commit -m "Your commit message"

# If issues are found:
# 1. Hook auto-fixes formatting issues
# 2. Re-stages fixed files
# 3. Shows summary
# 4. Commit is blocked until you commit again
```

## Disabling Temporarily

### Skip all hooks
```sh
git commit --no-verify -m "Emergency commit"
```

### Skip specific hook
```sh
# Add to your commit message: [skip hook]
git commit -m "[skip hook] Emergency commit"
```

### Disable permanently
```sh
rm .git/hooks/pre-commit
```

## Configuration

### clang-format
The project uses `.clang-format` for code style. Key settings:

```yaml
Language: C
BasedOnStyle: Google
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 80
```

### Ignoring Directories
Edit the `IGNORE_DIRS` variable in the hook:
```bash
IGNORE_DIRS="^(build|out|\\.git|fuzzer/harness)/"
```

## Exit Codes

| Code | Description |
|------|-------------|
| 0 | All checks passed |
| 1 | Issues found and auto-fixed |

## Troubleshooting

### clang-format not found
```sh
# Ubuntu/Debian
sudo apt install clang-format

# macOS
brew install clang-format

# Fedora
sudo dnf install clang-format
```

### Hook not running
```sh
# Check if hook is executable
ls -la .git/hooks/pre-commit

# Re-enable if needed
chmod +x .git/hooks/pre-commit
```

### Format conflicts with IDE
If your IDE reformats code differently, ensure it uses the project's `.clang-format`:
- **VS Code**: Install "Clang-Format" extension
- **CLion**: Use FileWatchers plugin
- **VS**: Use LLVM/clang-format extension

## Extending the Hook

### Adding new checks
Add new check functions and call them in the main loop:

```bash
check_custom() {
    local file="$1"
    # Your check logic here
    return 0  # 0 = pass, 1 = fail
}
```

### Adding new file types
Modify `CHECK_EXTENSIONS`:
```bash
CHECK_EXTENSIONS="\\.(c|h|cc|cpp|hpp)$"
```

## Design Decisions

### Why auto-fix instead of reject?
- **Better UX**: Developers don't need to manually fix issues
- **Faster workflow**: No context switching to run formatter
- **Consistency**: Everyone gets the same formatting

### Why staged files only?
- **Performance**: Full-repo scans are slow
- **Precision**: Only checks what you're about to commit
- **Safety**: Doesn't modify untested code

### Why use clang-format?
- **Industry standard**: Used by LLVM, Chromium, WebKit, etc.
- **LLVM-compatible**: Natural fit for LLVM-based projects
- **Configurable**: Easy to customize via `.clang-format`
