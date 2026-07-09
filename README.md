# Muesli Radio
A work in progress audio project whose primary purpose is experimenting with the newest C++ features.

## Building
Building with CMake for Windows and Linux (GCC):

`cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -Wno-dev -B .\build\debug`

Building with CMake for Linux (Clang):

`cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -Wno-dev -DCMAKE_CXX_COMPILER=/usr/bin/clang++-20 -DCMAKE_C_COMPILER=/usr/bin/clang-20 -DCMAKE_CXX_FLAGS=-stdlib=libc++`