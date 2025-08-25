#!/usr/bin/env bash
set -euo pipefail

# Ensure we run from repository root
cd "$(dirname "$0")/.."

mkdir -p build

# Build the compiler
./build.sh

# Compile runtime source into build/rt.o
if [ -f rt.c ]; then
  gcc -c rt.c -o build/rt.o
elif [ -f runtime.c ]; then
  gcc -c runtime.c -o build/rt.o
elif [ -f rt.s ]; then
  gcc -c rt.s -o build/rt.o
else
  echo "Runtime source not found" >&2
  exit 1
fi

TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0

for hsc in tests/cases/*.hsc; do
  name=$(basename "$hsc" .hsc)
  out_file="tests/cases/$name.out"
  exit_file="tests/cases/$name.exit"

  TOTAL=$((TOTAL + 1))

  if [ ! -f "$out_file" ]; then
    echo "[SKIP] $name"
    SKIPPED=$((SKIPPED + 1))
    continue
  fi

  # Emit assembly
  ./build/hsc --emit-asm "build/$name.asm" "$hsc"

  # Assemble and link
  gcc -c "build/$name.asm" -o "build/$name.o"
  gcc "build/$name.o" build/rt.o -o "build/$name.bin"

  # Run binary with optional timeout
  if command -v timeout >/dev/null 2>&1; then
    cmd=(timeout 5 "build/$name.bin")
  else
    cmd=("build/$name.bin")
  fi

  output="$(${cmd[@]} 2>&1)"
  status=$?

  # Clean output
  clean_output=$(printf '%s' "$output" | tr -d '\r' | sed 's/[ \t]*$//')
  clean_expected=$(cat "$out_file" | tr -d '\r' | sed 's/[ \t]*$//')

  diff_output=""
  if [ "$clean_output" != "$clean_expected" ]; then
    diff_output=$(diff -u <(printf '%s\n' "$clean_expected") <(printf '%s\n' "$clean_output") || true)
  fi

  expected_exit=0
  if [ -f "$exit_file" ]; then
    expected_exit=$(tr -d '\r\n ' < "$exit_file")
  fi

  exit_diff=""
  if [ "$status" -ne "$expected_exit" ]; then
    exit_diff="Expected exit code $expected_exit, got $status"
  fi

  if [ -z "$diff_output" ] && [ -z "$exit_diff" ]; then
    echo "[PASS] $name"
    PASSED=$((PASSED + 1))
  else
    echo "[FAIL] $name"
    if [ -n "$diff_output" ]; then
      echo "$diff_output"
    fi
    if [ -n "$exit_diff" ]; then
      echo "$exit_diff"
    fi
    FAILED=$((FAILED + 1))
  fi

done

echo "ASM Total: $TOTAL Passed: $PASSED Failed: $FAILED Skipped: $SKIPPED"

if [ "$FAILED" -ne 0 ]; then
  exit 1
fi