# Standard Library

XVR provides a standard library for common operations. The stdlib is located in `lib/std/`.

## Using the Standard Library

### include std;

Include the standard library module:

```xvr
include std;
```

This loads the stdlib module for use in your program. Built-in functions like `std::print` and `std::max` are automatically available.

## Built-in Functions

These functions are built into the compiler and available without any additional setup:

### std::print

Print output to the console with format string support:

```xvr
std::print("Hello, World!");
std::print("Value: {}\n", 42);
std::print("int: {} float: {}\n", 42, 3.14);
std::print("{} + {} = {}\n", 1, 2, 3);
```

The `std::print` function uses `{}` placeholders for formatting:
- `{}` - Auto-detects type (int, float, string)
- Use `\n` in the format string for newlines

Supported types in format placeholders:
- `int` - Prints as decimal integer
- `float` - Prints as floating point number
- `string` - Prints the string content
- `double` - Prints as double precision float

### std::max

Returns the greater of two or more values:

```xvr
var a = std::max(10, 20);              // a = 20
var b = std::max(5, 5);                // b = 5 (returns first if equal)
var c = std::max(-5, 10);              // c = 10
var d = std::max(1.5, 2.5);            // d = 2.5 (works with floats)
var e = std::max(3, 1, 2);             // e = 3 (multiple arguments)
var f = std::max(10, 20, 30, 5, 15);   // f = 30 (many arguments)
std::print("max: {}\n", std::max(2221, 2313));  // prints: 2313
```

Supported types:
- `int` - Integer values (uses signed comparison)
- `float` - Single precision floating point
- `double` - Double precision floating point

Requirements:
- At least one argument required
- All arguments must be the same type (int or float)

Complexity: O(n) time where n is the number of arguments, O(1) auxiliary space

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
