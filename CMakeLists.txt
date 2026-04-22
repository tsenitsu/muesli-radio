cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_MODULE_STD 1)

project(muesli-radio LANGUAGES CXX)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
if (NOT MSVC AND NOT GNU)
    set(CMAKE_CXX_EXTENSIONS ON)
endif()

option(ENABLE_ASAN "Enable Address sanitizer" OFF)
option(ENABLE_TSAN "Enable Thread sanitizer" OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "Address sanitizer and Thread sanitizer can not be enabled at the same time")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (MSVC)
        if (ENABLE_ASAN)
            add_compile_options(/fsanitize=address)
            add_link_options(/INCREMENTAL:NO)
        endif()
        if (ENABLE_TSAN)
            message(FATAL_ERROR "Thread sanitizer is not supported by MSVC")
        endif()
    else()
        if (ENABLE_ASAN)
            add_compile_options(-fsanitize=address,undefined,leak)
            add_link_options(-fsanitize=address,undefined,leak)
            if (GNU)
                add_compile_options(-D_GLIBCXX_SANITIZE_VECTOR)
            endif()
        endif()
        if (ENABLE_TSAN)
            add_compile_options(-fsanitize=thread)
            add_link_options(-fsanitize=thread)
        endif()
    endif()
endif()

# Fetch all libraries
include(FetchContent)

# Miniaudio
set(MINIAUDIO_BUILD_EXAMPLES OFF)
set(MINIAUDIO_BUILD_TESTS OFF)
set(MINIAUDIO_NO_EXTRA_NODES ON)
set(MINIAUDIO_NO_LIBVORBIS ON)
set(MINIAUDIO_NO_LIBOPUS ON)
set(MINIAUDIO_NO_SNDIO ON)
set(MINIAUDIO_NO_AUDIO4 ON)
set(MINIAUDIO_NO_OSS ON)
set(MINIAUDIO_NO_AAUDIO ON)
set(MINIAUDIO_NO_OPENSL ON)
set(MINIAUDIO_NO_WEBAUDIO ON)
set(MINIAUDIO_NO_CUSTOM ON)
set(MINIAUDIO_NO_FLAC ON)
set(MINIAUDIO_NO_MP3 ON)
set(MINIAUDIO_NO_RESOURCE_MANAGER ON)
set(MINIAUDIO_NO_NODE_GRAPH ON)
set(MINIAUDIO_NO_ENGINE ON)
set(MINIAUDIO_NO_GENERATION ON)

FetchContent_Declare(
        miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio
        GIT_TAG        master
)

FetchContent_MakeAvailable(miniaudio)

# GTest
set(INSTALL_GTEST OFF)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest
        GIT_TAG        v1.15.2
)

FetchContent_MakeAvailable(googletest)

if(MSVC)
   add_compile_options(/W4 /WX /w14242 /w14254 /w14287)
else()
    add_compile_options(-Werror -Wall -Wextra -pedantic -Wshadow -Wconversion -Wsign-conversion)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wpessimizing-move;-Wredundant-move>")

    if(GNU)
        add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wnrvo>")
    endif()
endif()

add_subdirectory(lib)

add_executable(muesli-radio main.cpp)
target_link_libraries(muesli-radio PRIVATE audio-engine async-task-scheduler managers)

enable_testing()
add_subdirectory(testing)
