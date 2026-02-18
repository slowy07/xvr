# Chapter 7

## Lets we repeat some name

How can i make "jane doe you are funtastic" with 50 times?, should i create 50 statement of prints ?. Absolutely no, we can use looping.

```
for (var increment: int = 0; increment < 50; increment++) {
    print("jane doe you are funtastic");
}
```

`increment` variable are incremental value that always change until statement `< 50` become `false`, so, the value will start from `0` -> `1` and ends with `49` because `0` are including.

```
var increment: int = 0;

while (increment < 50) {
    print("jane doe you are funtastic");
    increment++;
}
```

We can using `while` looping, its look more logically but its very different from `for`. `while` are used if we don't know how much will count it, `for` loop basically we know how much we need to repeat some statement.

But, be carefully when you using `while` loops.

```
while (true) {
    print("looping infinity");
}
```

If while statement always `true`, the statement inside the loops always will be execute. So, carefully for use it.
