# Muesli Radio
A multitrack input and output audio recorder written in C++.

## Building
You need Ninja and CMake 4.3.1 on both Windows and Linux. On Windows no additional libraries are required.

### Linux dependencies

RHEL (some `-devel` packages need the CRB repo enabled):
```bash
sudo dnf install libX11-devel mesa-libGL-devel mesa-libEGL-devel \
  libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libXext-devel \
  libxkbcommon-devel wayland-devel libcxx-devel libcxxabi-devel \
  autoconf autoconf-archive automake libtool perl make pkgconf-pkg-config
```

Debian/Ubuntu:
```bash
sudo apt install libx11-dev libgl-dev libegl-dev \
  libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev \
  libxkbcommon-dev libwayland-dev wayland-protocols libc++-21-dev libc++abi-21-dev \
  autoconf autoconf-archive automake libtool perl make pkg-config python3-venv
```

### Windows
```
cmake -DCMAKE_TOOLCHAIN_FILE=<path to vcpkg.cmake> -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_BUILD_TYPE=Debug -G Ninja -Wno-dev -B .\build\debug
cmake --build build\debug
```

### Linux (GCC 16):

```
cmake -DCMAKE_TOOLCHAIN_FILE=<path to vcpkg.cmake> -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_BUILD_TYPE=Debug -G Ninja -Wno-dev -B ./build/debug
cmake --build build/debug
```

### Linux (Clang 22):

```
cmake -DCMAKE_TOOLCHAIN_FILE=<path to vcpkg.cmake> -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_BUILD_TYPE=Debug -G Ninja -Wno-dev -DCMAKE_CXX_COMPILER=/usr/bin/clang++-22 -DCMAKE_C_COMPILER=/usr/bin/clang-22 -DCMAKE_CXX_FLAGS=-stdlib=libc++ -B ./build/debug
cmake --build build/debug
```