# toolchains/clang-libcxx.cmake
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# vcpkg's default Linux toolchain adds -fPIC, which you lose by chainloading
string(APPEND CMAKE_C_FLAGS_INIT " -fPIC")
string(APPEND CMAKE_CXX_FLAGS_INIT " -fPIC -stdlib=libc++")

string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " -stdlib=libc++")
string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT " -stdlib=libc++")
string(APPEND CMAKE_MODULE_LINKER_FLAGS_INIT " -stdlib=libc++")