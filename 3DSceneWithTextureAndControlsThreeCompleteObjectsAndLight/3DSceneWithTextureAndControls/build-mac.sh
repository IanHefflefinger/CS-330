#!/usr/bin/env bash
# Builds (and optionally runs) the 3D scene on macOS.
# Usage: ./build-mac.sh [--run]
set -euo pipefail
cd "$(dirname "$0")"

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required. Install it from https://brew.sh and re-run this script."
  exit 1
fi

for pkg in cmake glfw glew glm; do
  brew list "$pkg" >/dev/null 2>&1 || brew install "$pkg"
done

if [ ! -d "../resources/textures" ]; then
  echo "WARNING: ../resources/textures not found. The scene needs its texture PNGs there."
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build --config Release

echo "Build complete. Run it from this folder with: ./scene"
if [ "${1:-}" = "--run" ]; then
  ./scene
fi
