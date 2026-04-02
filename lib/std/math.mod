# XVR Math Module
# Provides mathematical functions using LLVM intrinsics and libm fallback

module math

# Mathematical constants
const PI: float64 = 3.14159265358979323846
const E: float64 = 2.71828182845904523536

# Square root functions
native sqrt_f32(float32): float32
native sqrt_f64(float64): float64

# Power functions
native pow_f32(float32, float32): float32
native pow_f64(float64, float64): float64

# Trigonometric functions
native sin_f32(float32): float32
native sin_f64(float64): float64
native cos_f32(float32): float32
native cos_f64(float64): float64
native tan_f32(float32): float32
native tan_f64(float64): float64
native atan_f32(float32): float32
native atan_f64(float64): float64
native atan2_f32(float32, float32): float32
native atan2_f64(float64, float64): float64

# Absolute value functions
native abs_i32(int32): int32
native abs_i64(int64): int64
native abs_f32(float32): float32
native abs_f64(float64): float64

# Rounding functions
native floor_f32(float32): float32
native floor_f64(float64): float64
native ceil_f32(float32): float32
native ceil_f64(float64): float64
native round_f32(float32): float32
native round_f64(float64): float64

# Logarithm and exponential
native log_f32(float32): float32
native log_f64(float64): float64
native exp_f32(float32): float32
native exp_f64(float64): float64
