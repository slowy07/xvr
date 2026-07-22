#!/bin/bash
# shellcheck disable=SC2317
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
XVR="${XVR:-$SCRIPT_DIR/../build/xvr}"
STAGE0_DIR="$SCRIPT_DIR/.."

PASSED=0
FAILED=0

echo "=========================================="
echo "Stage0 Test Runner"
echo "=========================================="
echo ""

if [ ! -f "$XVR" ]; then
    echo "Error: XVR compiler not found at $XVR"
    echo "Please build the compiler first: cmake --build build"
    exit 1
fi

echo "Using XVR compiler: $XVR"
echo ""

run_test() {
    local test_name=$1
    local test_file=$2
    local output_bin=$3

    echo "Running: $test_name"
    echo "-----------------------------------"

    if [ ! -f "$STAGE0_DIR/$test_file" ]; then
        echo "FAIL: Test file not found: $test_file"
        FAILED=$((FAILED + 1))
        echo ""
        return
    fi

    if $XVR "$STAGE0_DIR/$test_file" -o "$STAGE0_DIR/$output_bin" 2>&1; then
        if [ -f "$STAGE0_DIR/$output_bin" ]; then
            if "$STAGE0_DIR/$output_bin" 2>/dev/null; then
                echo "PASS: $test_name"
                PASSED=$((PASSED + 1))
            else
                echo "FAIL: $test_name (runtime error, exit $?)"
                FAILED=$((FAILED + 1))
            fi
        else
            echo "PASS: $test_name (compiled, no binary)"
            PASSED=$((PASSED + 1))
        fi
    else
        echo "FAIL: $test_name (compilation error)"
        FAILED=$((FAILED + 1))
    fi

    echo ""
}

echo "=========================================="
echo "Stage0 Lexer Tests"
echo "=========================================="
run_test "Lexer Token Types" "stage0/tests/test_lexer_stage0.xvr" "stage0/tests/test_lexer"

echo "=========================================="
echo "Stage0 Multi-char Operator Tests"
echo "=========================================="
run_test "Multi-char Ops" "stage0/tests/test_lexer_ops.xvr" "stage0/tests/test_lexer_ops"

echo "=========================================="
echo "Stage0 Parser Tests"
echo "=========================================="
run_test "Parser Tests" "stage0/tests/test_parser_stage0.xvr" "stage0/tests/test_parser"

echo "=========================================="
echo "Stage0 Expression Parser Tests"
echo "=========================================="
run_test "Expr Parser" "stage0/tests/test_parser_expr.xvr" "stage0/tests/test_parser_expr"

echo "=========================================="
echo "Stage0 Codegen Tests"
echo "=========================================="
run_test "Codegen Tests" "stage0/tests/test_codegen_stage0.xvr" "stage0/tests/test_codegen"

echo "=========================================="
echo "Stage0 Struct Tests"
echo "=========================================="
run_test "Struct Tests" "stage0/tests/test_struct.xvr" "stage0/tests/test_struct"

echo "=========================================="
echo "Stage0 Integration Tests"
echo "=========================================="
run_test "All Tests" "stage0/tests/test_stage0_all.xvr" "stage0/tests/test_all"

echo "=========================================="
echo "Results: $PASSED passed, $FAILED failed"
echo "=========================================="

exit $FAILED
