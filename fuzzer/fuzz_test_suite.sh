#!/bin/bash
# XVR Compiler Fuzzing Test Suite
# Tests edge cases and potential bug triggers

XVR="./out/xvr"
TEMP_DIR="/tmp/xvr_fuzz_$$"
mkdir -p "$TEMP_DIR/crashes"
PASS=0
FAIL=0
CRASH=0

cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

run_test() {
    local name="$1"
    local input="$2"
    local expected_exit="${3:-0}"
    local expected_output="$4"
    
    echo "$input" > "$TEMP_DIR/test.xvr"
    
    local output
    local exit_code
    
    output=$("$XVR" "$TEMP_DIR/test.xvr" 2>&1)
    exit_code=$?
    
    # Check for crashes (signals)
    if [ $exit_code -ge 128 ]; then
        echo "CRASH: $name (signal $((exit_code - 128)))"
        echo "  Input: $input" >> "$TEMP_DIR/crashes/crashes.txt"
        echo "  Signal: $((exit_code - 128))" >> "$TEMP_DIR/crashes/crashes.txt"
        ((CRASH++))
        return 1
    fi
    
    # Check for compiler errors (exit code 1 is expected for syntax errors)
    if [ $exit_code -eq 0 ] || [ $exit_code -eq 1 ]; then
        ((PASS++))
        return 0
    fi
    
    # Unexpected exit code
    echo "UNEXPECTED: $name (exit $exit_code)"
    echo "  Input: $input" >> "$TEMP_DIR/crashes/unexpected.txt"
    ((FAIL++))
    return 1
}

echo "=== XVR Compiler Fuzzing Test Suite ==="
echo ""

# Test 1: Empty input
run_test "empty" ""

# Test 2: Whitespace only
run_test "whitespace" "   \n\t  "

# Test 3: Random keywords
run_test "random_keywords" "proc var if else while for return"

# Test 4: Unterminated strings
run_test "unterminated_string" 'std::print("hello'
run_test "unterminated_string2" "std::print('hello)"
run_test "unterminated_string3" 'std::print("hello\nworld")'

# Test 5: Nested parentheses
run_test "nested_parens_1" "std::print((((1))));"
run_test "nested_parens_2" "std::print((((((1))))));"
run_test "nested_parens_10" "std::print((((((((((1))))))))));"

# Test 6: Unbalanced parentheses
run_test "unbalanced_parens_1" "std::print(1;"
run_test "unbalanced_parens_2" "std::print(1));"
run_test "unbalanced_parens_3" "std::print(1"

# Test 7: Empty function
run_test "empty_proc" "proc foo() { }"
run_test "empty_proc_with_ret" "proc foo(): int { return; }"

# Test 8: Deep nesting
run_test "deep_nesting" 'std::print(((((((((((("hello"))))))))));'

# Test 9: Invalid numbers
run_test "hex_number" "std::print(0xFF);"
run_test "bin_number" "std::print(0b1010);"
run_test "float_number" "std::print(3.14);"

# Test 10: Special characters in strings
run_test "string_newline" 'std::print("hello\nworld");'
run_test "string_tab" 'std::print("hello\tworld");'
run_test "string_quote" 'std::print("hello\"world");'
run_test "string_backslash" 'std::print("hello\\world");'

# Test 11: Zero-length everything
run_test "zero_var" "var x: int = 0;"
run_test "zero_array" "var arr: [int] = [];"
run_test "empty_string" 'var s = "";'

# Test 12: Maximum values
run_test "max_int" "var x: int = 2147483647;"
run_test "min_int" "var x: int = -2147483648;"

# Test 13: Division edge cases
run_test "div_zero" "var x = 1 / 0;"
run_test "mod_zero" "var x = 1 % 0;"

# Test 14: Undefined identifiers
run_test "undefined_var" "var x = undefined_var;"
run_test "undefined_fn" "undefined_fn();"

# Test 15: Shadowing
run_test "shadow_builtin" "proc std() { }"
run_test "shadow_var" "var x = 1; var x = 2;"

# Test 16: Recursion
run_test "infinite_recursion" "proc foo() { foo(); } foo();"

# Test 17: Type mismatches
run_test "type_mismatch_1" 'var x: int = "hello";'
run_test "type_mismatch_2" "var x: int = true;"
run_test "type_mismatch_3" "var x: bool = 42;"

# Test 18: Array operations
run_test "array_out_of_bounds" "var arr = [1, 2, 3]; arr[100];"
run_test "array_negative_index" "var arr = [1, 2, 3]; arr[-1];"
run_test "array_empty_access" "var arr = []; arr[0];"

# Test 19: String operations
run_test "string_concat" 'var s = "hello" + "world";'
run_test "string_concat_runtime" 'proc foo(s) { std::print("hello" + s); } foo("world");'

# Test 20: Function argument count
run_test "too_many_args" "proc foo() { } foo(1, 2, 3);"
run_test "too_few_args" "proc foo(a: int, b: int) { } foo(1);"

# Test 21: Return in void function
run_test "return_value_void" "proc foo(): void { return 42; }"

# Test 22: Control flow
run_test "unreachable_return" "proc foo(): int { return 1; return 2; }"
run_test "missing_return" "proc foo(): int { }"

# Test 23: Comments (if supported)
run_test "line_comment" "// comment\nstd::print(1);"
run_test "block_comment" "/* comment */\nstd::print(1);"

# Test 24: Unicode (if supported)
run_test "unicode" "std::print(\"héllo\");"

# Test 25: Mutated versions of valid programs
run_test "mutated_1" "std::print(\"hello\");"  # missing semicolon
run_test "mutated_2" "std::print(\"hello\";);"  # extra paren
run_test "mutated_3" "std::print(\"hello\")"   # missing semicolon, no newline

# Test 26: Binary operations
run_test "add_strings" 'std::print("a" + "b");'
run_test "add_int_string" 'var x = 1 + "hello";'

# Test 27: Boolean operations
run_test "bool_not_int" "var x = !5;"
run_test "bool_and_int" "var x = true && 1;"
run_test "bool_or_int" "var x = false || 0;"

# Test 28: Comparison operations
run_test "compare_strings" 'var x = "a" < "b";'
run_test "compare_mixed" 'var x = 1 < "a";'

# Test 29: Array literals
run_test "array_mixed" "var arr = [1, \"two\", 3];"
run_test "array_nested" "var arr = [[1, 2], [3, 4]];"
run_test "array_multidim" "var arr: [[int]] = [[1, 2], [3, 4]];"

# Test 30: Index expressions
run_test "index_chain" "var arr = [1, 2, 3]; arr[0] + arr[1] + arr[2];"
run_test "index_in_expr" "var x = [1, 2, 3][1] + 1;"

# Test 31: Function calls
run_test "fn_call_chain" "proc add(a: int, b: int): int { return a + b; } std::print(add(add(1, 2), 3));"

# Test 32: Ternary expressions
run_test "ternary_basic" "var x = true ? 1 : 2;"
run_test "ternary_nested" "var x = true ? (false ? 1 : 2) : 3;"

# Test 33: While loop edge cases
run_test "while_true" "while(true) { break; }"
run_test "while_false" "while(false) { std::print(1); }"

# Test 34: For loop edge cases
run_test "for_empty" "for(var i = 0; i < 0; i = i + 1) { std::print(i); }"
run_test "for_negative" "for(var i = 0; i > 10; i = i + 1) { }"

# Summary
echo ""
echo "=== Test Summary ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
echo "Crashes: $CRASH"

if [ $CRASH -gt 0 ]; then
    echo ""
    echo "Crashes detected! Check $TEMP_DIR/crashes/"
    exit 1
fi

if [ $FAIL -gt 0 ]; then
    exit 1
fi

echo ""
echo "All fuzzing tests passed!"
exit 0

# Test 35: Memory allocation edge cases
run_test "large_array" "var arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];"
run_test "long_string" 'std::print("a a a a a a a a a a a a a a a a a a a a a");'

# Test 36: Operator precedence
run_test "op_precedence_1" "var x = 1 + 2 * 3;"
run_test "op_precedence_2" "var x = (1 + 2) * 3;"
run_test "op_precedence_3" "var x = 1 + 2 + 3 + 4;"

# Test 37: String format placeholders
run_test "fmt_int" 'std::print("{}", 42);'
run_test "fmt_multiple" 'std::print("{} and {}", 1, 2);'
run_test "fmt_nested" 'std::print("{} + {} = {}", 1, 2, 1 + 2);'

# Test 38: Boolean expressions
run_test "bool_eq" "var x = (1 == 1);"
run_test "bool_ne" "var x = (1 != 2);"
run_test "bool_complex" "var x = (1 < 2) && (3 > 4) || (5 == 5);"

# Test 39: Chain operations
run_test "chain_assign" "var a = 1; var b = a; var c = b;"
run_test "chain_add" "var x = 1; x = x + 1; x = x + 1;"

# Test 40: Function with many parameters
run_test "many_params" "proc foo(a: int, b: int, c: int, d: int): int { return a + b + c + d; }"

# Test 41: Nested conditionals
run_test "nested_if" "if (true) { if (false) { std::print(1); } }"
run_test "deep_nested_if" "if (true) { if (true) { if (true) { std::print(1); } } }"

# Test 42: Compound assignment
run_test "add_assign" "var x = 1; x += 1;"
run_test "sub_assign" "var x = 5; x -= 3;"
run_test "mul_assign" "var x = 2; x *= 3;"

# Test 43: Post/pre increment
run_test "post_inc" "var x = 1; std::print(x++);"
run_test "pre_inc" "var x = 1; std::print(++x);"

# Test 44: Cast expressions
run_test "cast_int" "var x: int = int(3.14);"

# Test 45: Complex expressions
run_test "complex_1" "std::print(1 + 2 * 3 - 4 / 2);"
run_test "complex_2" "var x = [1, 2, 3][0] + (4 * 5);"
run_test "complex_3" "std::print(true ? 1 : 2);"

# Test 46: Multiple statements
run_test "multi_stmt_1" "var x = 1; var y = 2; std::print(x + y);"
run_test "multi_stmt_2" "std::print(1); std::print(2); std::print(3);"

# Test 47: Function returning function result
run_test "fn_return" "proc add(a: int, b: int): int { return a + b; } var x = add(1, 2);"

# Test 48: Expression statement
run_test "expr_stmt" "1 + 2;"
run_test "expr_stmt_2" "\"hello\";"

# Test 49: Empty blocks
run_test "empty_block" "{}"
run_test "empty_if" "if (true) {} else {}"

# Test 50: Special identifier patterns
run_test "underscore_var" "var _ = 1;"
run_test "underscore_num" "var x_1 = 1;"
run_test "camel_case" "proc myFunction() { }"

