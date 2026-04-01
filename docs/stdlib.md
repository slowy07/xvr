# Standard Library

XVR provides a standard library for common operations. The stdlib is located in `lib/std/`.

## Using the Standard Library

### include std;

Include the standard library module:

```xvr
include std;
```

This loads the stdlib module for use in your program. Built-in functions like `std::print` and `std::max` are automatically available.

## Module System

XVR has a module system that allows you to include external code:

### Module Resolution

When you use `include std;`, the compiler resolves the module path:

```
┌──────────────────────────────────────────────────────────────────────┐
│                      Module Resolution Flow                           │
└──────────────────────────────────────────────────────────────────────┘

  1. Parser encounters:  include std;
                         │
                         ▼
  2. ModuleResolver.resolve("std")
                         │
                         ▼
  3. Construct path:  {stdlib_path}/std.xvr
                     Default: ./lib/std/std.xvr
                         │
                         ▼
  4. LoadModule() reads and parses the .xvr file
                         │
                         ▼
  5. AST nodes merged into main compilation
```

### Module Loading

The module resolver:
1. Resolves the module name to a file path
2. Parses the module's `.xvr` source file
3. Merges the module's AST nodes into the main compilation

Example:
```xvr
include std;  // Loads ./lib/std/io.xvr
std::println("Hello!");
```

## File Structure

```
lib/std/
├── std.xvr          # Main std module (redirects to io)
├── std.mod          # Module metadata
├── io.xvr           # IO functions (print, println, etc.)
└── io.mod           # Module metadata
```

## Built-in Functions

These functions are built into the compiler and available without any additional setup:

### Builtin Functions (Compiler-level)

These are implemented in the compiler itself:

#### sizeof(T)

Returns the size in bits of a type:

```xvr
var int_size = sizeof(int);      // 32
var float_size = sizeof(float);  // 32
var double_size = sizeof(double); // 64
var bool_size = sizeof(bool);    // 8
```

Supported types:
- `void` - 0 bits
- `bool`, `int8`, `uint8` - 8 bits
- `int16`, `uint16` - 16 bits
- `int`, `int32`, `uint32`, `float32` - 32 bits
- `int64`, `uint64`, `float64` - 64 bits

#### len(x)

Returns the length of a collection (array, string):

```xvr
var arr = [1, 2, 3, 4, 5];
var length = len(arr);  // 5
```

#### panic(msg)

Causes the program to terminate with an error message:

```xvr
panic("XVR there's something wrong right here");
```

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

## Math Module

The `math` module provides comprehensive mathematical functions using LLVM intrinsics and libm fallback.

### Using the Math Module

```xvr
include math;
include std;

var result = math::sqrt(25.0);
std::println("sqrt(25) = {}", result);  // sqrt(25) = 5.000000
```

### Basic Functions

| Function | Description | Example |
|----------|-------------|---------|
| `math::sqrt(x)` | Square root | `math::sqrt(16.0)` → 4.0 |
| `math::pow(x, y)` | Power (x^y) | `math::pow(2.0, 3.0)` → 8.0 |
| `math::abs(x)` | Absolute value | `math::abs(-42)` → 42 |

### Trigonometric Functions

| Function | Description | Example |
|----------|-------------|---------|
| `math::sin(x)` | Sine | `math::sin(0.0)` → 0.0 |
| `math::cos(x)` | Cosine | `math::cos(0.0)` → 1.0 |
| `math::tan(x)` | Tangent | `math::tan(0.0)` → 0.0 |

### Inverse Trigonometric Functions

| Function | Description | Example |
|----------|-------------|---------|
| `math::asin(x)` | Arc sine | `math::asin(0.5)` → 0.523599 |
| `math::acos(x)` | Arc cosine | `math::acos(0.5)` → 1.047198 |
| `math::atan(x)` | Arc tangent | `math::atan(1.0)` → 0.785398 |
| `math::atan2(y, x)` | 2-arg arc tangent | `math::atan2(1.0, 1.0)` → 0.785398 |

### Logarithmic Functions

| Function | Description | Example |
|----------|-------------|---------|
| `math::log(x)` | Natural log | `math::log(1.0)` → 0.0 |
| `math::log10(x)` | Base-10 log | `math::log10(100.0)` → 2.0 |
| `math::log2(x)` | Base-2 log | `math::log2(8.0)` → 3.0 |

### Exponential Functions

| Function | Description | Example |
|----------|-------------|---------|
| `math::exp(x)` | e^x | `math::exp(1.0)` → 2.718282 |

### Rounding Functions

| Function | Description | Example |
|----------|-------------|---------|
| `math::floor(x)` | Round down | `math::floor(3.7)` → 3.0 |
| `math::ceil(x)` | Round up | `math::ceil(3.2)` → 4.0 |
| `math::round(x)` | Round to nearest | `math::round(3.5)` → 4.0 |
| `math::trunc(x)` | Truncate | `math::trunc(3.7)` → 3.0 |

### Hyperbolic Functions

| Function | Description | Example |
|----------|-------------|---------|
| `math::sinh(x)` | Hyperbolic sine | `math::sinh(1.0)` → 1.175201 |
| `math::cosh(x)` | Hyperbolic cosine | `math::cosh(0.0)` → 1.0 |
| `math::tanh(x)` | Hyperbolic tangent | `math::tanh(0.0)` → 0.0 |

### Inverse Hyperbolic Functions

| Function | Description | Example |
|----------|-------------|---------|
| `math::asinh(x)` | Inverse hyperbolic sine | `math::asinh(0.0)` → 0.0 |
| `math::acosh(x)` | Inverse hyperbolic cosine | `math::acosh(1.0)` → 0.0 |
| `math::atanh(x)` | Inverse hyperbolic tangent | `math::atanh(0.5)` → 0.549306 |

### Modulo Functions

| Function | Description | Example |
|----------|-------------|---------|
| `math::fmod(x, y)` | Floating-point remainder | `math::fmod(10.0, 3.0)` → 1.0 |

### Mathematical Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `math::PI` | 3.141592653589793 | Pi (π) |
| `math::E` | 2.718281828459045 | Euler's number (e) |

### Constant Folding

The math module supports compile-time constant folding for numeric literals:

```xvr
var x = math::sqrt(25.0);  // Computed at compile time: x = 5.0
var y = math::sin(0.0);   // Computed at compile time: y = 0.0
var z = math::log2(1024.0);  // Computed at compile time: z = 10.0
```

### Type Support

All math functions support `float32` and `float64` types. The `abs` function also supports `int32` and `int64`:

```xvr
var i = math::abs(-42);      // int32: 42
var f = math::abs(-3.14);    // float32: 3.14
```

### IEEE 754 Compliance

Math functions follow IEEE 754 behavior for edge cases:

```xvr
math::sqrt(-1.0)  // Returns NaN at runtime
math::log(0.0)     // Returns -Infinity
math::log(-1.0)    // Returns NaN
math::pow(0.0, -1.0)  // Returns Infinity
```

### Implementation Details

- Uses LLVM intrinsics (`llvm.sqrt.*`, `llvm.sin.*`, etc.) for efficient code generation
- Falls back to libm for functions without LLVM intrinsics (e.g., `sinh`, `cosh`, `tanh`)
- Linked with `-lm` for mathematical library functions

## Future Modules

Planned modules:
- `std/io` - Input/Output operations
- `std/string` - String manipulation
- `std/memory` - Memory management
- `std/fs` - File system operations
- `std/collections` - Data structures
