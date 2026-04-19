#!/bin/bash
# XVR Fuzzer Test Runner
# Runs pre-existing .xvr test files from corpus directory
#
# SECURITY: This runner only executes pre-defined .xvr files from the corpus.
# No external input is accepted, no C code injection possible.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
XVR_COMPILER="$PROJECT_DIR/build/xvr"
CORPUS_DIR="$SCRIPT_DIR/corpus"
TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

echo "========================================"
echo "XVR Fuzzer Test Suite"
echo "========================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Find all .xvr files in corpus
XVR_FILES=$(find "$CORPUS_DIR" -name "*.xvr" -type f 2>/dev/null | sort)

if [ -z "$XVR_FILES" ]; then
    echo -e "${RED}Error: No .xvr files found in $CORPUS_DIR${NC}"
    exit 1
fi

# Run each test file
for TEST_FILE in $XVR_FILES; do
    TEST_NAME=$(basename "$TEST_FILE" .xvr)
    TEST_COUNT=$((TEST_COUNT + 1))
    
    echo "Testing: $TEST_NAME"
    
    # Run the test and capture output
    if OUTPUT=$($XVR_COMPILER "$TEST_FILE" -r 2>&1); then
        echo -e "  ${GREEN}[PASS]${NC} $TEST_NAME"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "  ${RED}[FAIL]${NC} $TEST_NAME"
        echo "  Error output: $OUTPUT"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
done

echo ""
echo "========================================"
echo "Test Results"
echo "========================================"
echo "Total:  $TEST_COUNT"
echo -e "Passed: ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed: ${RED}$FAIL_COUNT${NC}"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
