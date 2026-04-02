# Math Module Documentation

## Overview

The XVR math module provides comprehensive mathematical functions for the XVR programming language. It is implemented using LLVM intrinsics for efficient code generation and libm for functions without LLVM intrinsics.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         XVR Math Module Architecture                         │
└─────────────────────────────────────────────────────────────────────────────┘

  User Code                    Parser                    LLVM Backend
┌─────────────────┐      ┌─────────────────┐       ┌─────────────────────────┐
│                 │      │                 │       │                         │
│ include math;   │      │  AST Node       │       │  xvr_llvm_expression_   │
│                 │─────▶│  Generation     │──────▶│  emitter.c              │
│ var x =         │      │                 │       │                         │
│   math::sqrt(   │      │                 │       │                         │
│     25.0);      │      │                 │       │                         │
│                 │      │                 │       │                         │
└─────────────────┘      └─────────────────┘       └─────────────────────────┘
                                                           │
                                                           ▼
                                                 ┌─────────────────────────┐
                                                 │   Function Dispatch     │
                                                 │                         │
                                                 │  ┌─────────────────┐    │
                                                 │  │ math::sqrt      │────┼──▶ LLVM Intrinsic
                                                 │  ├─────────────────┤    │
                                                 │  │ math::sin       │────┼──▶ LLVM Intrinsic
                                                 │  ├─────────────────┤    │
                                                 │  │ math::sinh      │────┼──▶ libm (sinhf)
                                                 │  ├─────────────────┤    │
                                                 │  │ math::fmod      │────┼──▶ libm (fmodf)
                                                 │  └─────────────────┘    │
                                                 └─────────────────────────┘
                                                           │
                                                           ▼
                                                 ┌─────────────────────────┐
                                                 │   Constant Folding      │
                                                 │   (Parser)              │
                                                 │                         │
                                                 │  ┌─────────────────┐    │
                                                 │  │ math::sqrt(25)  │────┼──▶ 5.0 (compile-time)
                                                 │  ├─────────────────┤    │
                                                 │  │ math::sin(0)    │────┼──▶ 0.0 (compile-time)
                                                 │  └─────────────────┘    │
                                                 └─────────────────────────┘
```

## Function Categories

### Basic Functions

| Function | Description | LLVM Intrinsic | Type Support |
|----------|-------------|----------------|-------------|
| `sqrt(x)` | Square root | `llvm.sqrt.f32/f64` | float32, float64 |
| `pow(x, y)` | Power (x^y) | `llvm.pow.f32/f64` | float32, float64 |
| `abs(x)` | Absolute value | N/A (builtin) | int32, int64, float32, float64 |

### Trigonometric Functions

| Function | Description | LLVM Intrinsic | libm Fallback |
|----------|-------------|----------------|---------------|
| `sin(x)` | Sine | `llvm.sin.f32/f64` | - |
| `cos(x)` | Cosine | `llvm.cos.f32/f64` | - |
| `tan(x)` | Tangent | `llvm.tan.f32/f64` | - |

### Inverse Trigonometric Functions

| Function | Description | libm Function | Notes |
|----------|-------------|--------------|-------|
| `asin(x)` | Arc sine | `asinf`, `asin` | Domain: [-1, 1] |
| `acos(x)` | Arc cosine | `acosf`, `acos` | Domain: [-1, 1] |
| `atan(x)` | Arc tangent | `atanf`, `atan` | Range: [-π/2, π/2] |
| `atan2(y, x)` | 2-argument arctangent | `atan2f`, `atan2` | All quadrants |

### Logarithmic Functions

| Function | Description | LLVM Intrinsic | libm Fallback |
|----------|-------------|----------------|---------------|
| `log(x)` | Natural logarithm | `llvm.log.f32/f64` | - |
| `log10(x)` | Base-10 logarithm | - | `log10f`, `log10` |
| `log2(x)` | Base-2 logarithm | - | `log2f`, `log2` |

### Exponential Functions

| Function | Description | LLVM Intrinsic |
|----------|-------------|----------------|
| `exp(x)` | e^x | `llvm.exp.f32/f64` |

### Rounding Functions

| Function | Description | LLVM Intrinsic |
|----------|-------------|----------------|
| `floor(x)` | Round down | `llvm.floor.f32/f64` |
| `ceil(x)` | Round up | `llvm.ceil.f32/f64` |
| `round(x)` | Round to nearest | `llvm.round.f32/f64` |
| `trunc(x)` | Truncate | `llvm.trunc.f32/f64` |

### Hyperbolic Functions

| Function | Description | libm Function |
|----------|-------------|--------------|
| `sinh(x)` | Hyperbolic sine | `sinhf`, `sinh` |
| `cosh(x)` | Hyperbolic cosine | `coshf`, `cosh` |
| `tanh(x)` | Hyperbolic tangent | `tanhf`, `tanh` |

### Inverse Hyperbolic Functions

| Function | Description | libm Function |
|----------|-------------|--------------|
| `asinh(x)` | Inverse hyperbolic sine | `asinhf`, `asinh` |
| `acosh(x)` | Inverse hyperbolic cosine | `acoshf`, `acosh` |
| `atanh(x)` | Inverse hyperbolic tangent | `atanhf`, `atanh` |

### Other Functions

| Function | Description | libm Function |
|----------|-------------|--------------|
| `fmod(x, y)` | Floating-point remainder | `fmodf`, `fmod` |

## Constant Folding

The math module supports compile-time constant folding for numeric literals.

```
┌─────────────────────────────────────────────────────────────────┐
│                    Constant Folding Flow                          │
└─────────────────────────────────────────────────────────────────┘

  Source Code                    Parser                      Output
┌─────────────────┐      ┌─────────────────┐       ┌─────────────────┐
│                 │      │                 │       │                 │
│ var x =         │      │  detect numeric │       │ var x = 5.0;    │
│   math::sqrt(   │─────▶│  literals       │──────▶│   (pre-computed │
│     25.0);      │      │                 │       │    at compile   │
│                 │      │                 │       │    time)        │
└─────────────────┘      └─────────────────┘       └─────────────────┘

  Supported Functions for Constant Folding:
  ┌─────────────────────────────────────────────────────────┐
  │ sqrt, sin, cos, tan, asin, acos, atan                   │
  │ log, log10, log2, exp                                   │
  │ floor, ceil, round, trunc                               │
  │ sinh, cosh, tanh, asinh, acosh, atanh                   │
  │ abs, pow, atan2, fmod                                   │
  └─────────────────────────────────────────────────────────┘
```

### Example

```xvr
// Compile-time evaluation
var a = math::sqrt(25.0);    // a = 5.0
var b = math::sin(0.0);      // b = 0.0
var c = math::log(1.0);      // c = 0.0
var d = math::log2(1024.0);  // d = 10.0

// Runtime evaluation
var e = math::sqrt(x);       // Computed at runtime
```

## Type System

### Type Promotion

```
┌─────────────────────────────────────────────────────────────────┐
│                    Type Resolution Flow                         │
└─────────────────────────────────────────────────────────────────┘

  Function Call                    Type Check                    Result
┌─────────────────┐      ┌─────────────────┐       ┌─────────────────┐
│                 │      │                 │       │                 │
│ math::sqrt(     │      │ determine arg   │       │ Generate        │
│   25.0)         │─────▶│ type            │──────▶│ llvm.sqrt.f32   │
│                 │      │   float32       │       │ (single         │
│                 │      │                 │       │  precision)     │
└─────────────────┘      └─────────────────┘       └─────────────────┘

  Type Mapping:
  ┌─────────────────────────────────────────────────────────┐
  │ float32  ────────────────────▶ llvm.sqrt.f32            │
  │ float64  ────────────────────▶ llvm.sqrt.f64            │
  │ int32    ────────────────────▶ (cast to float32)        │
  │ int64    ────────────────────▶ (cast to float64)        │
  └─────────────────────────────────────────────────────────┘
```

## Implementation Details

### LLVM Intrinsic Selection

```c
// From xvr_llvm_expression_emitter.c

if (kind == LLVMFloatTypeKind) {
    // Use float32 intrinsic
    LLVMValueRef sqrt_fn = LLVMGetNamedFunction(module, "llvm.sqrt.f32");
    if (!sqrt_fn) {
        sqrt_fn = LLVMAddFunction(module, "llvm.sqrt.f32", fn_type);
    }
    return LLVMBuildCall2(builder, fn_type, sqrt_fn, &arg, 1, "sqrt_f32");
} else if (kind == LLVMDoubleTypeKind) {
    // Use float64 intrinsic
    LLVMValueRef sqrt_fn = LLVMGetNamedFunction(module, "llvm.sqrt.f64");
    if (!sqrt_fn) {
        sqrt_fn = LLVMAddFunction(module, "llvm.sqrt.f64", fn_type);
    }
    return LLVMBuildCall2(builder, fn_type, sqrt_fn, &arg, 1, "sqrt_f64");
}
```

### libm Fallback Selection

```c
// For functions without LLVM intrinsics (e.g., sinh)

if (kind == LLVMFloatTypeKind) {
    // Use float32 libm function
    LLVMValueRef sinh_fn = LLVMGetNamedFunction(module, "sinhf");
    if (!sinh_fn) {
        sinh_fn = LLVMAddFunction(module, "sinhf", fn_type);
    }
    return LLVMBuildCall2(builder, fn_type, sinh_fn, &arg, 1, "sinh_f32");
}
```

## IEEE 754 Compliance

Math functions follow IEEE 754 behavior for edge cases:

```
┌─────────────────────────────────────────────────────────────────┐
│                    IEEE 754 Edge Cases                          │
└─────────────────────────────────────────────────────────────────┘

  Input                    Function              Output
┌─────────────────┐      ┌─────────────────┐   ┌─────────────────┐
│                 │      │                 │   │                 │
│ sqrt(-1.0)      │─────▶│ sqrt            │──▶│ NaN             │
│                 │      │                 │   │                 │
├─────────────────┤      ├─────────────────┤   ├─────────────────┤
│                 │      │                 │   │                 │
│ log(0.0)        │─────▶│ log             │──▶│ -Infinity       │
│                 │      │                 │   │                 │
├─────────────────┤      ├─────────────────┤   ├─────────────────┤
│                 │      │                 │   │                 │
│ log(-1.0)       │─────▶│ log             │──▶│ NaN             │
│                 │      │                 │   │                 │
├─────────────────┤      ├─────────────────┤   ├─────────────────┤
│                 │      │                 │   │                 │
│ exp(1000.0)     │─────▶│ exp             │──▶│ Infinity        │
│                 │      │                 │   │                 │
├─────────────────┤      ├─────────────────┤   ├─────────────────┤
│                 │      │                 │   │                 │
│ acosh(0.5)      │─────▶│ acosh           │──▶│ NaN (domain     │
│                 │      │                 │   │  error)         │
└─────────────────┘      └─────────────────┘   └─────────────────┘
```

### Domain Restrictions

| Function | Domain | Outside Domain |
|---------|--------|---------------|
| `asin(x)` | [-1, 1] | NaN |
| `acos(x)` | [-1, 1] | NaN |
| `acosh(x)` | [1, ∞) | NaN |
| `atanh(x)` | (-1, 1) | NaN (at boundaries) |
| `log(x)` | (0, ∞) | -Infinity or NaN |
| `log10(x)` | (0, ∞) | -Infinity or NaN |
| `log2(x)` | (0, ∞) | -Infinity or NaN |

## Compilation Pipeline

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Math Module Compilation Pipeline                         │
└─────────────────────────────────────────────────────────────────────────────┘

  1. Lexing & Parsing
  ┌───────────────────────────────────────────────────────────────────────────┐
  │                                                                           │
  │  Source: "include math; var x = math::sqrt(25.0);"                        │
  │          │                                                                │
  │          ▼                                                                │
  │  Tokens: [INCLUDE, IDENT(math), SEMICOLON, VAR, IDENT(x), ASSIGN,         │
  │            IDENT(math), SCOPE_RESOLVE, IDENT(sqrt), LPAREN,               │
  │            FLOAT(25.0), RPAREN, SEMICOLON]                                │
  │          │                                                                │
  │          ▼                                                                │
  │  AST: ProgramNode                                                         │
  │        ├── IncludeNode("math")                                            │
  │        └── VarDeclNode("x")                                               │
  │              └── FnCallNode("math::sqrt")                                 │
  │                    └── LiteralNode(25.0)                                  │
  │                                                                           │
  └───────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
  2. Constant Folding (Parser)
  ┌───────────────────────────────────────────────────────────────────────────┐
  │                                                                           │
  │  Before: FnCallNode("math::sqrt", [LiteralNode(25.0)])                    │
  │          │                                                                │
  │          ▼ (calcStaticMathFn)                                             │
  │  After:  LiteralNode(5.0)  // sqrt(25) = 5 at compile-time                │
  │                                                                           │
  └───────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
  3. LLVM IR Generation
  ┌───────────────────────────────────────────────────────────────────────────┐
  │                                                                           │
  │  @fmt_str = private constant [10 x i8] c"x = %lf\0A\00"                   │
  │                                                                           │
  │  define i32 @main() {                                                     │
  │  entry:                                                                   │
  │    ; Constant folded: sqrt(25.0) = 5.0                                    │
  │    %x = alloca float                                                      │
  │    store float 0x401400000, ptr %x                                        │
  │    ; Runtime call needed for user input                                   │
  │    ; %sqrt_call = call float @llvm.sqrt.f32(float %user_input)            │
  │    ret i32 0                                                              │
  │  }                                                                        │
  │                                                                           │
  └───────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
  4. Linking & Execution
  ┌───────────────────────────────────────────────────────────────────────────┐
  │                                                                           │
  │  Compilation: xvr source.xvr                                              │
  │          │                                                                │
  │          ▼                                                                │
  │  gcc -c source.o -o /tmp/xvr_runtime.o                                    │
  │          │                                                                │
  │          ▼                                                                │
  │  gcc source.o /tmp/xvr_runtime.o -lm -o executable                        │
  │          │                                                                │
  │          ▼                                                                │
  │  Execution: ./executable                                                  │
  │          │                                                                │
  │          ▼                                                                │
  │  Output: x = 5.000000                                                     │
  │                                                                           │
  └───────────────────────────────────────────────────────────────────────────┘
```

## Error Handling

### Compilation Errors

| Error | Cause | Example |
|-------|-------|---------|
| Type mismatch | Non-numeric argument | `math::sqrt("hello")` |
| Argument count | Wrong number of arguments | `math::sqrt(1.0, 2.0)` |
| Module not found | `math` not included | `var x = math::sqrt(4.0)` without `include math;` |

### Runtime Behavior

Per IEEE 754, functions do not throw exceptions but return special values:

```
┌─────────────────────────────────────────────────────────────────┐
│                    Runtime Behavior                             │
└─────────────────────────────────────────────────────────────────┘

  ┌─────────────┐     ┌─────────────────┐     ┌─────────────────┐
  │   Input     │────▶│   Function      │────▶│   Output        │
  └─────────────┘     └─────────────────┘     └─────────────────┘

  Examples:
  ┌─────────────────────────────────────────────────────────────────┐
  │  math::sqrt(-1.0)      →  NaN  (not an error)                   │
  │  math::log(0.0)       → -Inf  (not an error)                    │
  │  math::pow(0.0, -1.0) →  Inf  (not an error)                    │
  └─────────────────────────────────────────────────────────────────┘
```

## Performance Considerations

### Intrinsic vs libm

| Category | Implementation | Performance |
|----------|---------------|-------------|
| Has LLVM intrinsic | LLVM codegen | Optimal (inlined) |
| libm fallback | External function call | Good (optimized) |

### Optimization Opportunities

1. **Constant Folding**: Expressions with literal arguments are evaluated at compile-time
2. **Type Specialization**: Separate code generated for float32 and float64
3. **Dead Code Elimination**: Unused math results can be eliminated

## Testing

### Test Categories

```
┌─────────────────────────────────────────────────────────────────┐
│                    Math Module Test Coverage                    │
└─────────────────────────────────────────────────────────────────┘

  ┌───────────────────┐    ┌───────────────────┐    ┌───────────────────┐
  │   Basic Tests     │    │  Edge Case Tests  │    │  Stress Tests     │
  ├───────────────────┤    ├───────────────────┤    ├───────────────────┤
  │ sqrt(25) = 5      │    │ sqrt(-1) = NaN    │    │ Loop: 10000 iters │
  │ pow(2, 3) = 8     │    │ log(0) = -Inf     │    │ Nested functions  │
  │ sin(0) = 0        │    │ exp(1000) = Inf   │    │ Combined exprs    │
  └───────────────────┘    └───────────────────┘    └───────────────────┘
           │                         │                         │
           └─────────────────────────┴─────────────────────────┘
                                   │
                                   ▼
                     ┌───────────────────────────┐
                     │     60+ Test Cases        │
                     │     ALL PASSING           │
                     └───────────────────────────┘
```

### Test Files

- `code/test_math_all.xvr` - Comprehensive function tests
- `code/test_math_const.xvr` - Constant folding tests
- `code/test_math_edge.xvr` - Edge case tests
- `code/test_math_stress.xvr` - Stress/loop tests
- `fuzzer/test_fuzz_math_v2.c` - Fuzzing tests

## Usage Examples

### Basic Usage

```xvr
include math;
include std;

var a = math::sqrt(25.0);
var b = math::pow(2.0, 10.0);
var c = math::sin(math::PI / 2.0);

std::println("sqrt(25) = {}", a);     // sqrt(25) = 5.000000
std::println("2^10 = {}", b);         // 2^10 = 1024.000000
std::println("sin(PI/2) = {}", c);    // sin(PI/2) = 1.000000
```

### Trigonometric Calculations

```xvr
include math;
include std;

var angle = 45.0;
var radians = angle * (math::PI / 180.0);

var sin_val = math::sin(radians);
var cos_val = math::cos(radians);
var tan_val = math::tan(radians);

std::println("sin(45°) = {}", sin_val);
std::println("cos(45°) = {}", cos_val);
std::println("tan(45°) = {}", tan_val);
```

### Logarithmic Calculations

```xvr
include math;
include std;

var x = 1024.0;

std::println("ln({}) = {}", x, math::log(x));      // ln(1024) = 6.931472
std::println("log10({}) = {}", x, math::log10(x)); // log10(1024) = 3.000000
std::println("log2({}) = {}", x, math::log2(x));   // log2(1024) = 10.000000
```

### Hyperbolic Functions

```xvr
include math;
include std;

var x = 1.0;

std::println("sinh({}) = {}", x, math::sinh(x));
std::println("cosh({}) = {}", x, math::cosh(x));
std::println("tanh({}) = {}", x, math::tanh(x));

// Verify identity: cosh²(x) - sinh²(x) = 1
var identity = math::pow(math::cosh(x), 2.0) - math::pow(math::sinh(x), 2.0);
std::println("cosh² - sinh² = {}", identity);  // = 1.000000
```

### Combined Expressions

```xvr
include math;
include std;

// Pythagorean identity: sin²(x) + cos²(x) = 1
var angle = 0.5;
var sin_sq = math::pow(math::sin(angle), 2.0);
var cos_sq = math::pow(math::cos(angle), 2.0);
var identity = sin_sq + cos_sq;

std::println("sin² + cos² = {}", identity);  // = 1.000000
```

## See Also

- [Standard Library Documentation](stdlib.md)
- [LLVM Backend](LLVM.md)
- [Parser Implementation](../src/xvr_parser.c)
- [Expression Emitter](../src/backend/xvr_llvm_expression_emitter.c)
