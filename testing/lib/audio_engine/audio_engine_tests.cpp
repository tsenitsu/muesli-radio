#include <gtest/gtest.h>
#include <gmock/gmock.h>

import std;
import audio_engine;

using namespace audio_engine;

class AudioLibraryWrapperMock final: public audio_library_wrapper::AudioLibraryWrapper {
public:
    AudioLibraryWrapperMock([[maybe_unused]] const audio_library_wrapper::LogCallback& callback, [[maybe_unused]] const audio_driver::AudioDriver audioDriver) : AudioLibraryWrapper(nullptr) {}

    MOCK_METHOD((std::expected<std::vector<std::unique_ptr<const audio_device::AudioDevice>>, std::string>), probeDevices, (), (override));
    MOCK_METHOD((std::expected<audio_driver::AudioDriver, std::string>), audioDriver, (), (const, override));
    MOCK_METHOD(bool, openStream, (const audio_stream_params::AudioStreamParams& audioStreamParams, const audio_library_wrapper::AudioCallback& audioCallback), (override));
    MOCK_METHOD(void, closeStream, (), (override));
    MOCK_METHOD(bool, startStream, (), (override));
    MOCK_METHOD(bool, stopStream, (), (override));
    MOCK_METHOD(bool, isStreamOpen, (),  (const, override));
    MOCK_METHOD(bool, isStreamRunning, (), (const, override));
};

template <class T>
class AudioEngineMock final: public AudioEngine<T> {
public:
    AudioEngineMock() : AudioEngine<T> { nullptr } {}

    using AudioEngine<T>::getAudioDevice;
    using AudioEngine<T>::isBufferLengthAllowed;
    using AudioEngine<T>::openStream;
    using AudioEngine<T>::closeStream;
    using AudioEngine<T>::processInput;
    using AudioEngine<T>::processOutput;

    using AudioEngine<T>::m_allowedBufferLengths;
    using AudioEngine<T>::m_audioDevices;
    using AudioEngine<T>::m_audioLibraryWrapper;
    using AudioEngine<T>::m_audioStreamParams;
    using AudioEngine<T>::m_processedInputBuffer;
};

class AudioEngineTest: public testing::Test {
protected:
    AudioEngineMock<AudioLibraryWrapperMock> m_audioEngineMock;
};

auto makeInputDevice() -> auto { return audio_device::makeAudioDevice(audio_device::DeviceId { 1 }, "input", false,
                    audio_device::AudioDeviceType::Input, std::vector { audio_device::NativeDataFormat { audio_format::AudioFormat::Float32, 2, 44100, 0 } }).value(); }

auto makeOutputDevice() -> auto { return audio_device::makeAudioDevice(audio_device::DeviceId { 2 }, "output", false,
                    audio_device::AudioDeviceType::Output, std::vector { audio_device::NativeDataFormat { audio_format::AudioFormat::Float32, 2, 44100, 0 } }).value(); }

auto makeDefaultInputDevice() -> auto { return audio_device::makeAudioDevice(audio_device::DeviceId { 1 }, "defaultInput", true,
                    audio_device::AudioDeviceType::Input, std::vector { audio_device::NativeDataFormat { audio_format::AudioFormat::Float32, 2, 44100, 0 } }).value(); }

auto makeDefaultOutputDevice() -> auto { return audio_device::makeAudioDevice(audio_device::DeviceId { 2 }, "defaultOutput", true,
                    audio_device::AudioDeviceType::Output, std::vector { audio_device::NativeDataFormat { audio_format::AudioFormat::Float32, 2, 44100, 0 } }).value(); }

TEST_F(AudioEngineTest, getDefaultAudioDevice) {
    EXPECT_EQ(m_audioEngineMock.defaultInputAudioDeviceName(), std::unexpected { "Audio device not found" });
    EXPECT_EQ(m_audioEngineMock.defaultAudioOutputDeviceName(), std::unexpected { "Audio device not found" });

    m_audioEngineMock.m_audioDevices.push_back(makeInputDevice());
    m_audioEngineMock.m_audioDevices.push_back(makeOutputDevice());

    EXPECT_EQ(m_audioEngineMock.defaultInputAudioDeviceName(), std::unexpected { "Audio device not found" });
    EXPECT_EQ(m_audioEngineMock.defaultAudioOutputDeviceName(), std::unexpected { "Audio device not found" });

    m_audioEngineMock.m_audioDevices.push_back(makeDefaultInputDevice());
    m_audioEngineMock.m_audioDevices.push_back(makeDefaultOutputDevice());

    EXPECT_EQ(m_audioEngineMock.defaultInputAudioDeviceName(), (std::expected<std::string, std::string> { "defaultInput" }));
    EXPECT_EQ(m_audioEngineMock.defaultAudioOutputDeviceName(), (std::expected<std::string, std::string> { "defaultOutput" }));
}

TEST_F(AudioEngineTest, getAudioDevice) {

    EXPECT_EQ(m_audioEngineMock.getAudioDevice("input", audio_device::AudioDeviceType::Input), std::unexpected { "Audio device not found" });

    m_audioEngineMock.m_audioDevices.push_back(makeInputDevice());

    EXPECT_EQ(***m_audioEngineMock.getAudioDevice("input", audio_device::AudioDeviceType::Input), *makeInputDevice());
    EXPECT_EQ(m_audioEngineMock.getAudioDevice("input", audio_device::AudioDeviceType::Output), std::unexpected { "Audio device is not of type provided" });
    EXPECT_EQ(m_audioEngineMock.getAudioDevice("output", audio_device::AudioDeviceType::Output), std::unexpected { "Audio device not found" });

    m_audioEngineMock.m_audioDevices.push_back(makeOutputDevice());

    EXPECT_EQ(***m_audioEngineMock.getAudioDevice("output", audio_device::AudioDeviceType::Output), *makeOutputDevice());
    EXPECT_EQ(m_audioEngineMock.getAudioDevice("output", audio_device::AudioDeviceType::Input), std::unexpected { "Audio device is not of type provided" });
    EXPECT_EQ(m_audioEngineMock.getAudioDevice("non existing device", audio_device::AudioDeviceType::Input), std::unexpected { "Audio device not found" });
}

TEST_F(AudioEngineTest, isBufferLengthALlowed) {
    for (const auto bufferLength: m_audioEngineMock.m_allowedBufferLengths) {
        EXPECT_TRUE(m_audioEngineMock.isBufferLengthAllowed(bufferLength));
    }

    EXPECT_FALSE(m_audioEngineMock.isBufferLengthAllowed(512));
    EXPECT_FALSE(m_audioEngineMock.isBufferLengthAllowed(8193));
}

TEST_F(AudioEngineTest, startStream) {
    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), isStreamOpen)
        .Times(6)
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), isStreamRunning)
        .Times(6)
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), startStream)
        .Times(3)
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), stopStream)
        .Times(6)
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), openStream(testing::_, testing::_))
        .Times(3)
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), closeStream)
        .Times(3);

    EXPECT_EQ(m_audioEngineMock.startStream("input", "output", 1234), std::unexpected { std::string { "Buffer length 1234 is not allowed" } });
    EXPECT_EQ(m_audioEngineMock.startStream(std::nullopt, std::nullopt, 2048), std::unexpected { std::string { "Error creating stream params: No devices provided" } });
    EXPECT_EQ(m_audioEngineMock.startStream("input", std::nullopt, 2048), std::unexpected { std::string { "Could not find input device input: Audio device not found" } });
    EXPECT_EQ(m_audioEngineMock.startStream(std::nullopt, "output", 2048), std::unexpected { std::string { "Could not find output device output: Audio device not found" } });

    m_audioEngineMock.m_audioDevices.push_back(makeInputDevice());
    m_audioEngineMock.m_audioDevices.push_back(makeOutputDevice());

    EXPECT_EQ(m_audioEngineMock.startStream("input", std::nullopt, 2048), std::unexpected { std::string { "Could not close running stream" } });
    EXPECT_EQ(m_audioEngineMock.startStream("input", std::nullopt, 2048), (std::expected<void, std::string> {}));
    EXPECT_EQ(m_audioEngineMock.startStream(std::nullopt, "output", 2048), std::unexpected { std::string { "Could not close running stream" } });
    EXPECT_EQ(m_audioEngineMock.startStream(std::nullopt, "output", 2048), (std::expected<void, std::string> {}));
    EXPECT_EQ(m_audioEngineMock.startStream("input", "output", 2048), std::unexpected { std::string { "Could not close running stream" } });
    EXPECT_EQ(m_audioEngineMock.startStream("input", "output", 2048), (std::expected<void, std::string> {}));
}

TEST_F(AudioEngineTest, openStream) {
    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), startStream)
        .Times(2)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), openStream(testing::_, testing::_))
        .Times(3)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));

    m_audioEngineMock.m_audioStreamParams = audio_stream_params::makeAudioStreamParams(44100, audio_format::AudioFormat::Float32,
        2048, 3, audio_device::DeviceId { 1 }, 2).value();

    EXPECT_EQ(m_audioEngineMock.openStream(), true);
    EXPECT_EQ(m_audioEngineMock.openStream(), false);
    EXPECT_EQ(m_audioEngineMock.openStream(), false);
}

TEST_F(AudioEngineTest, closeStream) {
    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), isStreamOpen)
        .Times(4)
        .WillOnce(testing::Return(false))
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), isStreamRunning)
        .Times(3)
        .WillOnce(testing::Return(false))
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), stopStream)
        .Times(2)
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), closeStream)
        .Times(2);

    EXPECT_EQ(m_audioEngineMock.closeStream(), true);
    EXPECT_EQ(m_audioEngineMock.closeStream(), true);
    EXPECT_EQ(m_audioEngineMock.closeStream(), true);
    EXPECT_EQ(m_audioEngineMock.closeStream(), false);
}

TEST_F(AudioEngineTest, process) {
    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), isStreamOpen)
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), isStreamRunning)
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), startStream)
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), stopStream)
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), openStream(testing::_, testing::_))
        .WillRepeatedly(testing::Return(true));

    EXPECT_CALL(static_cast<AudioLibraryWrapperMock&>(*m_audioEngineMock.m_audioLibraryWrapper), closeStream)
        .Times(1);

    m_audioEngineMock.m_audioDevices.push_back(makeInputDevice());
    m_audioEngineMock.m_audioDevices.push_back(makeOutputDevice());

    EXPECT_EQ(m_audioEngineMock.startStream("input", "output", 2048), (std::expected<void, std::string> {}));

    std::array inputSamples { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
    auto inputBuffer { audio_buffer::makeAudioBuffer<float>(audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }) };
    inputBuffer->copyFromRawBuffer(inputSamples.data(), audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);

    std::array outputSamples { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    auto outputBuffer { audio_buffer::makeAudioBuffer<float>(audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }) };
    outputBuffer->copyFromRawBuffer(outputSamples.data(), audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);

    std::array processedInputSamples { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    auto processedInputBuffer { audio_buffer::makeAudioBuffer<float>(audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }) };
    processedInputBuffer->copyFromRawBuffer(processedInputSamples.data(), audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }, false);
    // Swapping because stream was opened with 2048 samples
    m_audioEngineMock.m_processedInputBuffer.swap(processedInputBuffer);
    processedInputBuffer = nullptr;

    m_audioEngineMock.processInput(*inputBuffer, *outputBuffer);
    m_audioEngineMock.m_processedInputBuffer->writeToRawBuffer(processedInputSamples.data(), audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }, false);
    outputBuffer->writeToRawBuffer(outputSamples.data(),audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);
    EXPECT_EQ(processedInputSamples, (std::array { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }));
    EXPECT_EQ(outputSamples, (std::array { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }));

    auto monoRouting { audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 0 }).value() };
    m_audioEngineMock.m_audioMixer->inputRouting(std::make_pair(*monoRouting, audio_mixer::ChannelRouting {}), 1);

    m_audioEngineMock.m_processedInputBuffer->clear();
    outputBuffer->clear();

    m_audioEngineMock.processInput(*inputBuffer, *outputBuffer);
    m_audioEngineMock.m_processedInputBuffer->writeToRawBuffer(processedInputSamples.data(), audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }, false);
    outputBuffer->writeToRawBuffer(outputSamples.data(),audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);
    EXPECT_EQ(processedInputSamples, (std::array { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }));
    EXPECT_EQ(outputSamples, (std::array { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }));

    m_audioEngineMock.m_audioMixer->inputRouting(std::make_pair(*monoRouting, audio_mixer::ChannelRouting {}), 0);

    m_audioEngineMock.m_processedInputBuffer->clear();
    outputBuffer->clear();

    m_audioEngineMock.processInput(*inputBuffer, *outputBuffer);
    m_audioEngineMock.m_processedInputBuffer->writeToRawBuffer(processedInputSamples.data(), audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }, false);
    outputBuffer->writeToRawBuffer(outputSamples.data(),audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);
    EXPECT_EQ(processedInputSamples, (std::array { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }));
    EXPECT_EQ(outputSamples, (std::array { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }));

    auto stereoRouting { audio_mixer::makeChannelRouting(audio_mixer::Routing_t { 0 }, audio_mixer::Routing_t { 1 }).value() };
    m_audioEngineMock.m_audioMixer->inputRouting(std::make_pair(*stereoRouting, audio_mixer::ChannelRouting {}), 1);

    m_audioEngineMock.m_processedInputBuffer->clear();
    outputBuffer->clear();

    m_audioEngineMock.processInput(*inputBuffer, *outputBuffer);
    m_audioEngineMock.m_processedInputBuffer->writeToRawBuffer(processedInputSamples.data(), audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }, false);
    outputBuffer->writeToRawBuffer(outputSamples.data(),audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);
    EXPECT_EQ(processedInputSamples, (std::array { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f }));
    EXPECT_EQ(outputSamples, (std::array { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }));

    m_audioEngineMock.m_audioMixer->inputRouting(std::make_pair(*stereoRouting, audio_mixer::ChannelRouting {}), 0);

    m_audioEngineMock.m_processedInputBuffer->clear();
    outputBuffer->clear();

    m_audioEngineMock.processInput(*inputBuffer, *outputBuffer);
    m_audioEngineMock.m_processedInputBuffer->writeToRawBuffer(processedInputSamples.data(), audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }, false);
    outputBuffer->writeToRawBuffer(outputSamples.data(),audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);
    EXPECT_EQ(processedInputSamples, (std::array { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f }));
    EXPECT_EQ(outputSamples, (std::array { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }));

    m_audioEngineMock.m_audioMixer->inputRouting(std::make_pair(audio_mixer::ChannelRouting {}, audio_mixer::ChannelRouting {}), 0);
    m_audioEngineMock.m_audioMixer->inputRouting(std::make_pair(*stereoRouting, *stereoRouting), 1);
    m_audioEngineMock.m_audioMixer->outputRouting(*stereoRouting, 1);

    m_audioEngineMock.m_processedInputBuffer->clear();
    outputBuffer->clear();

    m_audioEngineMock.processInput(*inputBuffer, *outputBuffer);
    m_audioEngineMock.m_processedInputBuffer->writeToRawBuffer(processedInputSamples.data(), audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }, false);
    outputBuffer->writeToRawBuffer(outputSamples.data(),audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);
    EXPECT_EQ(processedInputSamples, (std::array { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f }));
    EXPECT_EQ(outputSamples, (std::array { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f }));

    m_audioEngineMock.m_processedInputBuffer->clear();
    outputBuffer->clear();

    m_audioEngineMock.m_audioMixer->inputRouting(std::make_pair(*stereoRouting, *stereoRouting), 0);

    m_audioEngineMock.processInput(*inputBuffer, *outputBuffer);
    m_audioEngineMock.m_processedInputBuffer->writeToRawBuffer(processedInputSamples.data(), audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }, false);
    outputBuffer->writeToRawBuffer(outputSamples.data(),audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);
    EXPECT_EQ(processedInputSamples, (std::array { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f }));

    auto expectedResult  { std::array { 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 18.0f, 20.0f } };

    for (size_t i { 0 }; i < outputSamples.size(); ++i) {
        EXPECT_NEAR(outputSamples[i], expectedResult[i], std::numeric_limits<float>::epsilon());
    }

    m_audioEngineMock.m_processedInputBuffer->clear();
    outputBuffer->clear();

    m_audioEngineMock.m_audioMixer->inputRouting(std::make_pair(*monoRouting, *stereoRouting), 0);

    m_audioEngineMock.processInput(*inputBuffer, *outputBuffer);
    m_audioEngineMock.m_processedInputBuffer->writeToRawBuffer(processedInputSamples.data(), audio_device::ChannelCount_t { 4 }, audio_stream_params::BufferLength_t { 5 }, false);
    outputBuffer->writeToRawBuffer(outputSamples.data(),audio_device::ChannelCount_t { 2 }, audio_stream_params::BufferLength_t { 5 }, false);
    EXPECT_EQ(processedInputSamples, (std::array { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f }));

    expectedResult =  std::array { 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };

    for (size_t i { 0 }; i < outputSamples.size(); ++i) {
        EXPECT_NEAR(outputSamples[i], expectedResult[i], std::numeric_limits<float>::epsilon());
    }
}
