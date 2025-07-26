add_executable(
  audio-engine-unit-tests
  audio_driver_tests.cpp
  audio_format_tests.cpp
  audio_device_tests.cpp
  audio_stream_params_tests.cpp
  audio_buffer_tests.cpp
  audio_engine_tests.cpp
)

target_link_libraries(
  audio-engine-unit-tests PRIVATE
  GTest::gtest_main
  GTest::gmock_main
  audio-engine
  miniaudio
)

gtest_discover_tests(audio-engine-unit-tests)