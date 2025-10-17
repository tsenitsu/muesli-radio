module;
#include <miniaudio.h>
module audio_device;

namespace audio_engine::audio_device {

DeviceId::DeviceId(const ma_device_id& id)
 :  m_id { id } {}

DeviceId::DeviceId(const int id)
 :  m_id { .nullbackend = id } {}

ma_device_id DeviceId::id() const {
    return m_id;
}

auto DeviceId::operator==(const DeviceId &other) const -> bool {
    return ma_device_id_equal(&m_id, &other.m_id);
}

auto DeviceId::operator<=>(const DeviceId &other) const -> std::strong_ordering {
    if (ma_device_id_equal(&m_id, &other.m_id))
        return std::strong_ordering::equal;

    return std::strong_ordering::greater;
}

NativeDataFormat::NativeDataFormat(const audio_format::AudioFormat format,
                         const ChannelCount_t channels,
                         const SampleRate_t sampleRate,
                         const Flags_t flags)
 :  m_format { format },
    m_channels { channels },
    m_sampleRate { sampleRate },
    m_flags { flags }
{}

AudioDevice::AudioDevice(const DeviceId& id,
                        std::string&& name,
                        const bool isDefault,
                        const AudioDeviceType type,
                        std::vector<NativeDataFormat>&& formats)
 :  m_deviceId      { id },
    m_deviceName    { std::move(name) },
    m_isDefault     { isDefault },
    m_type          { type },
    m_nativeDataFormats { std::move(formats) }
{}

auto makeAudioDevice(const DeviceId& id,
                    std::string&& name,
                    const bool isDefault,
                    const AudioDeviceType type,
                    std::vector<NativeDataFormat>&& formats) -> std::expected<std::unique_ptr<const AudioDevice>, std::string> {

    if (formats.size() == 0)
        return std::unexpected { std::string { "No native formats found" } };

    return std::make_unique<AudioDevice>(id, std::move(name), isDefault, type, std::move(formats));
}

auto toString(const AudioDevice& device) -> std::string {
    std::string deviceStr { "" };
    deviceStr.append(std::format(
        "Name: {}\n"
        "Default: {}\n"
        "Type: {}\n"
        "Formats:\n",
        device.m_deviceName,
        device.m_isDefault,
        device.m_type == AudioDeviceType::Input? "Input" : "Output"
        ));

    for (const auto& format : device.m_nativeDataFormats) {
        deviceStr.append(toString(format));
    }

    return deviceStr;
}

auto toString(const NativeDataFormat& nativeFormat) -> std::string {
    return std::format(
        "Format: {}\n"
        "Channels: {}\n"
        "Sample Rate: {}\n"
        "Flags: {}\n",
        audio_format::toString(nativeFormat.m_format).value(),
        nativeFormat.m_channels,
        nativeFormat.m_sampleRate,
        nativeFormat.m_flags);
}

}