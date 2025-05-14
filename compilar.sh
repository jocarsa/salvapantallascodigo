#!/usr/bin/env bash
set -e

# Try to locate OpenCV 4
PKG_CFLAGS=$(pkg-config --cflags opencv4)
PKG_LDFLAGS=$(pkg-config --libs   opencv4)

if [ -z "$PKG_CFLAGS$PKG_LDFLAGS" ]; then
  echo "Error: pkg-config could not find opencv4."
  echo "Make sure libopencv-dev is installed and PKG_CONFIG_PATH is correct."
  exit 1
fi

# Compile simulacion.cpp with OpenMP and OpenCV
g++ $PKG_CFLAGS -std=c++17 -fopenmp simulacion.cpp -o simulacion $PKG_LDFLAGS

echo "Build complete: ./simulacion"
