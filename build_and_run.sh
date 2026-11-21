#!/bin/bash
# AllegroScore Build & Run Script
# Works on any Linux with Qt 6 installed

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "=== AllegroScore Build & Run ==="

# Create fresh build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "Running cmake..."
cd "$BUILD_DIR"
cmake "$PROJECT_DIR"

echo "Building..."
cmake --build "$BUILD_DIR" -j$(nproc)

echo ""
echo "=== Running AllegroScore (offscreen mode) ==="
echo "Close the window or press Ctrl+C to exit"
echo ""

# Run with offscreen platform (no display/GUI required)
"$BUILD_DIR/AllegroScore" -platform offscreen