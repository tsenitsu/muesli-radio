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
        audio_stream_params::DeviceSelection { inputDeviceId, numberOfInputChannels }) };

    ASSERT_TRUE(audioStreamParams.has_value());

    EXPECT_NO_THROW({
        const auto& inputAudioStreamParams { dynamic_cast<audio_stream_params::InputAudioStreamParams&>(*audioStreamParams.value()) };

        EXPECT_EQ(inputAudioStreamParams.m_sampleRate, 44100);
        EXPECT_EQ(inputAudioStreamParams.m_format, format);
        EXPECT_EQ(inputAudioStreamParams.m_bufferLength, bufferLength);
        EXPECT_EQ(inputAudioStreamParams.m_inputDeviceId, inputDeviceId);
        EXPECT_EQ(inputAudioStreamParams.m_numberOfInputChannels, numberOfInputChannels);
    });

    EXPECT_THROW({
        [[maybe_unused]] const auto& outputAudioStreamParams { dynamic_cast<audio_stream_params::OutputAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);

    EXPECT_THROW({
        [[maybe_unused]] const auto& duplexAudioStreamParams { dynamic_cast<audio_stream_params::DuplexAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);

    EXPECT_THROW({
        [[maybe_unused]] const auto& loopbackAudioStreamParams { dynamic_cast<audio_stream_params::LoopbackAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);
}

TEST(AudioStreamParams, createOutputAudioStreamParams) {
    constexpr audio_device::SampleRate_t sampleRate { 44100 };
    constexpr auto format { audio_format::AudioFormat::Float32 };
    constexpr audio_stream_params::BufferLength_t bufferLength { 2048 };
    constexpr audio_stream_params::PeriodSize_t periodSize { 3 };
    const audio_device::DeviceId outputDeviceId { 3 };
    constexpr audio_device::ChannelCount_t numberOfOutputChannels { 4 };

    const auto audioStreamParams { audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        std::nullopt, audio_stream_params::DeviceSelection { outputDeviceId, numberOfOutputChannels }) };

    ASSERT_TRUE(audioStreamParams.has_value());

    EXPECT_THROW({
        [[maybe_unused]] const auto& inputAudioStreamParams { dynamic_cast<audio_stream_params::InputAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);

    EXPECT_NO_THROW({
        const auto& outputAudioStreamParams { dynamic_cast<audio_stream_params::OutputAudioStreamParams&>(*audioStreamParams.value()) };

        EXPECT_EQ(outputAudioStreamParams.m_sampleRate, 44100);
        EXPECT_EQ(outputAudioStreamParams.m_format, format);
        EXPECT_EQ(outputAudioStreamParams.m_bufferLength, bufferLength);
        EXPECT_EQ(outputAudioStreamParams.m_outputDeviceId, outputDeviceId);
        EXPECT_EQ(outputAudioStreamParams.m_numberOfOutputChannels, numberOfOutputChannels);
    });

    EXPECT_THROW({
        [[maybe_unused]] const auto& duplexAudioStreamParams { dynamic_cast<audio_stream_params::DuplexAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);

    EXPECT_THROW({
        [[maybe_unused]] const auto& loopbackAudioStreamParams { dynamic_cast<audio_stream_params::LoopbackAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);
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
        audio_stream_params::DeviceSelection { inputDeviceId, numberOfInputChannels },
        audio_stream_params::DeviceSelection { outputDeviceId, numberOfOutputChannels }) };

    ASSERT_TRUE(audioStreamParams.has_value());

    EXPECT_NO_THROW({
        [[maybe_unused]] const auto& inputStreamParams { dynamic_cast<audio_stream_params::InputAudioStreamParams&>(*audioStreamParams.value()) };
    });

    EXPECT_NO_THROW({
        [[maybe_unused]] const auto& outputStreamParams { dynamic_cast<audio_stream_params::OutputAudioStreamParams&>(*audioStreamParams.value()) };
    });

    EXPECT_NO_THROW({
        const auto& duplexAudioStreamParams { dynamic_cast<audio_stream_params::DuplexAudioStreamParams&>(*audioStreamParams.value()) };

        EXPECT_EQ(duplexAudioStreamParams.m_sampleRate, 44100);
        EXPECT_EQ(duplexAudioStreamParams.m_format, format);
        EXPECT_EQ(duplexAudioStreamParams.m_bufferLength, bufferLength);
        EXPECT_EQ(duplexAudioStreamParams.m_inputDeviceId, inputDeviceId);
        EXPECT_EQ(duplexAudioStreamParams.m_numberOfInputChannels, numberOfInputChannels);
        EXPECT_EQ(duplexAudioStreamParams.m_outputDeviceId, outputDeviceId);
        EXPECT_EQ(duplexAudioStreamParams.m_numberOfOutputChannels, numberOfOutputChannels);
    });

    EXPECT_THROW({
        [[maybe_unused]] const auto& loopbackAudioStreamParams { dynamic_cast<audio_stream_params::LoopbackAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);
}

TEST(AudioStreamParams, createLoopbackAudioStreamParams) {
    constexpr audio_device::SampleRate_t sampleRate { 48000 };
    constexpr auto format { audio_format::AudioFormat::Float32 };
    constexpr audio_stream_params::BufferLength_t bufferLength { 2048 };
    constexpr audio_stream_params::PeriodSize_t periodSize { 3 };
    const audio_device::DeviceId loopbackDeviceId { 5 };
    constexpr audio_device::ChannelCount_t numberOfLoopbackChannels { 2 };

    const auto audioStreamParams { audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        std::nullopt, std::nullopt, audio_stream_params::DeviceSelection { loopbackDeviceId, numberOfLoopbackChannels }) };

    ASSERT_TRUE(audioStreamParams.has_value());

    EXPECT_NO_THROW({
        const auto& loopbackAudioStreamParams { dynamic_cast<audio_stream_params::LoopbackAudioStreamParams&>(*audioStreamParams.value()) };

        EXPECT_EQ(loopbackAudioStreamParams.m_sampleRate, 48000);
        EXPECT_EQ(loopbackAudioStreamParams.m_format, format);
        EXPECT_EQ(loopbackAudioStreamParams.m_bufferLength, bufferLength);
        EXPECT_EQ(loopbackAudioStreamParams.m_periodSize, periodSize);
        EXPECT_EQ(loopbackAudioStreamParams.m_loopbackDeviceId, loopbackDeviceId);
        EXPECT_EQ(loopbackAudioStreamParams.m_numberOfLoopbackChannels, numberOfLoopbackChannels);
    });

    EXPECT_THROW({
        [[maybe_unused]] const auto& inputAudioStreamParams { dynamic_cast<audio_stream_params::InputAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);

    EXPECT_THROW({
        [[maybe_unused]] const auto& outputAudioStreamParams { dynamic_cast<audio_stream_params::OutputAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);

    EXPECT_THROW({
        [[maybe_unused]] const auto& duplexAudioStreamParams { dynamic_cast<audio_stream_params::DuplexAudioStreamParams&>(*audioStreamParams.value()) };
    }, std::bad_cast);
}

TEST(AudioStreamParams, invalidAudioStreamParams) {
    audio_device::SampleRate_t sampleRate { 22050 };
    auto format { audio_format::AudioFormat::SignedInt32 };
    audio_stream_params::BufferLength_t bufferLength { 31 };
    audio_stream_params::PeriodSize_t periodSize { 1 };
    std::optional<audio_stream_params::DeviceSelection> input { std::nullopt };
    std::optional<audio_stream_params::DeviceSelection> output { std::nullopt };

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

    input = audio_stream_params::DeviceSelection { audio_device::DeviceId { 1 }, 0 };

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, input);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid number of input channels" } } );

    input = audio_stream_params::DeviceSelection { audio_device::DeviceId { 1 }, 1 };

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, input);
    ASSERT_TRUE(audioStreamParams.has_value());

    input = std::nullopt;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, input, output);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "No devices provided" } } );

    output = audio_stream_params::DeviceSelection { audio_device::DeviceId { 1 }, 0 };

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, std::nullopt, output);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid number of output channels" } } );

    output = audio_stream_params::DeviceSelection { audio_device::DeviceId { 1 }, 1 };

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, std::nullopt, output);
    ASSERT_TRUE(audioStreamParams.has_value());

    output = std::nullopt;

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize, input, output);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "No devices provided" } } );
}

TEST(AudioStreamParams, invalidDuplexChannels) {
    constexpr audio_device::SampleRate_t sampleRate { 44100 };
    constexpr auto format { audio_format::AudioFormat::Float32 };
    constexpr audio_stream_params::BufferLength_t bufferLength { 2048 };
    constexpr audio_stream_params::PeriodSize_t periodSize { 3 };

    const audio_stream_params::DeviceSelection validSelection { audio_device::DeviceId { 1 }, 2 };
    const audio_stream_params::DeviceSelection zeroChannelsSelection { audio_device::DeviceId { 1 }, 0 };

    auto audioStreamParams { audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        zeroChannelsSelection, validSelection) };
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid number of input channels" } } );

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        validSelection, zeroChannelsSelection);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid number of output channels" } } );

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        validSelection, validSelection);
    ASSERT_TRUE(audioStreamParams.has_value());
}

TEST(AudioStreamParams, invalidLoopbackAudioStreamParams) {
    constexpr audio_device::SampleRate_t sampleRate { 44100 };
    constexpr auto format { audio_format::AudioFormat::Float32 };
    constexpr audio_stream_params::BufferLength_t bufferLength { 2048 };
    constexpr audio_stream_params::PeriodSize_t periodSize { 3 };

    const audio_stream_params::DeviceSelection validSelection { audio_device::DeviceId { 1 }, 2 };
    const audio_stream_params::DeviceSelection zeroChannelsSelection { audio_device::DeviceId { 1 }, 0 };

    // Zero loopback channels.
    auto audioStreamParams { audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        std::nullopt, std::nullopt, zeroChannelsSelection) };
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid number of loopback channels" } } );

    // Loopback cannot be combined with input.
    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        validSelection, std::nullopt, validSelection);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Loopback cannot be combined with input or output" } } );

    // Loopback cannot be combined with output.
    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        std::nullopt, validSelection, validSelection);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Loopback cannot be combined with input or output" } } );

    // Loopback cannot be combined with input and output (duplex).
    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        validSelection, validSelection, validSelection);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Loopback cannot be combined with input or output" } } );

    // The common parameter checks still apply to loopback streams.
    audioStreamParams = audio_stream_params::makeAudioStreamParams(22050, format, bufferLength, periodSize,
        std::nullopt, std::nullopt, validSelection);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid sample rate" } } );

    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, 31, periodSize,
        std::nullopt, std::nullopt, validSelection);
    ASSERT_FALSE(audioStreamParams.has_value());
    EXPECT_EQ(audioStreamParams, std::unexpected { std::string { "Invalid buffer length" } } );

    // Sanity check: a valid loopback request succeeds.
    audioStreamParams = audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        std::nullopt, std::nullopt, validSelection);
    ASSERT_TRUE(audioStreamParams.has_value());
}

TEST(AudioStreamParams, loopbackAudioStreamParamsToString) {
    constexpr audio_device::SampleRate_t sampleRate { 48000 };
    constexpr auto format { audio_format::AudioFormat::Float32 };
    constexpr audio_stream_params::BufferLength_t bufferLength { 2048 };
    constexpr audio_stream_params::PeriodSize_t periodSize { 3 };

    const auto audioStreamParams { audio_stream_params::makeAudioStreamParams(sampleRate, format, bufferLength, periodSize,
        std::nullopt, std::nullopt, audio_stream_params::DeviceSelection { audio_device::DeviceId { 5 }, 2 }) };

    ASSERT_TRUE(audioStreamParams.has_value());

    const auto text { audio_stream_params::toString(*audioStreamParams.value()) };

    EXPECT_TRUE(text.contains("Number of loopback channels: 2"));
    EXPECT_FALSE(text.contains("input channels"));
    EXPECT_FALSE(text.contains("output channels"));
}