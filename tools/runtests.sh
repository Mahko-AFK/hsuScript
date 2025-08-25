#!/usr/bin/env bash
# ultra-minimal runner
set -u
cd "$(dirname "$0")/.."

echo "Building..."
./tools/build.sh || { echo "Build failed"; exit 1; }
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
      printf '\e[32m[PASS]\e[0m %s\n' "$name"; ((passed++))
    else
      printf '\e[31m[FAIL]\e[0m %s\n' "$name"; ((failed++))
      # uncomment to inspect:
      # sed -n '1,120p' "$tmp"
    fi

  else
    printf '\e[31m[FAIL]\e[0m %s (missing .ast oracle)\n' "$name"; ((failed++))
  fi

  ((total++))
  rm -f "$tmp"
done

echo "----------------------------------------------------------------------"
echo "Total: $total   Passed: $passed   Failed: $failed"
[[ $failed -eq 0 ]] || exit 1
