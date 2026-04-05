## Feature

- AOT (Ahead-of-Time) compilation using LLVM IR
- Compiles .xvr source files to native executables

> [!NOTE]
> For more information about project you can check the [docs](docs) for documentation about xvr, and you can check the sample code from [code](code) directory

## Build Instruction

> [!NOTE]
> For Windows using (mingw32 & Cygwin), For linux or Unix already support compiler

### Prerequisites
- CMake 3.16+
- LLVM 22+ (with llvm-config)
- C compiler (gcc/clang)

### Build with CMake
```sh
# Create build directory
mkdir build && cd build

# Configure (Debug build by default)
cmake ..

# Build the project
cmake --build .

# The compiler output is in ./build/xvr
```

### Build Options

```sh
# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build with AddressSanitizer
cmake -DXVR_SANITIZER=address ..

# Build with UndefinedBehaviorSanitizer
cmake -DXVR_SANITIZER=undefined ..

# Build without tests
cmake -DXVR_BUILD_TESTS=OFF ..

# Build without shared library
cmake -DXVR_BUILD_SHARED=OFF ..
```

### Quick Reference

| Command | Description |
|---------|-------------|
| `cmake -B build && cmake --build build` | Debug build |
| `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build` | Release build |
| `cmake -B build -DXVR_SANITIZER=address && cmake --build build` | Build with AddressSanitizer |
| `ctest --test-dir build` | Run tests |
| `cmake --install build --prefix /usr/local` | Install |

## Usage

```sh
# Compile and run a .xvr file (default)
xvr source.xvr

# Flags can be placed before or after the source file
xvr -O2 source.xvr
xvr source.xvr -O2
xvr -d -O2 source.xvr
xvr -v --timing -O3 source.xvr

# Compile to executable (default output: source name without extension)
xvr source.xvr              # → creates 'source' executable

# Compile to object file
xvr -c source.xvr          # → creates 'source.o'

# Output assembly (.s)
xvr --emit asm source.xvr  # → creates 'source.s'

# Output LLVM IR (.ll)
xvr --emit llvm-ir source.xvr # → creates 'source.ll'

# Custom output name
xvr source.xvr -o myprogram
xvr -c source.xvr -o myobject.o
```

### Command-Line Flags

| Flag | Description |
|------|-------------|
| `-h, --help` | Display help message |
| `-v, --version` | Display version info |
| `-v, --verbose` | Show detailed compilation info |
| `-d, --debug` | Enable verbose debug output |
| `-o, --output <file>` | Output file name |
| `-c, --compile [file]` | Compile to object file (.o) |
| `-S, -l, --llvm` | Output LLVM IR to stdout |
| `-e, --emit <type>` | Emit specific output (llvm-ir, asm, obj) |
| `-i, --input <code>` | Compile and run inline code |
| `-n` | Disable trailing newline in print |
| `-O<0|1|2|3>` | Optimization level |
| `-Z, --dump-tokens` | Dump lexer tokens |
| `--dump-ast` | Dump parsed AST |
| `--timing` | Show compilation timing |

### Output Types

The `-e, --emit` flag specifies the output format:

| Type | Extension | Description |
|------|-----------|-------------|
| `asm` | `.s` | Assembly language (native x86-64) |
| `llvm-ir` | `.ll` | LLVM IR (human-readable) |
| `obj` | `.o` | Object file (binary) |

Without `-e`, default behavior:
- No flag → executable (compiled binary)
- `-c` → object file (`.o`)
- Default filename = source name without `.xvr` extension

### Optimization Levels

XVR supports multiple optimization levels via the `-O` flag:

| Level | Description |
|-------|-------------|
| `-O0` | No optimization (fastest compile time) |
| `-O1` | Basic optimizations |
| `-O2` | Balanced optimizations (default) |
| `-O3` | Aggressive optimizations (maximum performance) |

```sh
# Compile with optimizations
xvr -O2 source.xvr

# Multiple flags
xvr -O3 -v source.xvr -o output
xvr -d --timing -O2 source.xvr
```

### Verbose Output

Use `-d` or `--debug` or `--verbose` flag to see compilation information:

```sh
xvr -v source.xvr
xvr --timing -O2 source.xvr
```

Output shows:
- Target architecture
- Output file size
- Compilation timing breakdown
- Total compilation time

Example:
```
  Target: x86_64-unknown-linux-gnu
  Size: 15.8 KB
  Time: 36.22 ms
```

## Say hello with Xvr
```xvr
std::println("Hello, World!");
```

## Print with Format Specifiers
```xvr
std::print("Hello, {}", "World");           // Hello, World
std::print("Number: {}", 42);                // Number: 42
std::print("Float: {}", 3.14);              // Float: 3.14
std::print("{} is {} years old", "arfy", 25);  // arfy is 25 years old

// Arrays (print elements individually)
var data: [int] = [1, 2, 3];
std::print(data);                            // 1 2 3
```

## Print with Newline (println)
```xvr
std::println("Hello, World!");              // Hello, World! (with newline)
std::println("Name: {} {}", "arfy", "slowy"); // Name: arfy slowy
std::println("Value: {}", 42);              // Value: 42
```

The `std::println` function automatically appends a newline character to the output.

XVR uses `{}` placeholders for non-string arguments. Supported types: integers, floats, strings, arrays.

## Builtin Functions

XVR includes compiler-level builtin functions:

### sizeof(T)

Returns the size in bits of a type:

```xvr
var int_size = sizeof(int);      // 32
var float_size = sizeof(float);  // 32
var double_size = sizeof(double); // 64
```

### Module System

```xvr
include std;  // Load standard library
std::println("Hello!");
```

## Need Tutorial?

You can check on [tutorial](docs/tutorial) for explore some tutorials.

## Optimization Architecture

XVR implements a modular, extensible optimization pipeline following modern compiler design principles (Rust, Clang, Swift).

### Design Principles

1. **Correctness First**: Optimizations never change program semantics
2. **Pass-Based Architecture**: Each optimization is an isolated, composable pass
3. **Deterministic Behavior**: Same input always produces same optimized output
4. **Safety**: No undefined behavior introduced by optimizations

### AST Optimization Passes

| Pass | Description | Example |
|------|-------------|---------|
| Constant Folding | Evaluates constant expressions | `2 + 3 * 4` → `14` |
| Dead Code Elimination | Removes unreachable branches | `if (false)` → removed |

### Compilation Pipeline

```
Source Code
    ↓
Lexer → Parser → AST
    ↓
[AST Optimization Passes]
    ↓
Lowering → LLVM IR
    ↓
[LLVM Optimization Pipeline]
    ↓
Machine Code
```

## Side project XvrLang

- Neovim Syntax highlighting for XvrLang: [xvrlang-treesitter](https://github.com/WargaSlowy/xvrlang-treesitter)
- Vscode Syntax highlighting for XvrLang: [xvrlang-vscode](https://github.com/WargaSlowy/xvrlang-vscode)
