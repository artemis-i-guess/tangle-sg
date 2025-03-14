#!/bin/bash

echo "Cleaning old build files..."
make clean

echo "Compiling the project..."
make

if [ $? -eq 0 ]; then
    echo "Compilation successful! Running the program..."
    ./tangle_poc
else
    echo "Compilation failed! Fix the errors and try again."
fi
