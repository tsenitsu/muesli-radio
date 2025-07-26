#include <gtest/gtest.h>

import std;
import audio_device;

using namespace audio_engine;

TEST(AudioDevice, makeAudioDevice) {
    const auto id { audio_device::DeviceId { 123 } };
    auto name { std::string { "Test device" } };
    constexpr auto isDefault { true };
    constexpr auto sampleRate { audio_device::SampleRate_t { 44100 } };
    constexpr auto channelCount { audio_device::ChannelCount_t { 2 } };
    constexpr auto format { audio_format::AudioFormat::Float32 };
    constexpr auto flags { audio_device::Flags_t { 0 } };
    auto formats { std::vector {
        audio_device::NativeDataFormat { format, channelCount, sampleRate, flags }
    } };

    const auto device { audio_device::makeAudioDevice(id, std::move(name), isDefault, audio_device::AudioDeviceType::Input, std::move(formats)) };

    ASSERT_TRUE(device.has_value());
    EXPECT_EQ(device.value()->m_deviceId, id);
    EXPECT_EQ(device.value()->m_deviceName, "Test device");
    EXPECT_EQ(device.value()->m_isDefault, isDefault);
    ASSERT_EQ(device.value()->m_nativeDataFormats.size(), 1);
    EXPECT_EQ(device.value()->m_nativeDataFormats[0].m_sampleRate, sampleRate);
    EXPECT_EQ(device.value()->m_nativeDataFormats[0].m_channels, channelCount);
    EXPECT_EQ(device.value()->m_nativeDataFormats[0].m_flags, flags);
    EXPECT_EQ(device.value()->m_nativeDataFormats[0].m_format, format);

    const auto deviceWithNoFormats { audio_device::makeAudioDevice(id, "Test Device 2", false, audio_device::AudioDeviceType::Output, std::vector<audio_device::NativeDataFormat> {}) };

    ASSERT_FALSE(deviceWithNoFormats.has_value());
    EXPECT_EQ(deviceWithNoFormats, std::unexpected { std::string { "No native formats found" } });
}
