#!/bin/bash

# Define the source file and output executable name
SOURCE_FILE="simulacion.cpp"
OUTPUT_EXECUTABLE="simulacion"

# Compile the code using g++
g++ -std=c++11 $SOURCE_FILE -o $OUTPUT_EXECUTABLE \
    `pkg-config --cflags --libs opencv4` \
    -O2

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Executable created: $OUTPUT_EXECUTABLE"
else
    echo "Compilation failed."
fi

