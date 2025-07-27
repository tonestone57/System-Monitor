#!/bin/bash

# Build script for SysMonTask on Haiku

echo "Building SysMonTask for Haiku..."

# Clean previous build
make clean

# Build the application
make

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Executable: SysMonTask"
    echo ""
    echo "To run the application:"
    echo "./SysMonTask"
else
    echo "Build failed!"
    exit 1
fi