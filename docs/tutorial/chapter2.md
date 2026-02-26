# Chapter 2

## Playing With Literals

And now we playing with literals, literals are constant value which is defining that value and write to stdout.

```literals.xvr

// literal integers
print(3);

// literal floating numbers
// using dot character not comma
print(25.3);

// literal string
print("wello Xvr, how are you today?");

// literal boolean
// true or false
print(true)
print(false)

// or you can using separator for readable
print(200_000_000);
print(3250_000);
```


## Print with Format Specifiers

The `print` function supports format specifiers for formatted output:

```format.xvr

// %s - string, boolean, or any value
print("Hello %s", "World");           // Hello World
print("Bool: %s", true);               // Bool: true
print("Num: %s", 42);                 // Num: 42

// %d or %i - integers
print("Number: %d", 42);              // Number: 42
print("Calc: %d", 10 + 5);            // Calc: 15

// %f or %g - floats
print("Float: %f", 3.14);             // Float: 3.14
print("Pi: %g", 3.14159);             // Pi: 3.14159

// %% - literal percent sign
print("100%% complete");              // 100% complete

// Escape sequences
print("Line1\nLine2");                // Line1
                                        // Line2

// Multiple specifiers
print("%s is %d years old", "arfy", 25);  // arfy is 25 years old
```

### Format Specifiers

| Specifier | Description | Example |
|-----------|-------------|---------|
| `%s` | String, boolean, integer, float, null, array | `"Hello %s", "World"` |
| `%d` or `%i` | Integer | `"Count: %d", 42` |
| `%f` | Float (default 6 decimal places) | `"Value: %f", 3.14` |
| `%g` | Compact float (auto-switches to scientific) | `"Value: %g", 3.14159` |
| `%e` | Scientific notation (lowercase) | `"%e", 1234.56` → `1.234560e+03` |
| `%E` | Scientific notation (uppercase) | `"%E", 1234.56` → `1.234560E+03` |
| `%%` | Literal percent | `"100%%"` |

### Precision Specifiers

You can specify the number of decimal places using `.N` before the specifier:

```precision.xvr
// %.Nf - precision for floats
print("%.2f", 3.14159);    // 3.14
print("%.4f", 3.14159);    // 3.1416
print("%.0f", 2.5);        // 2

// %.Ne - precision for scientific notation
print("%.2e", 1234.56);    // 1.23e+03
print("%.4e", 0.00001234); // 1.2340e-05

// %.Ng - precision for compact format
print("%.6g", 1234.5678);  // 1234.57
```

### Float Types

XVR supports different floating-point precision types:

```float_types.xvr
// Default float (implementation-defined precision)
print("%f", 3.14159);           // 3.141590

// Float32 - 32-bit floating point
print("%.4f", 1.234567f32);     // 1.2346

// Float64 - 64-bit floating point  
print("%.8f", 1.123456789f64);  // 1.12345679

// Using %s with float types
print("%s", 3.14f32);           // 3.14
print("%s", 2.718281828f64);    // 2.71828
```

> [!TIP]
> You can still use the old style string concatenation with `+`:
> ```xvr
> print("Hello " + "World");
> ```
