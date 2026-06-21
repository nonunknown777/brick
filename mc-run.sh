#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
META_C="$BUILD_DIR/meta-c"
RUNTIME_DIR="$PROJECT_DIR/runtime"

MC_FILE="$1"
if [ -z "$MC_FILE" ]; then
    echo "Uso: $0 <arquivo.mc>"
    exit 1
fi

BASENAME="$(basename "$MC_FILE" .mc)"
C_FILE="$BUILD_DIR/${BASENAME}.c"
BIN_FILE="$BUILD_DIR/${BASENAME}"

echo "--- $BASENAME ---"

"$META_C" "$MC_FILE" -o "$C_FILE"

gcc -O3 -I"$RUNTIME_DIR" "$C_FILE" \
    "$RUNTIME_DIR/block_memory.c" \
    "$RUNTIME_DIR/hot_reload.c" \
    -o "$BIN_FILE" -ldl

"$BIN_FILE"
