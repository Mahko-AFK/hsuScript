#!/usr/bin/env bash
set -euo pipefail

echo "===== Running language tests ====="
./runtests.sh

echo "===== Running assembly checks ====="
./tools/asm_check.sh "$@"
exit $?
