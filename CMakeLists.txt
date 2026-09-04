cmake_minimum_required(VERSION 4.3.1 FATAL_ERROR)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
set(CMAKE_CXX_MODULE_STD 1)

project(muesli-radio LANGUAGES CXX)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
if (NOT MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_EXTENSIONS ON)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(functions)
include(FetchContent)

option(ENABLE_ASAN "Enable Address sanitizer" OFF)
option(ENABLE_TSAN "Enable Thread sanitizer" OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "Address sanitizer and Thread sanitizer can not be enabled at the same time")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (MSVC)
        if (ENABLE_ASAN)
            set(VISAGE_ADDRESS_SANITIZER ON)

            add_compile_options(/fsanitize=address)
            add_link_options(/INCREMENTAL:NO)

            # This avoids annotation mismatch
            # CMake creates a hidden target called __cmake_cxx23 (you can see __cmake_cxx23.lib in your link line).
            # You can inject the ASan flags directly into that target after it comes into existence – i.e.,
            # after you’ve added the first library that uses import std;.
            if(TARGET __cmake_cxx23)
                target_compile_options(__cmake_cxx23 PRIVATE /fsanitize=address)
                target_link_options(__cmake_cxx23 PRIVATE /INCREMENTAL:NO)
            endif()
        endif()
        if (ENABLE_TSAN)
            message(FATAL_ERROR "Thread sanitizer is not supported by MSVC")
        endif()
    else()
        if (ENABLE_ASAN)
            set(VISAGE_ADDRESS_SANITIZER ON)

            add_compile_options(-fsanitize=address,undefined,leak)
            add_link_options(-fsanitize=address,undefined,leak)
            if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                add_compile_options(-D_GLIBCXX_SANITIZE_VECTOR)
            endif()
        endif()
        if (ENABLE_TSAN)
            add_compile_options(-fsanitize=thread)
            add_link_options(-fsanitize=thread)
        endif()
    endif()
endif()

add_subdirectory(lib)

add_executable(muesli-radio main.cpp)
target_custom_compiler_options(muesli-radio)
target_link_libraries(muesli-radio PRIVATE audio-engine async-task-scheduler managers logger)

enable_testing()
add_subdirectory(testing)
