# cmake/patch_visage.cmake turns a few `static` helpers in visage's headers into `inline` ones: GCC rejects them when
# the headers are used from C++ modules ("exposes TU-local entity"). The script explains the details.
# FetchContent clones main once per build folder and keeps that commit. To move an existing build folder to the latest
# main, delete CMakeFiles/fc-stamp/visage in it and configure again: visage is cloned again and patched.
FetchContent_Declare(
        visage
        GIT_REPOSITORY https://github.com/VitalAudio/visage
        GIT_TAG        main
        PATCH_COMMAND  ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/cmake/patch_visage.cmake"
        SYSTEM
)

FetchContent_MakeAvailable(visage)

add_subdirectory(audio_engine)
add_subdirectory(async_task_scheduler)
add_subdirectory(managers)
add_subdirectory(ui)
add_subdirectory(logger)
add_subdirectory(utils)