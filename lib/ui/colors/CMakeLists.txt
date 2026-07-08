add_library(colors)

target_custom_compiler_options(colors)

target_sources(colors
        PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES colors_module.cpp
)

target_link_libraries(colors PRIVATE visage)