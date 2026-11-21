module;
#include <miniaudio.h>
export module audio_device;

import std;
export import audio_format;

namespace audio_engine::audio_device {

export using SampleRate_t = unsigned int;
export using ChannelCount_t = unsigned int;
export using Flags_t = unsigned int;

#ifdef _WIN32
// RtAudio identifies devices with a plain unsigned int. A dedicated type gives it its own
// alternative in LibraryDeviceId_t and makes DeviceId::id<RtAudioDeviceId>() self-explanatory.
export struct RtAudioDeviceId final {
    unsigned int m_value;

    auto operator==(const RtAudioDeviceId&) const -> bool = default;
};
#endif

// One alternative per audio library, each with its own DeviceId constructor
#ifdef _WIN32
using LibraryDeviceId_t = std::variant<ma_device_id, RtAudioDeviceId>;
#else
using LibraryDeviceId_t = std::variant<ma_device_id>;
#endif

export class DeviceId final {
public:
    explicit DeviceId(const ma_device_id& id);
#ifdef _WIN32
    explicit DeviceId(RtAudioDeviceId id);
#endif

    // Returns the id in audio library T's representation, or an error if another library produced it.
    // Asking for a type that is not in LibraryDeviceId_t does not compile (std::get_if rejects it).
    template <typename T>
    [[nodiscard]] auto id() const -> std::expected<T, std::string> {
        if (const auto* const value { std::get_if<T>(&m_id) }; value != nullptr)
            return *value;

        return std::unexpected { std::string { "Device id belongs to a different audio library" } };
    }

    auto operator==(const DeviceId& other) const -> bool;
private:
    LibraryDeviceId_t m_id;
};

export enum class AudioDeviceType {
    Input,
    Output
};

export class NativeDataFormat {
public:
    NativeDataFormat(Flags_t flags,
                     audio_format::AudioFormat format,
                     ChannelCount_t minChannels,
                     ChannelCount_t maxChannels,
                     SampleRate_t minSampleRate,
                     SampleRate_t maxSampleRate);

    Flags_t m_flags;
    audio_format::AudioFormat m_format;
    ChannelCount_t m_minChannels;
    ChannelCount_t m_maxChannels;
    SampleRate_t m_minSampleRate;
    SampleRate_t m_maxSampleRate;

    auto operator==(const NativeDataFormat&) const -> bool = default;
};

export struct AudioDeviceSummary final {
    AudioDeviceSummary(std::string deviceName, ChannelCount_t channels);

    std::string m_deviceName;
    ChannelCount_t m_channels;

    auto operator==(const AudioDeviceSummary&) const -> bool = default;
};

export class AudioDevice final {
public:
    AudioDevice(const DeviceId& id,
                std::string&& name,
                bool isDefault,
                AudioDeviceType type,
                std::vector<NativeDataFormat>&& formats);

    DeviceId m_deviceId;
    std::string m_deviceName;
    bool m_isDefault;
    AudioDeviceType m_type;
    std::vector<NativeDataFormat> m_nativeDataFormats;

    auto operator==(const AudioDevice&) const -> bool = default;
};

export [[nodiscard]] auto makeAudioDevice(const DeviceId& id,
                    std::string&& name,
                    bool isDefault,
                    AudioDeviceType type,
                    std::vector<NativeDataFormat>&& formats) -> std::expected<std::unique_ptr<const AudioDevice>, std::string>;

export [[nodiscard]] auto toString(const AudioDevice& device) -> std::string;
export [[nodiscard]] auto toString(const NativeDataFormat& nativeFormat) -> std::string;
}