#!/usr/bin/env bash
# Compile assembly snippets and link with the runtime.
set -euo pipefail

# Run from repository root
cd "$(dirname "$0")/.."

BUILD_DIR=build
RT_SRC=runtime/rt.c
RT_OBJ="$BUILD_DIR/rt.o"

mkdir -p "$BUILD_DIR"

# Compile runtime if needed
if [[ ! -f "$RT_SRC" ]]; then
  echo "missing runtime source: $RT_SRC" >&2
  exit 1
fi
if [[ ! -f "$RT_OBJ" || "$RT_SRC" -nt "$RT_OBJ" ]]; then
  echo "  CC  $RT_SRC"
  gcc -Iinclude -Wall -Wextra -c "$RT_SRC" -o "$RT_OBJ"
fi

GREEN='\e[32m'
RED='\e[31m'
RESET='\e[0m'
status=0

# Optionally compile, link, and run any assembly files provided as arguments
for asm in "$@"; do
  base="$(basename "$asm")"
  obj="$BUILD_DIR/${base%.s}.o"
  exe="$BUILD_DIR/${base%.s}"

  if [[ ! -f "$asm" ]]; then
    printf '%b %s\n' "${RED}[FAIL]${RESET}" "$asm (missing file)"
    status=1
    continue
  fi

  echo "  AS  $asm"
  if ! gcc -c "$asm" -o "$obj"; then
    printf '%b %s\n' "${RED}[FAIL]${RESET}" "$asm (assemble)"
    status=1
    continue
  fi

  echo "  LD  $exe"
  if ! gcc "$obj" "$RT_OBJ" -o "$exe"; then
    printf '%b %s\n' "${RED}[FAIL]${RESET}" "$asm (link)"
    status=1
    continue
  fi

  if "$exe" >/dev/null 2>&1; then
    printf '%b %s\n' "${GREEN}[PASS]${RESET}" "$asm"
  else
    printf '%b %s\n' "${RED}[FAIL]${RESET}" "$asm (run)"
    status=1
  fi
done

exit $status

