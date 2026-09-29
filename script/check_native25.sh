#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${STONEAGE_BUILD_DIR:-$ROOT_DIR/build/native25}"
cmake -S "$ROOT_DIR/native" -B "$BUILD_DIR" -DSTONEAGE_SANITIZERS=ON
cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure
if [[ $# -gt 0 ]]; then
  exec "$BUILD_DIR/stoneage25-probe" "$@"
fi
