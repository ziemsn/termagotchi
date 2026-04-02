#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
CONFIG="Debug"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG"
cmake --build "$BUILD_DIR" -j
"$BUILD_DIR/termagotchi"
