![image_banner](.github/images/banner_xvr.png)

## Feature

- AOT (Ahead-of-Time) compilation using LLVM IR
- Compiles .xvr source files to native executables

> [!NOTE]
> For more information about project you can check the [docs](docs) for documentation about xvr, and you can check the sample code from [code](code) directory

## Build Instruction

> [!NOTE]
> For Windows using (mingw32 & Cygwin), For linux or Unix already support compiler

Build the compiler:
```sh
# Build the xvr compiler (default target)
make

# The compiler output is in ./out/xvr
```

## Usage

```sh
# Compile and run a .xvr file (default)
./out/xvr source.xvr

# Compile to object file only
./out/xvr -c source.xvr -o output.o

# Dump LLVM IR
./out/xvr -S source.xvr
```

## Say hello with Xvr
```xvr
print("Hello, World!");
```

## Print with Format Specifiers
```xvr
print("Hello %s", "World");           // Hello World
print("Number: %d", 42);               // Number: 42
print("Float: %f", 3.14);             // Float: 3.14
print("%s is %d years old", "arfy", 25);  // arfy is 25 years old
```

Supported specifiers: `%s` (string/any), `%d`/`%i` (integer), `%f`/`%g` (float), `%%` (literal %), `\n` (newline)

## Need Tutorial?

You can check on [tutorial](docs/tutorial) for explore some tutorials.

## Side project XvrLang

- Neovim Syntax highlighting for XvrLang: [xvrlang-treesitter](https://github.com/WargaSlowy/xvrlang-treesitter)
- Vscode Syntax highlighting for XvrLang: [xvrlang-vscode](https://github.com/WargaSlowy/xvrlang-vscode)
