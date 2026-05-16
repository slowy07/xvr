#!/bin/bash
set -e

XVR="${XVR:-./build/xvr}"
STAGE0_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== Creating build_self directory ==="
mkdir -p "$STAGE0_DIR/build_self"

echo "=== Concatenating source files ==="
BUILD_SELF="$STAGE0_DIR/build_self/stage0_all.xvr"
: > "$BUILD_SELF"

for f in "$STAGE0_DIR/src/main.xvr" "$STAGE0_DIR/src/token.xvr" "$STAGE0_DIR/src/ast.xvr" "$STAGE0_DIR/src/lexer.xvr" "$STAGE0_DIR/src/parser.xvr" "$STAGE0_DIR/src/codegen.xvr"; do
    filename=$(basename "$f")
    echo "//#source $filename" >> "$BUILD_SELF"
    cat "$f" >> "$BUILD_SELF"
    echo >> "$BUILD_SELF"
done

BYTE_COUNT=$(wc -c < "$BUILD_SELF")
echo "=== Concatenated source: $BYTE_COUNT bytes ==="

echo "=== Building stage0 ==="
$XVR "$STAGE0_DIR/src/main.xvr" -o "$STAGE0_DIR/stage0" || exit 1

echo "=== Stage0 binary created: $STAGE0_DIR/stage0 ==="

if [ ! -f "$STAGE0_DIR/stage0" ]; then
    echo "ERROR: Stage0 binary not found!"
    exit 1
fi

echo "=== Build completed successfully ==="
