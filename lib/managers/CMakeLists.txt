add_library(managers
        audio_engine_manager.cpp
        ui_manager.cpp
        logger_manager.cpp
)

target_custom_compiler_options(managers)

target_sources(managers
        PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES audio_engine_manager_module.cpp
        ui_manager_module.cpp
        logger_manager_module.cpp
)

target_link_libraries(managers PRIVATE audio-engine async-task-scheduler components logger)