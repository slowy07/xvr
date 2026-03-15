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
print(3);
print(42);
print(200_000_000);  // Underscores for readability
```

### Floats

```xvr
print(3.14);
print(25.3);
```

### Strings

```xvr
print("Hello, XVR!");
```

### Booleans

```xvr
print(true);
print(false);
```

## Print with String Interpolation

XVR uses `{}` placeholders:

```xvr
var name = "World";
var age = 25;
var pi = 3.14159;

print("Hello, {}!", name);              // Hello, World!
print("Age: {}", age);                  // Age: 25
print("Pi: {:.2f}", pi);                // Pi: 3.14
print("{} is {} years old", name, age); // World is 25 years old
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
print("{{hello}}");  // {hello}
```

## Multiple Values

```xvr
var a = 10;
var b = 20;
print("{} + {} = {}", a, b, a + b);  // 10 + 20 = 30
```
