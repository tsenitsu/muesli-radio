# Muesli Radio
A multitrack input and output audio recorder written in C++.

## Building
You need ninja on both Windows and Linux and CMake version 4.2.2. On Windows, no additional libraries
are required. On Linux, you need to install graphics libraries (list to be updated).

Building with CMake for Windows and Linux (GCC):

`cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -Wno-dev -B .\build\debug && ninja`

Building with CMake for Linux (Clang):

`cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -Wno-dev -DCMAKE_CXX_COMPILER=/usr/bin/clang++-20 -DCMAKE_C_COMPILER=/usr/bin/clang-20 -DCMAKE_CXX_FLAGS=-stdlib=libc++ && ninja`