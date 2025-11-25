# Chapter 6

## Are you good at math?

Can we evaulate some question about 5 (C) + (C)(D)? when C = 1 / 5 and D = 15 , and yes of course.

```
// compute 5(C)
// 5(C) = 5 x 1 / 5;

// compute (C) (D)
// (C)(D) = 1 / 5 x 15 = 15 / 5

// and then add it
var c: int = 1 / 5;
var d: int = 15;
var answer: int = ((5 * 1) / 5) + ((15 * 1) / 5);

print(answer);
```

you can re assign some variable.

```
var data: int = 30 * 40;

print(data);

data = 30 + 40;

print(data);
```

**fun fact**: you can using `+` for combine some string

```
print("jane " + "slowy");
```

## Playing with variable

Can we print other data types combine with string ?, absolutely no, you must convert data types to string like `integer`, `float`

```
var age: int = 20;

print("mamads are: " + string age + " years old");
```

in `xvr` you can use `string` keyword to convert `integer` or `float` data type to string for print it with strings

## Assign with other data types

you can make some variable names as alias for data types, using `astype`, with specific are `type`

```
// dataList cannot assign a value
// only working with assign as data type
var dataList: type = astype [int];
var my_list_number: dataList = [1, 2, 3, 4];

print(my_list_number);
```

```
// integersNumber cannot assign a value
// only working with assign as data type
var integersNumber: type = astype int;
var my_number: integersNumber = 20;

print("my number are: " + string my_number + " look cool right");
```

> [!NOTE]
> print will support with or without parentheses (round bracket).
