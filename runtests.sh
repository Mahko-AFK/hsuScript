#!/bin/bash
set -euo pipefail

# Build the compiler
./build.sh >/dev/null 2>&1

status=0
for case in tests/cases/*.hsc; do
  name=$(basename "$case" .hsc)
  tmp=$(mktemp)
  ./build/hsc "$case" >"$tmp" 2>&1
  rc=$?
  if [ -f "tests/cases/$name.ast" ]; then
    if ! diff -u "tests/cases/$name.ast" "$tmp"; then
      echo "AST mismatch: $name" >&2
      status=1
    fi
    if [ $rc -ne 0 ]; then
      echo "Unexpected exit code $rc for $name" >&2
      status=1
    fi
  elif [ -f "tests/cases/$name.err" ]; then
    if ! diff -u "tests/cases/$name.err" "$tmp"; then
      echo "Error output mismatch: $name" >&2
      status=1
    fi
    expected_rc=$(cat "tests/cases/$name.exit")
    if [ $rc -ne $expected_rc ]; then
      echo "Exit code mismatch for $name: expected $expected_rc got $rc" >&2
      status=1
    fi
  else
    echo "No expected output for $name" >&2
    status=1
  fi
  rm "$tmp"
done

exit $status
