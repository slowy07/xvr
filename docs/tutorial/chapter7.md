# Chapter 7

## Lets we repeat some name

How can i make "jane doe you are funtastic" with 50 times?, should i create 50 statement of prints ?. Absolutely no, we can use looping.

## For Loop

The `for` loop repeats a block of code a specified number of times:

```xvr
for (var increment: int = 0; increment < 50; increment++) {
    std::print("jane doe you are funtastic");
}
```

`increment` variable are incremental value that always change until statement `< 50` become `false`, so, the value will start from `0` -> `1` and ends with `49` because `0` are including.

### For Loop Syntax

```xvr
for (init; condition; increment) {
    // body
}
```

| Part | Description |
|------|-------------|
| `init` | Executed once before the loop starts (e.g., `var i = 0`) |
| `condition` | Evaluated before each iteration; loop continues if true |
| `increment` | Executed after each iteration (e.g., `i++`) |

### For Loop with std::print

```xvr
include std;

for (var i = 0; i < 20; i++) {
    std::print("{}\n", i);
}
```

**Output:**
```
0
1
2
...
18
19
```

### For Loop with Increment/Decrement

```xvr
// Count up
for (var i: int32 = 0; i < 5; i++) {
    std::print("{}", i);  // 0, 1, 2, 3, 4
}

// Count down
for (var i: int32 = 5; i > 0; i--) {
    std::print("{}", i);  // 5, 4, 3, 2, 1
}
```

### For Loop Example

```xvr
for (var i: int32 = 0; i < 5; i++) {
    std::print("Iteration: {}", i);
}
// Output: 0, 1, 2, 3, 4
```

## While Loop

The `while` loop repeats as long as a condition is true:

```xvr
var increment: int = 0;

while (increment < 50) {
    std::print("jane doe you are funtastic");
    increment++;
}
```

### When to Use While vs For

| Loop Type | Use Case |
|-----------|----------|
| `for` | When you know the number of iterations |
| `while` | When you don't know how many iterations needed |

We can using `while` looping, its look more logically but its very different from `for`. `while` are used if we don't know how much will count it, `for` loop basically we know how much we need to repeat some statement.

### While Loop Syntax

```xvr
while (condition) {
    // body
}
```

## Break Statement

The `break` statement **exits the loop immediately**:

```xvr
var i: int32 = 0;
while (i < 100) {
    if (i == 10) {
        break;  // Exit the loop when i reaches 10
    }
    std::print("{}", i);
    i++;
}
// Output: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
```

### Break in For Loop

```xvr
for (var i: int32 = 0; i < 100; i++) {
    if (i == 20) {
        break;  // Stop at 20
    }
    std::print("{}", i);
}
// Output: 0 through 19
```

## Continue Statement

The `continue` statement **skips the rest of the current iteration** and moves to the next one:

```xvr
for (var i: int32 = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;  // Skip even numbers
    }
    std::print("{}", i);  // Only prints odd: 1, 3, 5, 7, 9
}
```

### Continue in While Loop

```xvr
var i: int32 = 0;
while (i < 10) {
    i++;
    if (i % 2 == 0) {
        continue;  // Skip even
    }
    std::print("{}", i);  // Only odd: 1, 3, 5, 7, 9
}
```

## Practical Examples

### Finding an Element

```xvr
var arr = [1, 2, 3, 4, 5];
var target: int32 = 3;
var found: bool = false;

for (var i: int32 = 0; i < 5; i++) {
    if (arr[i] == target) {
        found = true;
        break;  // No need to continue searching
    }
}

if (found) {
    std::print("Found!");
} else {
    std::print("Not found");
}
```

### Skip Invalid Values

```xvr
var count: int32 = 0;
var i: int32 = 0;

while (count < 10) {
    i++;
    if (i < 0) {
        continue;  // Skip negative values
    }
    count++;
    std::print("Valid: {}", i);
}
```

### Infinite Loop with Break

```xvr
var input: int32 = 0;

while (true) {
    if (input == 42) {
        std::print("You found the magic number!");
        break;  // Exit infinite loop
    }
    input = input + 1;  // In real code, this would be user input
}
```

### Nested Loops

```xvr
for (var i: int32 = 0; i < 3; i++) {
    for (var j: int32 = 0; j < 3; j++) {
        std::print("({}, {})", i, j);
    }
}
// Output: (0,0) (0,1) (0,2) (1,0) (1,1) (1,2) (2,0) (2,1) (2,2)
```

## Increment and Decrement Operators

XVR supports both prefix and postfix increment/decrement:

| Operator | Description | Example | Result |
|----------|-------------|---------|--------|
| `++` | Postfix increment | `i++` | Returns old value, then increments |
| `--` | Postfix decrement | `i--` | Returns old value, then decrements |
| `++` | Prefix increment | `++i` | Increments, then returns new value |
| `--` | Prefix decrement | `--i` | Decrements, then returns new value |

### Example

```xvr
var a: int32 = 5;
var b: int32 = a++;  // b = 5, a = 6
var c: int32 = ++a;  // c = 7, a = 7
```

## Error Handling

### Break/Continue Outside Loop

```xvr
if (true) {
    break;  // ERROR: break must be inside a loop
}
```

Compiler output:
```
error: break statement must be inside a loop
help: place the 'break' statement inside a 'while' or 'for' loop
```

### Non-Boolean Condition

```xvr
var x = 5;
while (x) { }  // ERROR: condition must be boolean
```

Compiler output:
```
error: condition of while statement must be boolean, got 'i32'
help: use a comparison operator (e.g., 'x > 0') or wrap the condition with 'bool()'
```

## LLVM IR Generation

Loops generate proper LLVM basic blocks:

### For Loop IR

```llvm
; for (var i = 0; i < 5; i++) { std::print(i); }
entry:
  %i = alloca i32
  store i32 0, ptr %i
  br label %for_cond

for_cond:                                   ; preds = %entry, %for_inc
  %i1 = load i32, ptr %i
  %lt = icmp slt i32 %i1, 5
  br i1 %lt, label %for_body, label %for_end

for_body:                                   ; preds = %for_cond
  %i2 = load i32, ptr %i
  call @printf(ptr @fmt, i32 %i2)
  br label %for_inc

for_inc:                                    ; preds = %for_body
  %i3 = load i32, ptr %i
  %inc = add i32 %i3, 1
  store i32 %inc, ptr %i
  br label %for_cond

for_end:                                    ; preds = %for_cond
  ret i32 0
```

### While Loop IR

```llvm
; while (i < 10) { i = i + 1; }
entry:
  br label %while_cond

while_cond:                                   ; preds = %entry, %while_body
  %cond = icmp slt i32 %i, 10
  br i1 %cond, label %while_body, label %while_end

while_body:                                   ; preds = %while_cond
  ; loop body code
  br label %while_cond

while_end:                                    ; preds = %while_cond
  ; code after loop
```

## Best Practices

1. **Avoid infinite loops** unless intentional
2. **Use break for early exit** - clearer than complex conditions
3. **Use continue for skipping** - cleaner than nested if statements
4. **Keep loops simple** - extract complex logic to functions
5. **Watch for off-by-one errors** - test boundary conditions
6. **Use for loops for counted iterations** - more readable

```xvr
// Good - clear loop condition
for (var i: int32 = 0; i < length; i++) {
    process(arr[i]);
}

// Avoid - confusing condition
for (var i: int32 = 1; i <= length - 1; i++) {
    process(arr[i]);
}
```

## Summary

| Statement | Description |
|-----------|-------------|
| `for` | Loop with known iteration count |
| `while` | Loop with condition |
| `break` | Exit loop immediately |
| `continue` | Skip to next iteration |
| `++` / `--` | Increment/decrement operators |
| Conditions | Must be boolean |

- `for` loops are best when you know the number of iterations
- `while` loops are best when the number of iterations is unknown
- `break` and `continue` provide fine-grained control over loop execution
- Always ensure loops have a way to terminate
- Use `std::print()` for formatted output with `\n` for newlines
