#!/bin/bash

# Create and enter the build directory
mkdir -p build
cd build

# Run CMake with macOS-specific toolchain file and target triplet
cmake -DCMAKE_TOOLCHAIN_FILE=~/Documents/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=arm64-osx -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=native" ..

# Build the project
if cmake --build . --config Release; then
    echo "Build succeeded."
else
    echo "Build failed."
    read -p "Press [Enter] key to continue..."
    exit 1
fi

# Copy the executable to the parent directory
cp -f main ..

# Return to the original directory
cd ..

# Wait for user input before closing the script
read -p "Press [Enter] key to exit..."