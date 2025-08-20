cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
set(CMAKE_CXX_MODULE_STD 1)

project(muesli-radio LANGUAGES CXX)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

if(MSVC)
   add_compile_options(/W4 /WX)
else()
    add_compile_options(-Werror -Wall -Wextra -pedantic -Wpessimizing-move -Wredundant-move -Wshadow)
    if(GNU)
        add_compile_options(-Wnrvo)
    endif()
endif()

add_subdirectory(lib)

enable_testing()
add_subdirectory(testing)

add_executable(muesli-radio main.cpp)
target_link_libraries(muesli-radio PRIVATE audio-engine)

