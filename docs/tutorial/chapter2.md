# Chapter 2 - Variables and Literals

## Variables

Use `var` to declare variables:

```xvr
var x = 42;
var name = "hello";
var pi = 3.14;
var isActive = true;
```

## Literals

### Integers

```xvr
std::print(3);
std::print(42);
std::print(200_000_000);  // Underscores for readability
```

### Floats

```xvr
std::print(3.14);
std::print(25.3);
```

### Strings

```xvr
std::print("Hello, XVR!");
```

### Booleans

```xvr
std::print(true);
std::print(false);
```

## Print with String Interpolation

XVR uses `{}` placeholders:

```xvr
var name = "World";
var age = 25;
var pi = 3.14159;

std::print("Hello, {}!", name);              // Hello, World!
std::print("Age: {}", age);                  // Age: 25
std::print("Pi: {:.2f}", pi);                // Pi: 3.14
std::print("{} is {} years old", name, age); // World is 25 years old
```

### Type Inference

The format string automatically detects types:

| Value | Type | Format |
|-------|------|--------|
| `"hello"` | string | `%s` |
| `42` | integer | `%d` |
| `3.14` | float | `%lf` |
| `true` | boolean | `%s` |

### Escaping Braces

Use `{{` and `}}` for literal braces:

```xvr
std::print("{{hello}}");  // {hello}
```

## Multiple Values

```xvr
var a = 10;
var b = 20;
std::print("{} + {} = {}", a, b, a + b);  // 10 + 20 = 30
```
