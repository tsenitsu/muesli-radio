add_library(utils)

target_custom_compiler_options(utils)

target_sources(utils
        PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES utils_module.cpp
        time_utils_module.cpp
        file_utils_module.cpp)