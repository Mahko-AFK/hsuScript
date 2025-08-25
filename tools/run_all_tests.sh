#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

echo "===== Running language tests ====="
./tools/runtests.sh

echo "===== Running execution tests ====="
./tools/runexec.sh

if (( "$#" )); then
  echo "===== Running assembly checks ====="
  ./tools/asm_check.sh "$@"
fi
