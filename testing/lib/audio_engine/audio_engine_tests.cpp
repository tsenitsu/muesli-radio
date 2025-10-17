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

    using AudioEngine<T>::m_allowedBufferLengths;
    using AudioEngine<T>::m_audioDevices;
    using AudioEngine<T>::m_audioLibraryWrapper;

    friend class AudioEngineTest;
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
