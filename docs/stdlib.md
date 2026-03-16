# Standard Library

XVR provides a standard library for common operations. The stdlib is located in `lib/std/`.

## Using the Standard Library

### std::print

Print output to the console:

```xvr
std::print("Hello, World!");
std::print("Value: {}", 42);
std::print("{} + {} = {}", 1, 2, 3);
```

The `std::print` function uses `{}` placeholders for formatting, similar to `print`.

### include std;

Include the standard library module:

```xvr
include std;
```

This loads the stdlib module for use in your program.

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
