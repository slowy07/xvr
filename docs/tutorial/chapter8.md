# Chapter 8

## Can we make some custom statement ?

Yes, of course, `proc` can do it.

```
proc call() {
    std::print("hello");
    std::print("we can make some procedure");
}

// call it
call();
```

We can use parameter, when we call it, we can use using some values.

```
proc call_name(name) {
    std::print("wello " + name);
}

// or we can make specification with data types
proc add_multiple(first: int, second: int) {
    std::print(first + second * first);
}

call_name("jane");
add_multiple(5, 3);
```

## Void Functions

Functions that don't return a value should declare `: void`:

```
proc greet(): void {
    std::print("Hello, World!");
}

proc print_header(title: string): void {
    std::print("====================================");
    std::print("  " + title);
    std::print("====================================");
}

greet();
print_header("Welcome");
```

### Why use `: void`?

- **Explicit intent**: Makes it clear the function doesn't return a value
- **Type safety**: Prevents accidentally returning a value from a procedure
- **Best practice**: Recommended for all functions that don't return values

## Return Values

You can return values from functions by declaring the return type:

```
proc answer(value: int): int {
    return value + 30;
}

var number: int = answer(20);
std::print(number);
```

Or use an explicit return type for clarity:

```
proc answer(value: int): int {
    return value + 12;
}

var number: int = answer(3) + 20;
std::print(number);
```

### Implicit Return

The last expression in a function is automatically returned:

```
proc add(a: int, b: int): int {
    a + b  // implicitly returned
}

var sum = add(10, 20);
std::print(sum);  // prints 30
```

## Lets we play it

How about fibonacci ?

```
proc fibonacci(number: int): int {
  if (number < 2) {
    return number;
  }

  return fibonacci(number - 1) + fibonacci(number - 2);
}

for (var i = 0; i < 20; i++) {
  var answer = fibonacci(i);
  std::print(answer);
}
```

## Function Best Practices

1. **Use void for procedures** - Functions that only perform side effects (like printing) should return `: void`
2. **Name descriptively** - `print_header()` not `ph()`
3. **Keep functions focused** - Each function should do one thing well
4. **Use explicit return types** - Makes code easier to understand
