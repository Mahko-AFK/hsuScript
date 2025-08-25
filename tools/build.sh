#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

mkdir -p build

set -x
gcc -Iinclude -Wall -Wextra -c runtime/rt.c -o build/rt.o
ld -r -b binary build/rt.o -o build/rt_blob.o
objcopy --redefine-sym _binary_build_rt_o_start=rt_o_start \
        --redefine-sym _binary_build_rt_o_end=rt_o_end \
        --redefine-sym _binary_build_rt_o_size=rt_o_size \
        build/rt_blob.o build/rt_embed.o
gcc -Iinclude \
  -Wall -Wextra \
  main.c lexer.c parser.c tools.c sem.c codegen.c build/rt_embed.o \
  -o build/hsc
set +x

echo "Built: $(realpath build/hsc)"

