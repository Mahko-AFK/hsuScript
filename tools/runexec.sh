#!/usr/bin/env bash
# Compile sources to assembly, link with runtime, and verify program output.
set -euo pipefail
cd "$(dirname "$0")/.."

# Build compiler
echo "Building..."
if ! ./tools/build.sh > /dev/null; then
  echo "Build failed" >&2
  exit 1
fi

# Compile runtime once
BUILD_DIR=build
RT_SRC=runtime/rt.c
RT_OBJ="$BUILD_DIR/rt.o"
mkdir -p "$BUILD_DIR"

echo "Compiling runtime..."
if ! gcc -Iinclude -Wall -Wextra -c "$RT_SRC" -o "$RT_OBJ"; then
  echo "Failed to compile runtime" >&2
  exit 1
fi

strip_trailing() { sed -E 's/[[:space:]]+$//' "$1"; }

# Discover all .hsc test cases under tests/exec
mapfile -d '' -t cases < <(find tests/exec -type f -name '*.hsc' -print0 | sort -z)
echo "Discovered ${#cases[@]} case(s):"
for c in "${cases[@]}"; do echo "  $c"; done
(( ${#cases[@]} > 0 )) || { echo "No .hsc tests found."; exit 1; }

# Ensure all oracle files exist before running anything
for case_path in "${cases[@]}"; do
  dir="$(dirname "$case_path")"
  base="$(basename "$case_path" .hsc)"
  [[ -f "$dir/$base.out" && -f "$dir/$base.exit" ]] || {
    echo "Missing oracle for $case_path" >&2
    exit 1
  }
done

passed=0
failed=0
total=0

for case_path in "${cases[@]}"; do
  dir="$(dirname "$case_path")"
  base="$(basename "$case_path" .hsc)"
  exp_out="$dir/$base.out"
  exp_exit="$dir/$base.exit"

  # Use a path-derived name to avoid collisions between directories
  rel="${case_path#tests/exec/}"
  safe="${rel//\//_}"
  safe="${safe%.hsc}"
  name="$rel"

  asm="$BUILD_DIR/$safe.s"
  obj="$BUILD_DIR/$safe.o"
  exe="$BUILD_DIR/$safe"

  # Only run execution tests when both .out and .exit oracles exist
  if [[ -f "$exp_out" && -f "$exp_exit" ]]; then
    if ! ./build/hsc --emit-asm "$asm" "$case_path" >/dev/null 2>&1; then
      printf '\e[31m[FAIL]\e[0m %s (emit)\n' "$name"
      failed=$((failed+1))
      total=$((total+1))
      continue
    fi

    if ! gcc -c "$asm" -o "$obj"; then
      printf '\e[31m[FAIL]\e[0m %s (assemble)\n' "$name"
      failed=$((failed+1))
      total=$((total+1))
      continue
    fi

    if ! gcc "$obj" "$RT_OBJ" -o "$exe"; then
      printf '\e[31m[FAIL]\e[0m %s (link)\n' "$name"
      failed=$((failed+1))
      total=$((total+1))
      continue
    fi

    tmp="$(mktemp)"
    if "$exe" >"$tmp"; then
      rc=0
    else
      rc=$?
    fi

    expected_rc="$(cat "$exp_exit")"
    if diff -q <(strip_trailing "$exp_out") <(strip_trailing "$tmp") >/dev/null && [[ "$rc" == "$expected_rc" ]]; then
      printf '\e[32m[PASS]\e[0m %s\n' "$name"
      passed=$((passed+1))
    else
      printf '\e[31m[FAIL]\e[0m %s\n' "$name"
      failed=$((failed+1))
    fi
    rm -f "$tmp"
    total=$((total+1))
  fi

done

echo "----------------------------------------------------------------------"
echo "Total: $total   Passed: $passed   Failed: $failed"
[[ $failed -eq 0 ]] || exit 1
