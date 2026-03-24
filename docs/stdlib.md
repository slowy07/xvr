# Standard Library

XVR provides a standard library for common operations. The stdlib is located in `lib/std/`.

## Using the Standard Library

### include std;

Include the standard library module:

```xvr
include std;
```

This loads the stdlib module for use in your program.

### std::print

Print output to the console:

```xvr
std::print("Hello, World!");
std::print("Value: {}", 42);
std::print("{} + {} = {}", 1, 2, 3);
```

The `std::print` function uses `{}` placeholders for formatting.

### String Concatenation

XVR supports both compile-time and runtime string concatenation:

```xvr
// Compile-time (both operands are literals - constant-folded)
var msg = "Hello, " + "World!";  // "Hello, World!" at compile time

// Runtime (at least one operand is a variable/procedure param)
var name = "Rusdi";
var greeting = "Hello, " + name;  // uses runtime string_concat proc
```

## Module Structure

The standard library is organized into modules:

- `lib/std/std.mod` - Main std module
- `lib/std/io.mod` - IO module

## Future Modules

Planned modules:
- `std/io` - Input/Output operations
- `std/string` - String manipulation
- `std/memory` - Memory management
- `std/math` - Mathematical functions
- `std/fs` - File system operations
- `std/collections` - Data structures
