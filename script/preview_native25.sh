#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${STONEAGE_BUILD_DIR:-$ROOT_DIR/build/native25}"
if [[ $# -ne 1 ]]; then echo "Usage: $0 /path/to/stoneage2.5" >&2; exit 2; fi
cmake -S "$ROOT_DIR/native" -B "$BUILD_DIR" -DSTONEAGE_SANITIZERS=ON -DSTONEAGE_SDL_PREVIEW=ON
cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure
exec "$BUILD_DIR/stoneage25-assets" "$1"
