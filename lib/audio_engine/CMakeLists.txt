include(FetchContent)

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
set(MINIAUDIO_NO_DECODING ON)
set(MINIAUDIO_NO_ENCODING ON)
set(MINIAUDIO_NO_WAV ON)
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

add_library(audio-engine
        audio_driver.cpp
        audio_format.cpp
        audio_device.cpp
        audio_stream_params.cpp
        audio_library_wrapper.cpp
        miniaudio_library_wrapper.cpp
)

target_sources(audio-engine
        PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES audio_engine_module.cpp
        audio_device_module.cpp
        audio_driver_module.cpp
        audio_format_module.cpp
        audio_stream_params_module.cpp
        audio_buffer_module.cpp
        audio_library_wrapper_module.cpp
        miniaudio_library_wrapper_module.cpp
)

target_link_libraries(audio-engine PRIVATE miniaudio)