#!/bin/bash

# Exit on any error
set -e

echo "Compiling simulacion.cpp with OpenCV..."

# Try to determine the OpenCV version
if pkg-config --exists opencv4; then
    echo "Found OpenCV 4.x"
    OPENCV_FLAGS=$(pkg-config --cflags --libs opencv4)
elif pkg-config --exists opencv; then
    echo "Found OpenCV (legacy version)"
    OPENCV_FLAGS=$(pkg-config --cflags --libs opencv)
else
    echo "Error: OpenCV not found. Please install OpenCV."
    exit 1
fi

# Compile the code
g++ -std=c++11 -O3 simulacion.cpp -o matrix_simulator $OPENCV_FLAGS

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful! Executable created: matrix_simulator"
    echo "You can run it with: ./matrix_simulator"
    
    # Make the executable file runnable
    chmod +x matrix_simulator
else
    echo "Compilation failed."
    exit 1
fi
