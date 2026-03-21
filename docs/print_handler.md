# Print Handler

In the XVR AOT compiler, printing is provided via `std::print()` from the standard library.

## Print Function

**Note:** The standalone `print()` statement is no longer supported. Use `std::print()` instead.

```xvr
std::print("Hello, world!");
std::print("Value: {}", 42);
std::print("Name: {}, Age: {}", name, age);
```

Using a standalone `print()` will result in a compilation error:

```xvr
print("Hello");  // Error: print() is not supported, use std::print() instead
```

## String Interpolation

XVR uses `{}` placeholders for formatted output:

```xvr
var x = 10;
var y = 20;
std::print("{} + {} = {}", x, y, x + y);  // 10 + 20 = 30
```

### Type Inference

The format string parser automatically detects argument types:

| Type | Example | printf format |
|------|---------|---------------|
| string | `"hello"` | `%s` |
| integer | `42` | `%d` |
| float | `3.14` | `%lf` |
| boolean | `true` | `%s` |

### Security

Only **literal strings** are parsed as format strings:

```xvr
var userInput = getInput();
std::print(userInput);  // Passed directly to printf (safe)
std::print("{}", userInput);  // Also safe - uses %s for strings
```

## Implementation

The compiler generates code like:

```c
// Runtime (linked automatically)
#include <stdio.h>
#include <stdarg.h>
int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vprintf(fmt, args);
    va_end(args);
    return result;
}
```

## LLVM IR Generation

```xvr
var name = "world";
std::print("Hello, {}!", name);
```

Generates:

```llvm
@fmt_str = private unnamed_addr constant [11 x i8] c"Hello, %s!\00", align 1

define i32 @main() {
entry:
  %name = alloca ptr, align 8
  store ptr @str_literal, ptr %name, align 8
  %name1 = load ptr, ptr %name, align 8
  %printf_call = call i32 (ptr, ...) @printf(ptr @fmt_str, ptr %name1)
  ret i32 0
}
```

## Command Line

The `-n` flag disables trailing newlines (if supported by runtime).
