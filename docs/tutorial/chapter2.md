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

> [!NOTE]
>  to write output xvr using `print` builtin function to stdout, `print` support with or without parentheses (round bracket)

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
| `%f` or `%g` | Float | `"Value: %f", 3.14` |
| `%%` | Literal percent | `"100%%"` |
| `\n` | Newline | `"Line1\nLine2"` |

> [!TIP]
> You can still use the old style string concatenation with `+`:
> ```xvr
> print("Hello " + "World");
> ```
