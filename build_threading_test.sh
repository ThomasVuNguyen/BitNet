#!/bin/bash

# Build script for testing BitNet threading on Raspberry Pi 5

echo "Building BitNet threading test for Raspberry Pi 5..."

# Set compiler flags for Pi 5
export CXXFLAGS="-march=armv8.2-a+dotprod+fp16 -mtune=cortex-a76 -O2 -funroll-loops -fprefetch-loop-arrays"
export CFLAGS="-march=armv8.2-a+dotprod+fp16 -mtune=cortex-a76 -O2 -funroll-loops -fprefetch-loop-arrays"

# Create build directory
mkdir -p build_threading
cd build_threading

# Configure with threading support
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DGGML_BITNET_ARM_TL1=ON \
    -DGGML_BITNET_THREADING=ON \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
    -DCMAKE_C_FLAGS="$CFLAGS"

# Build
make -j4

# Build the threading test
echo "Building threading test..."
g++ -std=c++17 \
    -I../include \
    -I../3rdparty/llama.cpp/ggml/include \
    -march=armv8.2-a+dotprod+fp16 \
    -mtune=cortex-a76 \
    -O2 \
    -funroll-loops \
    -fprefetch-loop-arrays \
    -pthread \
    ../test_threading.cpp \
    ../src/bitnet-threading.cpp \
    ../src/bitnet-lut-kernels-threaded.cpp \
    -o test_threading

echo "Build completed! Run './test_threading' to test threading performance."
