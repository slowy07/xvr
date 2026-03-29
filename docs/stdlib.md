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
- String literals can be printed directly (e.g., `std::print("hello")`)
- Variables require format specifiers (e.g., `std::print("{}", myVar)`)

Supported types in format placeholders:
- `int` - Prints as decimal integer
- `float` - Prints as floating point number
- `string` - Prints the string content
- `double` - Prints as double precision float
- `array` - Prints all elements separated by spaces

### std::println

Print output to the console with automatic newline. Similar to `std::print` but always appends a newline character:

```xvr
std::println("Hello, World!");                    // Hello, World!
std::println("Value: {}", 42);                     // Value: 42
std::println("int: {} float: {}", 42, 3.14);       // int: 42 float: 3.140000
std::println("{} + {} = {}", 1, 2, 3);             // 1 + 2 = 3
std::println("my name is {} {}", "arfy", "slowy"); // my name is arfy slowy
```

The `std::println` function uses `{}` placeholders for formatting:
- `{}` - Auto-detects type (int, float, string)
- Automatically appends `\n` to the output
- String literals can be printed directly
- Variables require format specifiers

**Variables:**
```xvr
var name: string = "arfy";
var other_name = "slowy";
std::println("my name is {} {}", name, other_name); // my name is arfy slowy
```

Supported types in format placeholders:
- `int` - Prints as decimal integer
- `float` - Prints as floating point number
- `string` - Prints the string content
- `double` - Prints as double precision float
- `array` - Prints all elements separated by spaces

**Error Handling:**

`std::println` includes comprehensive error handling:

| Error | Description |
|-------|-------------|
| First argument must be a string | First argument must be a string, not int/float |
| Format placeholder count mismatch | Number of `{}` placeholders must match arguments |
| Invalid format string | Malformed format string |
| Memory allocation failed | Out of memory |

Example error:
```xvr
std::println(42);              // error: println: first argument must be a string
std::println("{} {}", 42);      // error: println: format placeholder count (2) does not match argument count (1)
```

### std::max

Returns the greater of two or more values:

```xvr
var a = std::max(10, 20);              // a = 20
var b = std::max(5, 5);                // b = 5 (returns first if equal)
var c = std::max(-5, 10);              // c = 10
var d = std::max(1.5, 2.5);            // d = 2.5 (works with floats)
var e = std::max(3, 1, 2);             // e = 3 (multiple arguments)
var f = std::max(10, 20, 30, 5);       // f = 30 (up to 4 args O(1), more O(n))
std::print("max: {}\n", std::max(2221, 2313));  // prints: 2313
```

Supported types:
- `int` - Integer values (uses signed comparison)
- `float` - Single precision floating point
- `double` - Double precision floating point

Requirements:
- 1-8 arguments supported
- All arguments must be the same type (int or float)

Complexity:
- 1-4 arguments: O(1) - constant time with unrolled comparisons
- 5+ arguments: O(n) - linear time with loop

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
