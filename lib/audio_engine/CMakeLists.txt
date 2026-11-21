set(MINIAUDIO_BUILD_EXAMPLES OFF)
set(MINIAUDIO_BUILD_TESTS OFF)
set(MINIAUDIO_NO_EXTRA_NODES ON)
set(MINIAUDIO_NO_LIBVORBIS ON)
set(MINIAUDIO_NO_LIBOPUS ON)
set(MINIAUDIO_NO_SNDIO ON)
set(MINIAUDIO_NO_PIPEWIRE ON)
set(MINIAUDIO_NO_AUDIO4 ON)
set(MINIAUDIO_NO_OSS ON)
set(MINIAUDIO_NO_AAUDIO ON)
set(MINIAUDIO_NO_OPENSL ON)
set(MINIAUDIO_NO_WEBAUDIO ON)
set(MINIAUDIO_NO_DREAMCAST ON)
set(MINIAUDIO_NO_XAUDIO ON)
set(MINIAUDIO_NO_VITA ON)
set(MINIAUDIO_NO_SDL2 ON)
set(MINIAUDIO_NO_CUSTOM ON)
set(MINIAUDIO_NO_FLAC ON)
set(MINIAUDIO_NO_MP3 ON)
set(MINIAUDIO_NO_RESOURCE_MANAGER ON)
set(MINIAUDIO_NO_NODE_GRAPH ON)
set(MINIAUDIO_NO_ENGINE ON)
set(MINIAUDIO_NO_GENERATION ON)

if (MSVC)
    set(MINIAUDIO_NO_PULSEAUDIO ON)
endif()

FetchContent_Declare(
        miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio
        GIT_TAG        dev-0.12
        SYSTEM
)

FetchContent_MakeAvailable(miniaudio)

if (WIN32)
    # RtAudio only provides the ASIO driver: every other Windows driver comes from miniaudio.
    # JACK and PulseAudio are disabled too because RtAudio looks for them on every OS.
    set(RTAUDIO_API_ASIO ON)
    set(RTAUDIO_API_WASAPI OFF)
    set(RTAUDIO_API_DS OFF)
    set(RTAUDIO_API_JACK OFF)
    set(RTAUDIO_API_PULSE OFF)
    set(RTAUDIO_BUILD_STATIC_LIBS ON)
    set(RTAUDIO_BUILD_TESTING OFF)
    set(RTAUDIO_INSTALL OFF)

    # Pinned to a master commit because cmake/patch_rtaudio.cmake is written against it. The patch fixes a heap corruption
    # with 24-bit ASIO drivers (e.g. Realtek ASIO): since RtAudio commit f0899de, RtApiAsio copies 4 bytes per sample into
    # driver buffers that hold 3 bytes per sample. The script explains the details.
    # When moving to a newer commit, check whether the patch is still needed and delete _deps/rtaudio-* in the build
    # folders: the patched files are local changes, which FetchContent would try to carry over to the new commit.
    FetchContent_Declare(
            rtaudio
            GIT_REPOSITORY https://github.com/thestk/rtaudio
            GIT_TAG        8ce4f2635a3b8f52687ec34625c8027e18024fe9 # master, 27 Oct 2026
            PATCH_COMMAND  ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/cmake/patch_rtaudio.cmake"
            SYSTEM
    )

    # RtAudio requires CMake 3.13, which leaves policy CMP0091 unset in its directory: CMAKE_MSVC_RUNTIME_LIBRARY
    # would be ignored there and its Debug build would link the release runtime (LNK2038 against /MTd)
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

    FetchContent_MakeAvailable(rtaudio)
endif()

add_library(audio-engine
        audio_engine.cpp
        audio_driver.cpp
        audio_device.cpp
        audio_stream_params.cpp
        audio_library_wrapper.cpp
        miniaudio_library_wrapper.cpp
        channel_routing.cpp
        audio_recorder.cpp
)

target_custom_compiler_options(audio-engine)

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
        mixer_channel_module.cpp
        channel_routing_module.cpp
        audio_mixer_module.cpp
        ring_audio_buffer_module.cpp
        audio_writer_module.cpp
        audio_recorder_module.cpp
)

target_link_libraries(audio-engine PRIVATE miniaudio logger utils)

if (WIN32)
    target_sources(audio-engine PRIVATE rtaudio_library_wrapper.cpp)

    target_sources(audio-engine
            PUBLIC
            FILE_SET cxx_modules
            TYPE CXX_MODULES
            FILES rtaudio_library_wrapper_module.cpp
    )

    target_link_libraries(audio-engine PRIVATE rtaudio)
endif()