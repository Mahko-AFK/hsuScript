#!/usr/bin/env bash
# build_asan.sh — AddressSanitizer build
set -euo pipefail

# always run from repo root (directory of this script)
cd "$(dirname "$0")/.."

# choose compiler (override with: CC=clang ./build_asan.sh)
CC="${CC:-gcc}"

# common flags
CFLAGS=(
  -Iinclude
  -Wall -Wextra
  -O0 -g
  -fsanitize=address
  -fno-omit-frame-pointer
)
LDFLAGS=(
  -fsanitize=address
)

# sources → objects
SRC=( main.c lexer.c parser.c tools.c sem.c codegen.c )
OBJ=()

# out dir
mkdir -p build

# optional: quick clean
if [[ "${1-}" == "clean" ]]; then
  rm -f build/*.asan.o build/hsc-asan
  echo "Cleaned ASan build artifacts."
  exit 0
fi

# trap to clean partial objects if something fails
cleanup_on_error() {
  echo "Build failed; cleaning partial ASan objects..." >&2
  rm -f build/*.asan.o 2>/dev/null || true
}
trap cleanup_on_error ERR

echo "+ compiler: $CC"
echo "+ compiling with AddressSanitizer..."

# compile
for s in "${SRC[@]}"; do
  if [[ ! -f "$s" ]]; then
    echo "error: missing source file: $s" >&2
    exit 1
  fi
  o="build/${s%.c}.asan.o"
  OBJ+=( "$o" )
  echo "  CC  $s"
  "$CC" "${CFLAGS[@]}" -c "$s" -o "$o"
done

# link
echo "  LD  build/hsc-asan"
"$CC" "${OBJ[@]}" -o build/hsc-asan "${LDFLAGS[@]}"

# success
echo "Built: $(realpath build/hsc-asan 2>/dev/null || echo build/hsc-asan)"
echo
echo "Tip: run with extra checks:"
echo "  ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 ./build/hsc-asan path/to/file.hsc"

