add_library(managers
        audio_engine_manager.cpp
)

target_sources(managers
        PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES audio_engine_manager_module.cpp
)

target_link_libraries(managers PRIVATE audio-engine async-task-scheduler)