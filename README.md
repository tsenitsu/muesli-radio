# Muesli Radio
A work in progress audio project whose primary purpose is experimenting with the newest C++ features.

## Building
Building with CMake for Windows:
`cmake -DCMAKE_BUILD_TYPE=Debug -Wno-dev -DCMAKE_TOOLCHAIN_FILE=<PATH TO vcpkg.cmake FILE> -DVCPKG_TARGET_TRIPLET=x64-windows-static -G Ninja -B .\build\debug`