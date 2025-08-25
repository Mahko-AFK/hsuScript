#!/usr/bin/env bash
# Compile sources to assembly, link with runtime, and verify program output.
set -euo pipefail
cd "$(dirname "$0")/.."

echo "Building..."
if ! ./tools/build.sh > /dev/null; then
  echo "Build failed" >&2
  exit 1
fi

BUILD_DIR=build
RT_OBJ="$BUILD_DIR/rt_tmp.o"
mkdir -p "$BUILD_DIR"

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

  rel="${case_path#tests/exec/}"
  safe="${rel//\//_}"
  safe="${safe%.hsc}"
  name="$rel"

  asm="$BUILD_DIR/$safe.s"
  obj="$BUILD_DIR/$safe.o"
  exe="$BUILD_DIR/$safe"

  if [[ -f "$exp_out" && -f "$exp_exit" ]]; then
    # Emit assembly
    if ! ./build/hsc --emit-asm "$asm" "$case_path" >/dev/null 2>&1; then
      printf '\e[31m[FAIL]\e[0m %s (emit)\n' "$name"
      failed=$((failed+1)); total=$((total+1)); continue
    fi

    # Assemble (silence executable-stack warnings)
    # (You should ALSO add `.section .note.GNU-stack,"",@progbits` in emitted .s)
    if ! gcc -Wa,--noexecstack -c "$asm" -o "$obj"; then
      printf '\e[31m[FAIL]\e[0m %s (assemble)\n' "$name"
      failed=$((failed+1)); total=$((total+1)); continue
    fi

    # Link
    if ! gcc "$obj" "$RT_OBJ" -o "$exe"; then
      printf '\e[31m[FAIL]\e[0m %s (link)\n' "$name"
      failed=$((failed+1)); total=$((total+1)); continue
    fi

    # Run and capture RAW stdout + stderr (no normalization)
    out_tmp="$(mktemp)"
    err_tmp="$(mktemp)"
    if "$exe" >"$out_tmp" 2>"$err_tmp"; then
      rc=0
    else
      rc=$?
    fi

    expected_rc="$(cat "$exp_exit")"

    # Byte-for-byte comparison
    if cmp -s "$exp_out" "$out_tmp" && [[ "$rc" == "$expected_rc" ]]; then
      printf '\e[32m[PASS]\e[0m %s\n' "$name"
      passed=$((passed+1))
    else
      printf '\e[31m[FAIL]\e[0m %s\n' "$name"
      echo "Expected exit: $expected_rc, Got: $rc"
      echo "---- diff (expected vs actual stdout; raw) ----"
      diff -u "$exp_out" "$out_tmp" || true
      if [[ -s "$err_tmp" ]]; then
        echo "---------------- stderr ----------------"
        # Show only first ~200 lines to avoid spam
        sed -n '1,200p' "$err_tmp"
      fi
      echo "----------------------------------------"
      failed=$((failed+1))
    fi

    rm -f "$out_tmp" "$err_tmp"
    total=$((total+1))
  fi
done

echo "----------------------------------------------------------------------"
echo "Total: $total   Passed: $passed   Failed: $failed"
[[ $failed -eq 0 ]] || exit 1