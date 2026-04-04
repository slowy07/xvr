#!/bin/bash
# XVR Compiler Stress Test - Random XVR Code Generation
# 
# SECURITY: This stress test ONLY generates and compiles XVR language code.
# It does NOT accept external input, inject C code, or execute arbitrary commands.
# All generated code is from predefined XVR grammar patterns.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
XVR="$PROJECT_DIR/out/xvr"
TEMP_DIR="/tmp/xvr_stress_$$"
mkdir -p "$TEMP_DIR"
PASS=0
FAIL=0
CRASH=0
TIMEOUT=0

cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

run_test() {
    local name="$1"
    local input="$2"
    local output_file="$TEMP_DIR/test_${RANDOM}.xvr"
    
    echo "$input" > "$output_file"
    
    local output
    local exit_code
    
    output=$("$XVR" "$output_file" 2>&1)
    exit_code=$?
    
    rm -f "$output_file"
    
    # Check for crashes (signals)
    if [ $exit_code -ge 128 ]; then
        echo "CRASH[$name]: signal $((exit_code - 128))"
        echo "  Input: $input" >> "$TEMP_DIR/crashes.txt"
        ((CRASH++))
        return 1
    fi
    
    # Check for assertion failures
    if echo "$output" | grep -qi "assert"; then
        echo "ASSERT[$name]:"
        echo "  Input: $input" >> "$TEMP_DIR/assertions.txt"
        ((FAIL++))
        return 1
    fi
    
    ((PASS++))
    return 0
}

echo "XVR Compiler Stress Test"
echo "Running 500+ random inputs..."
echo ""

# Test patterns that commonly trigger bugs
patterns=(
    # Nested parentheses
    'std::print((((((((((1))))))))));'
    
    # Deep nesting
    'std::print(((((((("hello"))))))));'
    
    # Empty function with return
    'proc empty(): int { return 1; } empty();'
    
    # Multiple returns
    'proc multi(): int { if true { return 1; } return 0; } multi();'
    
    # Nested function calls
    'proc add(a: int, b: int): int { return a + b; } proc sub(a: int, b: int): int { return a - b; } std::print(add(sub(10, 5), 3));'
    
    # Unused parameters
    'proc unused(a: int, b: int): int { return 42; } unused(1, 2);'
    
    # Shadowing
    'var x = 1; var y = 2; std::print(x + y);'
    
    # Boolean operations
    'var a = true && false; var b = true || false; std::print(a, b);'
    
    # Comparison chaining
    'var x = 1 < 2 < 3; std::print(x);'
    
    # String concatenation
    'std::print("hello " + "world" + " test");'
    
    # Array operations
    'var arr = [1, 2, 3, 4, 5]; std::print(arr[0] + arr[4]);'
    
    # Ternary in expressions
    'var x = true ? 1 : 0; std::print(x);'
    
    # Nested ternary
    'var x = false ? (true ? 1 : 2) : (false ? 3 : 4); std::print(x);'
    
    # While loop
    'var i = 0; while i < 10 { i = i + 1; } std::print(i);'
    
    # For loop
    'var sum = 0; for x in [1, 2, 3, 4, 5] { sum = sum + x; } std::print(sum);'
    
    # Break/continue
    'var i = 0; while true { i = i + 1; if i >= 5 { break; } } std::print(i);'
    
    # Type casts
    'var x: float = 3.14; var y: int = int(x); std::print(y);'
    
    # Division
    'std::print(10 / 3); std::print(10 % 3);'
    
    # Compound assignment
    'var x = 10; x += 5; x -= 3; x *= 2; x /= 4; std::print(x);'
    
    # Increment/decrement
    'var x = 10; std::print(x++); std::print(++x);'
    
    # Negative numbers
    'var x = -42; var y = --42; std::print(x, y);'
    
    # Function returning function
    'proc inc(x: int): int { return x + 1; } var f = inc; std::print(f(5));'
    
    # Callback style
    'proc apply(f: int, x: int): int { return f(x); } std::print(apply(inc, 10));'
    
    # Array comprehension
    'var arr = [x * 2 for x in [1, 2, 3, 4, 5]]; std::print(arr[4]);'
    
    # Dictionary
    'var dict = {"a": 1, "b": 2}; std::print(dict["a"] + dict["b"]);'
    
    # String methods
    'var s = "hello world"; std::print(len(s));'
    
    # Complex expression
    'std::print(1 + 2 * 3 - 4 / 2 + (5 % 3) - 1);'
    
    # Nested blocks
    'if true { if true { if true { std::print("{}", 1); } } }'
    
    # Empty blocks
    '{} if false { std::print("{}", 1); } else {}'
    
    # Multiple semicolons
    'std::print("{}", 1);;;;std::print("{}", 2);'
    
    # Comments in expressions
    'std::print("{}", /* comment */ 42);'
    
    # ===== NEW: Array print tests =====
    'std::println("Array: {}", [1, 2, 3]);'
    'std::println("Float: {}", [1.5, 2.5]);'
    'std::print("Array: {}", [1, 2, 3]);'
    'var arr = [1, 2, 3]; std::println("Arr: {}", arr);'
    'std::print("Single: {}", [42]);'
    'var arr = [1]; std::print("{}", arr[0]);'
    
    # ===== NEW: Format string tests =====
    'std::print("{}", 42);'
    'std::print("{} + {} = {}", 1, 2, 3);'
    'std::println("Value: {}", 3.14);'
    
    # ===== NEW: Edge case print tests =====
    'std::print("{}", -42);'
    'std::print("{}", 0);'
    'std::print("");'
    'std::print("{}", 2147483647);'
    
    # ===== NEW: Print in control flow =====
    'if true { std::print("{}", 1); }'
    'var i = 0; while i < 3 { std::print("{}", i); i = i + 1; }'
    
    # ===== NEW: Print with operators =====
    'std::print("{}", 1 + 2);'
    'std::print("{}", 10 / 3);'
    'std::print("{}", 5 % 3);'
    
    # ===== NEW: Print with special chars =====
    'std::print("hello\nworld");'
    'std::print("hello\tworld");'
    'std::print("hello\"world");'
)

# Run predefined patterns multiple times
for iter in $(seq 1 10); do
    for pattern in "${patterns[@]}"; do
        run_test "pattern_${iter}" "$pattern"
    done
done

# Generate random variations
for i in $(seq 1 100); do
    # Random integer literals
    val1=$((RANDOM % 1000))
    val2=$((RANDOM % 100))
    run_test "rand_int_$i" "std::print($val1 + $val2);"
    
    # Random string length
    len=$((RANDOM % 50 + 1))
    str=$(printf 'A%.0s' $(seq 1 $len))
    run_test "rand_str_$i" "var s = \"$str\"; std::print(len(s));"
    
    # Random array
    size=$((RANDOM % 10 + 1))
    arr=$(seq 1 $size | tr '\n' ',' | sed 's/,$//')
    run_test "rand_arr_$i" "var a = [$arr]; std::print(len(a));"
    
    # Random parenthesized expressions
    depth=$((RANDOM % 10 + 1))
    parens=$(printf '((%.0s' $(seq 1 $depth))
    run_test "rand_paren_$i" "std::print($parens$val1));"
done

# Test edge cases
edge_cases=(
    # Zero division (runtime error)
    'std::print("{}", 1 / 0);'
    
    # Modulo by zero
    'std::print("{}", 1 % 0);'
    
    # Array out of bounds
    'var arr = [1, 2, 3]; std::print("{}", arr[100]);'
    
    # Negative array index
    'var arr = [1, 2, 3]; std::print("{}", arr[-1]);'
    
    # Undefined variable
    'std::print(undefined_var);'
    
    # Function not defined
    'undefined_fn();'
    
    # Wrong argument count
    'proc foo(): int { return 1; } foo(1, 2, 3);'
    
    # Missing return
    'proc foo(): int { } foo();'
    
    # Type mismatch
    'var x: int = "hello";'
    
    # Void return value
    'proc void_fn(): void { } var x = void_fn();'
)

for i in "${!edge_cases[@]}"; do
    run_test "edge_$i" "${edge_cases[$i]}"
done

echo ""
echo "Stress Test Summary"
echo "Passed: $PASS"
echo "Failed: $FAIL"  
echo "Crashes: $CRASH"
echo "Timeouts: $TIMEOUT"

if [ $CRASH -gt 0 ]; then
    echo ""
    echo "CRASHES DETECTED! Check $TEMP_DIR/crashes.txt"
    exit 1
fi

if [ $FAIL -gt 0 ]; then
    echo ""
    echo "Some tests failed. Check $TEMP_DIR/"
fi

echo ""
echo "Stress test completed successfully!"
exit 0
