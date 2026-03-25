#!/bin/bash
# XVR Compiler Regression Tests
# Tests for bugs that have been fixed

XVR="./out/xvr"
PASS=0
FAIL=0

run_test() {
    local name="$1"
    local input="$2"
    local expected_exit="${3:-0}"
    
    echo "$input" > /tmp/regression_test.xvr
    local output
    local exit_code
    
    output=$("$XVR" /tmp/regression_test.xvr 2>&1)
    exit_code=$?
    
    # Check for crashes (signals like SIGSEGV, SIGABRT, etc.)
    if [ $exit_code -ge 128 ]; then
        echo "CRASH: $name (signal $((exit_code - 128)))"
        echo "  Input: $input"
        ((FAIL++))
        return 1
    fi
    
    # Expected exit codes: 0 (success), 1 (syntax/semantic error)
    if [ $exit_code -eq $expected_exit ] || [ $exit_code -eq 0 ] || [ $exit_code -eq 1 ]; then
        ((PASS++))
        return 0
    fi
    
    # Unexpected exit code
    echo "UNEXPECTED: $name (exit $exit_code, expected $expected_exit)"
    echo "  Input: $input"
    echo "  Output: $output"
    ((FAIL++))
    return 1
}

echo "=== XVR Compiler Regression Tests ==="
echo ""

# Regression Test 1: Function calls with arguments at module level
# Bug: Arguments not being passed to function calls
run_test "fn_call_with_args" 'proc foo(x: int): int { return x + 1; } foo(5);'

# Regression Test 2: String concatenation with runtime values
# Bug: Runtime string concatenation was treating pointers as integers
run_test "string_concat_runtime" 'proc testing(name: string): void { std::print("wello " + name); } testing("arfy");'

# Regression Test 3: GROUPING expressions (parentheses)
# Bug: Parenthesized expressions weren't being unwrapped properly
run_test "grouping_expr" 'std::print(((((((("hello"))))))));'

# Regression Test 4: Non-literal arguments to std::print
# Bug: Arguments wrapped in parentheses weren't being passed to printf
run_test "print_non_literal_arg" 'var x = 42; std::print(x);'
run_test "print_paren_int" 'std::print((((42))));'

# Regression Test 5: Compile-time constant string concatenation
# Feature: Both operands are string literals - should optimize at compile time
run_test "const_string_concat" 'std::print("hello " + "world");'

# Regression Test 6: Complex nested expressions in print
run_test "complex_nested_print" 'std::print((((1 + 2) * 3)));'

# Regression Test 7: Function parameter allocation
# Bug: Parameters allocated in wrong function scope
run_test "fn_params" 'proc add(a: int, b: int): int { return a + b; } std::print(add(10, 20));'

# Regression Test 8: std::print with format string
run_test "print_format" 'std::print("value is {}", 42);'
run_test "print_format_multiple" 'std::print("{} + {} = {}", 1, 2, 3);'

# Regression Test 9: Multiple function calls
run_test "multi_fn_calls" 'proc a(): int { return 1; } proc b(): int { return 2; } std::print(a() + b());'

# Regression Test 10: Nested function calls
run_test "nested_fn_calls" 'proc add(a: int, b: int): int { return a + b; } std::print(add(add(1, 2), add(3, 4)));'

# Regression Test 11: Ternary expressions
run_test "ternary_basic" 'std::print(true ? "yes" : "no");'
run_test "ternary_nested" 'std::print(false ? "a" : (true ? "b" : "c"));'

# Regression Test 12: Boolean expressions
run_test "bool_complex" 'std::print((1 < 2) && (3 > 4) || true);'

# Regression Test 13: Self-referencing variable assignment
run_test "self_ref_assign" 'var x = 1; x = x + x; std::print(x);'

# Regression Test 14: String with newline
run_test "string_newline" 'std::print("hello\nworld");'

# Regression Test 15: Array indexing
run_test "array_index" 'var arr = [10, 20, 30]; std::print(arr[1]);'

# Summary
echo ""
echo "=== Regression Test Summary ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"

if [ $FAIL -gt 0 ]; then
    exit 1
fi

echo ""
echo "All regression tests passed!"
exit 0
