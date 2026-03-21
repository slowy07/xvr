# Chapter 1 - Getting Started

## Installation

### Linux/macOS

Build XVR from source:

```sh
git clone https://github.com/slowy07/xvr
cd xvr
make
```

The compiler is in `out/xvr`.

### Windows

#### Prerequisites

1. **Install LLVM**
   - Download LLVM from https://github.com/llvm/llvm-project/releases
   - Choose the latest stable version (e.g., LLVM-22.x.x)
   - During installation, select "Add LLVM to the system PATH"

2. **Install MSYS2** (recommended)
   - Download from https://www.msys2.org/
   - Install MSYS2 and open "MSYS2 MINGW64" terminal


#### Build XVR

```bash
# Clone the repository
git clone https://github.com/slowy07/xvr
cd xvr

# Build using MinGW
mingw32-make
```

Or using PowerShell:

```powershell
# Using PowerShell
# Install LLVM first via winget or download
winget install LLVM.LLVM

# Clone and build
git clone https://github.com/slowy07/xvr
cd xvr
mingw32-make
```

#### Troubleshooting Windows Build

| Error | Solution |
|-------|----------|
| `clang: command not found` | Add LLVM `bin` folder to PATH |
| `undefined reference to LLVM*` | Ensure LLVM libraries are in PATH |
| `make: *** No rule to make target` | Use `mingw32-make` on Windows |

#### Verify Installation

```cmd
out\xvr.exe --version
```

## Hello World

Create a file `hello.xvr`:

```xvr
std::print("Hello, World!");
```

## Running

```sh
# Compile and run
./out/xvr hello.xvr
```

Output:
```
Hello, World!
```

## Options

```sh
./out/xvr -h          # Show help
./out/xvr -v          # Show version
./out/xvr hello.xvr -l    # Dump LLVM IR
./out/xvr hello.xvr -o prog  # Compile to executable
./out/xvr hello.xvr -c -o hello.o  # Compile to object file
```
