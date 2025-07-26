include(FetchContent)

set(INSTALL_GTEST OFF)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG        v1.15.2
)

FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_subdirectory(lib)