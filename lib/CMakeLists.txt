FetchContent_Declare(
        visage
        GIT_REPOSITORY https://github.com/VitalAudio/visage
        GIT_TAG        main
)

FetchContent_MakeAvailable(visage)

add_subdirectory(audio_engine)
add_subdirectory(async_task_scheduler)
add_subdirectory(managers)
add_subdirectory(ui)