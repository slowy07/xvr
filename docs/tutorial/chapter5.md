# Chapter 5

## Playing with conditional

What if today was raining? maybe you using jacket or umbrella, but, what if today are sunny?, you maybe wearing short pants or using sunglasses. We can play it with conditional statement.

## If Statement

The `if` statement is a conditional statement that executes code based on a boolean condition.

### Syntax

```xvr
if (condition) {
    // then branch - executed when condition is true
}
```

### Basic If Statement

```xvr
var today_raining: bool = false;

if (today_raining) {
    std::print("wear jacket or use umbrella");
}
```

### If-Else Statement

```xvr
var today_raining: bool = false;

if (today_raining) {
    std::print("wear jacket or use umbrella");
} else {
    std::print("use sunglasses");
}
```

### If-Else-If Chain

For multiple conditions, use else-if:

```xvr
var score: int32 = 85;

if (score >= 90) {
    std::print("Grade: A");
} else if (score >= 80) {
    std::print("Grade: B");
} else if (score >= 70) {
    std::print("Grade: C");
} else {
    std::print("Grade: F");
}
```

### Nested If Statements

You can nest if statements inside each other:

```xvr
var age: int32 = 25;
var has_license: bool = true;

if (age >= 18) {
    if (has_license) {
        std::print("Can drive");
    } else {
        std::print("Need a license");
    }
} else {
    std::print("Too young to drive");
}
```


In XVR, `if` can be used as an **expression** that returns a value. This is similar to ternary-like if expressions.

### Basic Expression If

```xvr
var x: int32 = 10;
var result: int32 = if (x > 5) { 100 } else { 0 };
std::print("{}", result);  // Output: 100
```

### With Else-If Chains

```xvr
var score: int32 = 85;
var grade: string = if (score >= 90) {
    "A"
} else if (score >= 80) {
    "B"
} else if (score >= 70) {
    "C"
} else {
    "F"
};
std::print(grade);  // Output: B
```

### Important Notes

1. **Type annotation required**: When using if as an expression, the variable must have an explicit type annotation
2. **All branches must have values**: In expression form, every branch must return a value of compatible type

```xvr
// This works
var x: int32 = if (condition) { 1 } else { 2 };

// This will cause an error - type annotation required
var x = if (condition) { 1 } else { 2 };  // ERROR
```

## Conditions

Conditions in if statements **must be boolean** (`bool`). The compiler will report an error if you use non-boolean types.

### Valid Conditions

```xvr
var x: int32 = 10;

// Comparison operators return boolean
if (x > 5) {
    std::print("x is greater than 5");
}

// Boolean variables work directly
var is_valid: bool = true;
if (is_valid) {
    std::print("Valid!");
}

// Logical operators
if (x > 5 and x < 20) {
    std::print("x is between 5 and 20");
}
```

### Comparison Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `>` | Greater than | `x > 10` |
| `<` | Less than | `x < 10` |
| `>=` | Greater than or equal | `x >= 10` |
| `<=` | Less than or equal | `x <= 10` |
| `==` | Equal | `x == 10` |
| `!=` | Not equal | `x != 10` |

### Logical Operators

| Operator | Description |
|----------|-------------|
| `and` | Logical AND |
| `or` | Logical OR |
| `!` | Logical NOT |

## Error Handling

The compiler provides clear error messages with helpful hints.

### Non-Boolean Condition

```xvr
var x = 5;
if (x) { }  // ERROR
```

Compiler output:
```
error: condition of if statement must be boolean, got 'i32'
help: use a comparison operator (e.g., 'x > 0') or wrap the condition with 'bool()'
```

### Expression If Without Type Annotation

```xvr
var x = if (true) { 1 } else { 2 };  // ERROR
```

Compiler output:
```
error: expression-based if requires explicit type annotation
help: declare the variable with a type: var x: int32 = if ...
```

## LLVM IR Generation

When compiled, `if` statements generate proper LLVM basic blocks:

### Statement Form

```llvm
entry:
  %cond = icmp sgt i32 %x, 10
  br i1 %cond, label %then, label %else

then:                                             ; preds = %entry
  ; then branch code
  br label %ifcont

else:                                             ; preds = %entry
  ; else branch code
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  ; execution continues here
```

### Expression Form

```llvm
entry:
  %result = alloca i32
  %cond = icmp sgt i32 %x, 10
  br i1 %cond, label %then, label %else

then:                                             ; preds = %entry
  store i32 100, ptr %result
  br label %ifcont

else:                                             ; preds = %entry
  store i32 0, ptr %result
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %value = load i32, ptr %result
  ; %value contains the result
```

The compiler generates:
- **Conditional branch** (`br i1`) based on the condition
- **Basic blocks** for then and else branches
- **Merge point** (`ifcont`) where control flow joins

## Best Practices

1. **Always use explicit boolean conditions** - Don't rely on implicit truthiness
2. **Keep conditions simple** - Complex conditions should be extracted to variables
3. **Prefer early returns** - Handle edge cases first
4. **Use consistent formatting** - Always use braces, even for single statements
5. **Use expression if for simple assignments** - Cleaner than if-else statements

```xvr
// Good - simple conditional assignment
var max: int32 = if (a > b) { a } else { b };

// Good - explicit statement form for complex logic
if (condition) {
    doSomething();
    doAnotherThing();
}

// Avoid
if (condition)
    doSomething();
```

## Summary

- `if` executes code when condition is `true`
- `else` executes when condition is `false`
- `else if` chains multiple conditions
- Conditions must be boolean
- Nested ifs are allowed but should be minimized
- **Expression-based if** returns values (requires type annotation)
- Error messages include helpful hints for fixing issues
