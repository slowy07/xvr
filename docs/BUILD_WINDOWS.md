# Building XVR on Windows

## Prerequisites

1. **LLVM/Clang**: Install LLVM or Clang from https://releases.llvm.org/ or via winget:
   ```
   winget install LLVM.LLVM
   ```
   
   Make sure to add LLVM to your PATH (default: `C:\Program Files\LLVM\bin`)

2. **CMake**: Install CMake from https://cmake.org/download/ or via winget:
   ```
   winget install Kitware.CMake
   ```

3. **Build Tools**: Visual Studio 2022 or Build Tools for Visual Studio 2022

## Building with CMake

### Using Visual Studio Developer Command Prompt

```batch
cd C:\path\to\xvr
mkdir build
cd build
cmake -G "NMake Makefiles" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..
cmake --build .
```

### Using Ninja (recommended)

```batch
cd C:\path\to\xvr
mkdir build
cd build
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..
cmake --build .
```

### Using Visual Studio Generator

```batch
cd C:\path\to\xvr
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

## Alternative: Manual Build with Clang

```batch
:: Set LLVM path
set LLVM_DIR=C:\Program Files\LLVM

:: Compile source files
clang -c -I src -I src/backend -DXVR_IMPORT -DXVR_EXPORT_LLVM src\xvr_*.c src\backend\*.c

:: Compile compiler
clang -c main_compiler.c compiler_tools.c

:: Link
clang -o xvr.exe main_compiler.obj compiler_tools.obj xvr_*.obj backend\*.obj -lLLVMCore -lLLVMBitWriter -lLLVMSupport
```

## Running the Compiler

```batch
.\build\xvr_compiler.exe your_file.xvr
```

## Troubleshooting

- **LLVM not found**: Set `LLVM_DIR` environment variable to your LLVM installation path
- **Link errors**: Ensure LLVM libraries are in your PATH or specify `-DLLVM_DIR`
- **Runtime errors**: Install Visual C++ Redistributable if needed