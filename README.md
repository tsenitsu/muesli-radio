# Muesli Radio
A multitrack input and output audio recorder written in C++.

## Building
You need Ninja and CMake 4.5.0 on both Windows and Linux. On Windows no additional libraries are required.

### Linux dependencies

RHEL (some `-devel` packages need the CRB repo enabled):
```bash
sudo dnf install libX11-devel libxcb-devel mesa-libGL-devel mesa-libEGL-devel \
  libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libXext-devel \
  libxkbcommon-devel wayland-devel fontconfig-devel libcxx-devel libcxxabi-devel \
  autoconf autoconf-archive automake libtool perl make pkgconf-pkg-config
```

Debian/Ubuntu (including WSL). Set `LLVM_VERSION` to the major version of Clang/libc++ you have installed:
```bash
LLVM_VERSION=21
sudo apt install libx11-dev libxcb1-dev libxcb-present-dev libgl-dev libegl-dev \
  libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev \
  libxkbcommon-dev libwayland-dev wayland-protocols libfontconfig-dev \
  libc++-${LLVM_VERSION}-dev libc++abi-${LLVM_VERSION}-dev \
  autoconf autoconf-archive automake libtool perl make pkg-config python3-venv
```

### Windows
Tested with Visual Studio.
```
cmake -DCMAKE_TOOLCHAIN_FILE=<path to vcpkg.cmake> -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_BUILD_TYPE=Debug -G Ninja -B .\build\debug
cmake --build build\debug
```

### Linux (GCC)
GCC 16 or newer is required.
```
cmake -DCMAKE_TOOLCHAIN_FILE=<path to vcpkg.cmake> -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_BUILD_TYPE=Debug -G Ninja -B ./build/debug
cmake --build build/debug
```

### Linux (Clang)
Tested with Clang 22 on RHEL and Clang 21 on Ubuntu (WSL).

Clang builds use libc++, so vcpkg must build the dependencies against it too. This is what the `x64-linux-libcxx` triplet (provided in `vcpkg/triplets`) is for. Run the commands from the repository root:

```
cmake -DCMAKE_TOOLCHAIN_FILE=<path to vcpkg.cmake> -DVCPKG_TARGET_TRIPLET=x64-linux-libcxx -DVCPKG_OVERLAY_TRIPLETS="$PWD/vcpkg/triplets" -DCMAKE_BUILD_TYPE=Debug -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_FLAGS=-stdlib=libc++ -B ./build/debug
cmake --build build/debug
```

On Ubuntu (WSL), CMake also needs to be told where libc++'s module manifest is, otherwise `import std` fails. Add this to the configure command, using the same `LLVM_VERSION` as above:

```
-DCMAKE_CXX_STDLIB_MODULES_JSON=/usr/lib/llvm-${LLVM_VERSION}/lib/libc++.modules.json
```