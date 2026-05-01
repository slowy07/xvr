#!/bin/bash
set -e

XVR="${XVR:-./build/xvr}"
STAGE0_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "Building stage0..."
$XVR "$STAGE0_DIR/src/main.xvr" -o "$STAGE0_DIR/stage0" || exit 1

echo "Stage0 binary created: $STAGE0_DIR/stage0"
