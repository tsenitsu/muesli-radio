add_library(logger
        log_entry.cpp
        console_logger.cpp
        file_logger.cpp
)

target_custom_compiler_options(logger)

target_sources(logger
        PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES log_level_module.cpp
        log_entry_module.cpp
        logger_module.cpp
        console_logger_module.cpp
        file_logger_module.cpp)

target_link_libraries(logger PRIVATE utils)