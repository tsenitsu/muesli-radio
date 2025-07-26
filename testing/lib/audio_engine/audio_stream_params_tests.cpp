#include <gtest/gtest.h>

import std;
import audio_stream_params;
import audio_device;

using namespace audio_engine;

TEST(AudioStreamParams, createInputAudioStreamParams) {
    constexpr audio_device::SampleRate_t sampleRate { 44100 };
    constexpr auto format { audio_format::AudioFormat::Float32 };
    constexpr audio_stream_params::BufferLength_t bufferLength { 2048 };
    constexpr audio_stream_params::PeriodSize_t periodSize { 3 };
    const audio_device::DeviceId inputDeviceId { 2 };
    constexpr audio_device::ChannelCount_t numberOfInputChannels { 3 };

    const auto audioStreamParams { audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        inputDeviceId, numberOfInputChannels) };

    ASSERT_TRUE(audioStreamParams.has_value());
    const auto inputAudioStreamParams { std::dynamic_pointer_cast<audio_stream_params::InputAudioStreamParams>(audioStreamParams.value()) };
    const auto outputAudioStreamParams { std::dynamic_pointer_cast<audio_stream_params::OutputAudioStreamParams>(audioStreamParams.value()) };
    const auto duplexAudioStreamParams { std::dynamic_pointer_cast<audio_stream_params::DuplexAudioStreamParams>(audioStreamParams.value()) };

    EXPECT_NE(inputAudioStreamParams, nullptr);
    EXPECT_EQ(outputAudioStreamParams, nullptr);
    EXPECT_EQ(duplexAudioStreamParams, nullptr);

    EXPECT_EQ(inputAudioStreamParams->m_sampleRate, 44100);
    EXPECT_EQ(inputAudioStreamParams->m_format, format);
    EXPECT_EQ(inputAudioStreamParams->m_bufferLength, bufferLength);
    EXPECT_EQ(inputAudioStreamParams->m_inputDeviceId, inputDeviceId);
    EXPECT_EQ(inputAudioStreamParams->m_numberOfInputChannels, numberOfInputChannels);
}

TEST(AudioStreamParams, createOutputAudioStreamParams) {
    constexpr audio_device::SampleRate_t sampleRate { 44100 };
    constexpr auto format { audio_format::AudioFormat::Float32 };
    constexpr audio_stream_params::BufferLength_t bufferLength { 2048 };
    constexpr audio_stream_params::PeriodSize_t periodSize { 3 };
    const audio_device::DeviceId outputDeviceId { 3 };
    constexpr audio_device::ChannelCount_t numberOfOutputChannels { 4 };

    const auto audioStreamParams { audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        std::nullopt, std::nullopt, outputDeviceId, numberOfOutputChannels) };

    ASSERT_TRUE(audioStreamParams.has_value());
    const auto inputAudioStreamParams { std::dynamic_pointer_cast<audio_stream_params::InputAudioStreamParams>(audioStreamParams.value()) };
    const auto outputAudioStreamParams { std::dynamic_pointer_cast<audio_stream_params::OutputAudioStreamParams>(audioStreamParams.value()) };
    const auto duplexAudioStreamParams { std::dynamic_pointer_cast<audio_stream_params::DuplexAudioStreamParams>(audioStreamParams.value()) };

    EXPECT_EQ(inputAudioStreamParams, nullptr);
    EXPECT_NE(outputAudioStreamParams, nullptr);
    EXPECT_EQ(duplexAudioStreamParams, nullptr);

    EXPECT_EQ(outputAudioStreamParams->m_sampleRate, 44100);
    EXPECT_EQ(outputAudioStreamParams->m_format, format);
    EXPECT_EQ(outputAudioStreamParams->m_bufferLength, bufferLength);
    EXPECT_EQ(outputAudioStreamParams->m_outputDeviceId, outputDeviceId);
    EXPECT_EQ(outputAudioStreamParams->m_numberOfOutputChannels, numberOfOutputChannels);
}

TEST(AudioStreamParams, createDuplexAudioStreamParams) {
    constexpr audio_device::SampleRate_t sampleRate { 44100 };
    constexpr auto format { audio_format::AudioFormat::Float32 };
    constexpr audio_stream_params::BufferLength_t bufferLength { 2048 };
    constexpr audio_stream_params::PeriodSize_t periodSize { 3 };
    const audio_device::DeviceId inputDeviceId { 2 };
    constexpr audio_device::ChannelCount_t numberOfInputChannels { 3 };
    const audio_device::DeviceId outputDeviceId { 3 };
    constexpr audio_device::ChannelCount_t numberOfOutputChannels { 4 };

    const auto audioStreamParams { audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        inputDeviceId, numberOfInputChannels, outputDeviceId, numberOfOutputChannels) };

    ASSERT_TRUE(audioStreamParams.has_value());
    const auto inputAudioStreamParams { std::dynamic_pointer_cast<audio_stream_params::InputAudioStreamParams>(audioStreamParams.value()) };
    const auto outputAudioStreamParams { std::dynamic_pointer_cast<audio_stream_params::OutputAudioStreamParams>(audioStreamParams.value()) };
    const auto duplexAudioStreamParams { std::dynamic_pointer_cast<audio_stream_params::DuplexAudioStreamParams>(audioStreamParams.value()) };

    EXPECT_NE(inputAudioStreamParams, nullptr);
    EXPECT_NE(outputAudioStreamParams, nullptr);
    EXPECT_NE(duplexAudioStreamParams, nullptr);

    EXPECT_EQ(duplexAudioStreamParams->m_sampleRate, 44100);
    EXPECT_EQ(duplexAudioStreamParams->m_format, format);
    EXPECT_EQ(duplexAudioStreamParams->m_bufferLength, bufferLength);
    EXPECT_EQ(duplexAudioStreamParams->m_inputDeviceId, inputDeviceId);
    EXPECT_EQ(duplexAudioStreamParams->m_numberOfInputChannels, numberOfInputChannels);
    EXPECT_EQ(duplexAudioStreamParams->m_outputDeviceId, outputDeviceId);
    EXPECT_EQ(duplexAudioStreamParams->m_numberOfOutputChannels, numberOfOutputChannels);
}

TEST(AudioStreamParams, invalidAudioStreamParams) {
    audio_device::SampleRate_t sampleRate { 22050 };
    auto format { audio_format::AudioFormat::SignedInt32 };
    audio_stream_params::BufferLength_t bufferLength { 31 };
    audio_stream_params::PeriodSize_t periodSize { 1 };
    std::optional<audio_device::DeviceId> inputDeviceId { std::nullopt };
    std::optional<audio_device::ChannelCount_t> numberOfInputChannels { std::nullopt };
    std::optional<audio_device::DeviceId> outputDeviceId { std::nullopt };
    std::optional<audio_device::ChannelCount_t> numberOfOutputChannels { std::nullopt };

    auto audioStreamParams { audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize) };
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid sample rate" } } );

    sampleRate = 44100;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid format" } } );

    format = audio_format::AudioFormat::Float32;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid buffer length" } } );

    bufferLength = 2048;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid period size" } } );

    periodSize = 3;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "No devices provided" } } );

    inputDeviceId = audio_device::DeviceId { 1 };

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, inputDeviceId);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Number of input channels not set" } } );

    numberOfInputChannels = 0;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, inputDeviceId, numberOfInputChannels);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid number of input channels" } } );

    numberOfInputChannels = 1;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, inputDeviceId, numberOfInputChannels);
    ASSERT_TRUE(audioStreamParams.has_value());

    inputDeviceId = std::nullopt;
    numberOfInputChannels = std::nullopt;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, inputDeviceId, numberOfInputChannels, outputDeviceId, numberOfOutputChannels);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "No devices provided" } } );

    outputDeviceId = audio_device::DeviceId { 1 };

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, std::nullopt, std::nullopt, outputDeviceId);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Number of output channels not set" } } );

    numberOfOutputChannels = 0;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, std::nullopt, std::nullopt, outputDeviceId, numberOfOutputChannels);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid number of output channels" } } );

    numberOfOutputChannels = 1;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, std::nullopt, std::nullopt, outputDeviceId, numberOfOutputChannels);
    ASSERT_TRUE(audioStreamParams.has_value());

    outputDeviceId = std::nullopt;
    numberOfOutputChannels = std::nullopt;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, inputDeviceId, numberOfInputChannels, outputDeviceId, numberOfOutputChannels);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "No devices provided" } } );
}
