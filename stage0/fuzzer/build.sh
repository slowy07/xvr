#!/bin/bash
set -e

FUZZER_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$FUZZER_DIR/.."

CC="${CC:-clang}"
FLAGS="-fsanitize=fuzzer,address -g -O1"

echo "Building stage0 fuzzer..."
$CC $FLAGS fuzzer/fuzz_all.c -o fuzzer/fuzz_all 2>&1

echo "Fuzzer built: fuzzer/fuzz_all"
echo "Run: ./fuzzer/fuzz_all [corpus_dir]"