# Chapter 3

## Working with variables

now we make some fun with variable, variable are label which is can defining the some value, its like some box called "slowy shoes box" and the shoes are inside the box. variable defined with `var` keyword

```
// defining the variable
var name = "arfy"

// can explicitly with some data types
var other: string = "james"

// cannot using space if using long variable name
var someNumber: int  = 30
var SomeFloating: float = 3.14
var sOmeStRing: string = "wello"
var soMeboOlean: bool = true
var somEArray: [int] = [1, 2, 3]
var sOmeData: [string: int] = ["pertama": 1, "kedua": 2]

// write to stdout
std::print(someNumber)
```

## Data types

data types are categorical value which is describe that value, for example: 
- integer value are like `20`, `1`, `-90`
- float value are like `3.14`, `-20.2`, `12.2`
- string value are like `"wello"`, `"xaviera"`, `"jhon"`, `"arfy slowy"`
- boolean value are like `true`, `false`
- array value are like `[1, 2, 3]`, `["arfy", "mamads"]`
- datatable value are like `["key": 12345]`

### Integer Types

XVR supports various integer types with different sizes:

| Type | Size | Range | Suffix |
|------|------|-------|--------|
| `int` | Platform default | ~±2 billion | (none) |
| `int8` | 8-bit | -128 to 127 | (type annotation) |
| `int16` | 16-bit | -32,768 to 32,767 | (type annotation) |
| `int32` | 32-bit | ~±2 billion | (type annotation) |
| `int64` | 64-bit | ~±9 quintillion | (type annotation) |
| `uint8` | 8-bit | 0 to 255 | (type annotation) |
| `uint16` | 16-bit | 0 to 65,535 | (type annotation) |
| `uint32` | 32-bit | 0 to ~4 billion | (type annotation) |
| `uint64` | 64-bit | 0 to ~18 quintillion | (type annotation) |

```xvr
// Integer literals
var a = 42           // int (default)
var b = 100i8        // int8
var c = 1000i16      // int16
var d = 10000i32     // int32
var e = 100000i64    // int64
var f = 255u8        // uint8
var g = 65535u16     // uint16

// Explicit type annotations
var num1: int = 42
var num2: int8 = 42
var num3: uint8 = 42
```

### Float Types

XVR supports floating-point types with different precisions:

| Type | Size | Precision | Suffix |
|------|------|----------|--------|
| `float` | Implementation-defined | ~6-15 digits | (none) |
| `float16` | 16-bit (half) | ~3 decimal digits | `f16` |
| `float32` | 32-bit (single) | ~7 decimal digits | `f32` |
| `float64` | 64-bit (double) | ~15 decimal digits | `f64` |

```xvr
// Float literals
var pi = 3.14159           // float (default)
var pi_f16 = 3.14f16       // float16
var pi_f32 = 3.14f32       // float32
var pi_f64 = 3.141592653589793f64  // float64

// Explicit type annotations
var x: float = 3.14
var y: float32 = 3.14
var z: float64 = 3.14
```

### Other Types

```xvr
// String
var greeting: string = "Hello, XVR!"

// Boolean
var isActive: bool = true
var isComplete: bool = false

// Array of integers
var numbers: [int] = [1, 2, 3, 4, 5]

// Array of strings
var names: [string] = ["arfy", "mamads", "jane"]

// Dictionary (key-value pairs)
var scores: [string: int] = ["arfy": 100, "jane": 95]

// Null
var empty = null
```
