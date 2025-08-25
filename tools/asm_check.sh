#!/usr/bin/env bash
# Compile assembly snippets and link with the runtime.
set -euo pipefail

# Run from repository root
cd "$(dirname "$0")/.."

BUILD_DIR=build
RT_SRC=runtime/rt.c
RT_OBJ="$BUILD_DIR/rt.o"

mkdir -p "$BUILD_DIR"

# Compile runtime exactly once per script invocation
echo "  CC  $RT_SRC"
gcc -Iinclude -Wall -Wextra -c "$RT_SRC" -o "$RT_OBJ"

# Optionally compile and link any assembly files provided as arguments
for asm in "$@"; do
  base="$(basename "$asm")"
  obj="$BUILD_DIR/${base%.s}.o"
  exe="$BUILD_DIR/${base%.s}"
  echo "  AS  $asm"
  gcc -c "$asm" -o "$obj"
  echo "  LD  $exe"
  gcc "$obj" "$RT_OBJ" -o "$exe"
done