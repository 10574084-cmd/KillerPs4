#!/usr/bin/env sh
set -eu

CC="${CC:-cc}"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build/host-tests"

mkdir -p "$BUILD_DIR"
"$CC" -std=c11 -O2 -Wall -Wextra -Werror \
    -I"$ROOT/src" \
    "$ROOT/src/ki1_data.c" "$ROOT/tests/test_ki1_data.c" \
    -o "$BUILD_DIR/test_ki1_data"

"$BUILD_DIR/test_ki1_data"
