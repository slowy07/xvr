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
    print("wear jacket or use umbrella");
}
```

### If-Else Statement

```xvr
var today_raining: bool = false;

if (today_raining) {
    print("wear jacket or use umbrella");
} else {
    print("use sunglasses");
}
```

### If-Else-If Chain

For multiple conditions, use else-if:

```xvr
var score: int32 = 85;

if (score >= 90) {
    print("Grade: A");
} else if (score >= 80) {
    print("Grade: B");
} else if (score >= 70) {
    print("Grade: C");
} else {
    print("Grade: F");
}
```

### Nested If Statements

You can nest if statements inside each other:

```xvr
var age: int32 = 25;
var has_license: bool = true;

if (age >= 18) {
    if (has_license) {
        print("Can drive");
    } else {
        print("Need a license");
    }
} else {
    print("Too young to drive");
}
```

## Conditions

Conditions in if statements **must be boolean** (`bool`). The compiler will report an error if you use non-boolean types.

### Valid Conditions

```xvr
var x: int32 = 10;

// Comparison operators return boolean
if (x > 5) {
    print("x is greater than 5");
}

// Boolean variables work directly
var is_valid: bool = true;
if (is_valid) {
    print("Valid!");
}

// Logical operators
if (x > 5 and x < 20) {
    print("x is between 5 and 20");
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

## LLVM IR Generation

When compiled, `if` statements generate proper LLVM basic blocks:

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

The compiler generates:
- **Conditional branch** (`br i1`) based on the condition
- **Basic blocks** for then and else branches
- **Merge point** (`ifcont`) where control flow joins

## Best Practices

1. **Always use explicit boolean conditions** - Don't rely on implicit truthiness
2. **Keep conditions simple** - Complex conditions should be extracted to variables
3. **Prefer early returns** - Handle edge cases first
4. **Use consistent formatting** - Always use braces, even for single statements

```xvr
// Good
if (is_valid) {
    process();
}

// Avoid
if (is_valid)
    process();
```

## Summary

- `if` executes code when condition is `true`
- `else` executes when condition is `false`
- `else if` chains multiple conditions
- Conditions must be boolean
- Nested ifs are allowed but should be minimized

