#!/bin/bash
# Build script for DraStic RetroArch core

set -e

echo "=== Building DraStic RetroArch Core ==="
echo ""

# Check for cross compiler
if ! command -v aarch64-linux-gnu-g++ &> /dev/null; then
    echo "Error: aarch64-linux-gnu-g++ not found"
    echo "Please install aarch64 cross-compilation toolchain"
    exit 1
fi

# Build
make clean
make

if [ -f "drastic_libretro.so" ]; then
    echo ""
    echo "✅ Build successful!"
    echo "Core file: drastic_libretro.so"
    echo ""
    echo "To install, copy to RetroArch cores directory:"
    echo "  cp drastic_libretro.so ~/.config/retroarch/cores/"
else
    echo "❌ Build failed"
    exit 1
fi
