#!/usr/bin/env bash
# ultra-minimal runner
set -u
cd "$(dirname "$0")"

echo "Building..."
./build.sh || { echo "Build failed"; exit 1; }
echo "Build OK."

# Discover tests robustly
mapfile -d '' -t cases < <(find tests/cases -type f -name '*.hsc' -print0 | sort -z)
echo "Discovered ${#cases[@]} test(s):"
for c in "${cases[@]}"; do echo "  $c"; done
(( ${#cases[@]} > 0 )) || { echo "No .hsc tests found."; exit 1; }

strip_trailing() { sed -E 's/[[:space:]]+$//' "$1"; }

total=0
passed=0
failed=0

for case_path in "${cases[@]}"; do
  name="$(basename "$case_path" .hsc)"
  exp_ast="tests/cases/$name.ast"
  exp_err="tests/cases/$name.err"
  exp_exit="tests/cases/$name.exit"
  tmp="$(mktemp)"
  rc=0

  # Prefer --ast-only when an .ast oracle exists; fall back if unsupported
  if [[ -f "$exp_ast" ]]; then
    if ./build/hsc --ast-only "$case_path" >"$tmp" 2>&1; then rc=0; else rc=$?; fi
    # fallback if the binary doesn't know --ast-only
    if grep -qiE 'unknown|invalid|usage' "$tmp"; then
      : >"$tmp"
      if ./build/hsc "$case_path" >"$tmp" 2>&1; then rc=0; else rc=$?; fi
    fi

    if diff -q <(strip_trailing "$exp_ast") <(strip_trailing "$tmp") >/dev/null && [[ $rc -eq 0 ]]; then
      echo "[PASS] $name"; ((passed++))
    else
      echo "[FAIL] $name"; ((failed++))
      # uncomment to inspect:
      # sed -n '1,120p' "$tmp"
    fi

  elif [[ -f "$exp_err" && -f "$exp_exit" ]]; then
    if ./build/hsc "$case_path" >"$tmp" 2>&1; then rc=0; else rc=$?; fi
    expected_rc="$(cat "$exp_exit")"
    if diff -q <(strip_trailing "$exp_err") <(strip_trailing "$tmp") >/dev/null && [[ "$rc" == "$expected_rc" ]]; then
      echo "[PASS] $name"; ((passed++))
    else
      echo "[FAIL] $name"; ((failed++))
      # echo "rc=$rc expected=$expected_rc"
      # sed -n '1,120p' "$tmp"
    fi

  else
    echo "[FAIL] $name (no .ast or .err/.exit oracle)"; ((failed++))
  fi

  ((total++))
  rm -f "$tmp"
done

echo "----------------------------------------------------------------------"
echo "Total: $total   Passed: $passed   Failed: $failed"
[[ $failed -eq 0 ]] || exit 1
