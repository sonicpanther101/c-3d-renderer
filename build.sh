#!/bin/bash

mkdir -p build
cd build || exit

# Run CMake with export compile commands option for clangd
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

make
./C++-renderer
