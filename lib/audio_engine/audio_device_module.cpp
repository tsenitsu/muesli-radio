module;
#include <miniaudio.h>
export module audio_device;

import std;
export import audio_format;

namespace audio_engine::audio_device {

export using SampleRate_t = unsigned int;
export using ChannelCount_t = unsigned int;
export using Flags_t = unsigned int;

export class DeviceId final {
public:
    explicit DeviceId(const ma_device_id& id);
    explicit DeviceId(int id);

     [[nodiscard]] ma_device_id id() const;

    auto operator==(const DeviceId& other) const -> bool;
    auto operator<=>(const DeviceId&) const -> std::strong_ordering;

private:
    ma_device_id m_id;
};

export enum class AudioDeviceType {
    Input,
    Output
};

export class NativeDataFormat {
public:
    NativeDataFormat(audio_format::AudioFormat format,
                     ChannelCount_t channels,
                     SampleRate_t sampleRate,
                     Flags_t flags);

    audio_format::AudioFormat m_format;
    ChannelCount_t m_channels;
    SampleRate_t m_sampleRate;
    Flags_t m_flags;

    auto operator<=>(const NativeDataFormat&) const = default;
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

    auto operator<=>(const AudioDevice&) const = default;
};

export [[nodiscard]] auto makeAudioDevice(const DeviceId& id,
                    std::string&& name,
                    bool isDefault,
                    AudioDeviceType type,
                    std::vector<NativeDataFormat>&& formats) -> std::expected<std::unique_ptr<const AudioDevice>, std::string>;

export [[nodiscard]] auto toString(const AudioDevice& device) -> std::string;
export [[nodiscard]] auto toString(const NativeDataFormat& nativeFormat) -> std::string;
}