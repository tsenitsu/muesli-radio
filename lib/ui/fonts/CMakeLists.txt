add_library(fonts)

target_custom_compiler_options(fonts)

target_sources(fonts
        PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES fonts_module.cpp
)

target_link_libraries(fonts PRIVATE visage)