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
./build/xvr source.xvr

# Compile to executable
./build/xvr source.xvr -o output

# Compile to object file only
./build/xvr source.xvr -c output.o

# Dump LLVM IR to stdout
./build/xvr source.xvr -l

# Verbose compilation (show build stages)
./build/xvr -d source.xvr
```

### Optimization Levels

XVR supports multiple optimization levels via the `-O` flag:

| Level | Description |
|-------|-------------|
| `-O0` | No optimization (fastest compile time) |
| `-O1` | Basic optimizations |
| `-O2` | Balanced optimizations (default for Release) |
| `-O3` | Aggressive optimizations (maximum performance) |

```sh
# Compile with optimizations
./build/xvr source.xvr -O2

# Compile with aggressive optimization
./build/xvr source.xvr -O3 -o output
```

### Optimization Pipeline

The compiler applies optimizations in two stages:

1. **AST Optimizations** (frontend):
   - Constant Folding: Evaluates constant expressions at compile time
   - Dead Code Elimination: Removes unreachable code branches

2. **LLVM Optimizations** (backend):
   - Advanced interprocedural optimizations
   - Loop optimizations
   - Vectorization
   - Register allocation

Example transformations:
```xvr
# Before optimization (source)
var x = 2 + 3 * 4;    # becomes 14
if (false) {
    println("unreachable");
}

# After optimization (LLVM IR)
%1 = add i32 14
br label %exit
```

### Verbose Output

Use `-d` or `--debug` flag to see compilation information:

```sh
./build/xvr -d source.xvr
```

Output shows:
- Target architecture
- Output file size
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
