# Chapter 8

## Can we make some custom statement ?

Yes, of course, `proc` can do it.

```
proc call() {
    print("hello");
    print("we can make some procedure");
}

// call it
call();
```

We can use parameter, when we call it, we can use using some values.

```
proc call_name(name) {
    print("wello " + name);
}

// or we can make specification with data types
proc add_multiple(first: int, second: int) {
    print(first + second * first);
}

call_name("jane");
add_multiple(5, 3);
```

You can assign that function as value on variable.

```
proc answer(value: int) {
    return value + 30;
}

var number: int = answer(20);
print(number);
```

## Lets we play it

How about fibonacci ?

```
proc fibonacci(number: int) {
  if (number < 2) {
    return number;
  }

  return fibonacci(number - 1) + fibonacci(number - 2);
}

for (var i = 0; i < 20; i++) {
  var answer = number(i);
  print(answer);
}
```

> [!NOTE]
> print will support with or without parentheses (round bracket).
