add_library(audio-engine
        audio_driver.cpp
        audio_device.cpp
        audio_stream_params.cpp
        audio_library_wrapper.cpp
        miniaudio_library_wrapper.cpp
        channel_routing.cpp
        audio_recorder.cpp
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
        mixer_channel_module.cpp
        channel_routing_module.cpp
        audio_mixer_module.cpp
        ring_audio_buffer_module.cpp
        audio_writer_module.cpp
        audio_recorder_module.cpp
)

target_link_libraries(audio-engine PRIVATE miniaudio)