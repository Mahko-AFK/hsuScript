#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

mkdir -p build

set -x
gcc -Iinclude \
  -Wall -Wextra \
  main.c lexer.c parser.c tools.c sem.c codegen.c \
  -o build/hsc
set +x

echo "Built: $(realpath build/hsc)"

