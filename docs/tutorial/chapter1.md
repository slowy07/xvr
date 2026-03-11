# Chapter 1 - Getting Started

## Installation

Build XVR from source:

```sh
git clone https://github.com/slowy07/xvr
cd xvr
make
```

The compiler is in `out/xvr`.

## Hello World

Create a file `hello.xvr`:

```xvr
print("Hello, World!");
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
