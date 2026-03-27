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

# Debug build with sanitizers
cmake -DXVR_SANITIZE=address ..

# Build without tests
cmake -DXVR_BUILD_TESTS=OFF ..

# Build without shared library
cmake -DXVR_BUILD_SHARED=OFF ..
```

### Quick Reference

| Command | Description |
|---------|-------------|
| `cmake ..` | Debug build |
| `cmake -DCMAKE_BUILD_TYPE=Release ..` | Release build |
| `cmake -DXVR_SANITIZE=address ..` | Build with AddressSanitizer |
| `cmake --build .` | Compile |
| `ctest --output-on-failure` | Run tests |
| `cmake --install . --prefix /usr/local` | Install |

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
```

## Say hello with Xvr
```xvr
std::println("Hello, World!");
```

## Print with Format Specifiers
```xvr
std::print("Hello, {}", "World");           // Hello, World
std::print("Number: {}", 42);                // Number: 42
std::print("Float: {}", 3.14);               // Float: 3.14
std::print("{} is {} years old", "arfy", 25);  // arfy is 25 years old

// Arrays
var data: [int] = [1, 2, 3];
std::print("array: {}", data);               // array: 1 2 3
```

## Print with Newline (println)
```xvr
std::println("Hello, World!");               // Hello, World! (with newline)
std::println("Name: {} {}", "arfy", "slowy"); // Name: arfy slowy
std::println("Value: {}", 42);                // Value: 42
```

The `std::println` function automatically appends a newline character to the output.

XVR uses `{}` placeholders. Supported types: integers, floats, strings, arrays.

## Need Tutorial?

You can check on [tutorial](docs/tutorial) for explore some tutorials.

## Side project XvrLang

- Neovim Syntax highlighting for XvrLang: [xvrlang-treesitter](https://github.com/WargaSlowy/xvrlang-treesitter)
- Vscode Syntax highlighting for XvrLang: [xvrlang-vscode](https://github.com/WargaSlowy/xvrlang-vscode)
