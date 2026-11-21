module;
#include <miniaudio.h>
module audio_device;

namespace audio_engine::audio_device {

DeviceId::DeviceId(const ma_device_id& id)
 :  m_id { id } {}

#ifdef _WIN32
DeviceId::DeviceId(const RtAudioDeviceId id)
 :  m_id { id } {}
#endif

auto DeviceId::operator==(const DeviceId &other) const -> bool {
    // Ids from different audio libraries never refer to the same device
    if (m_id.index() != other.m_id.index())
        return false;

    return std::visit([&other]<typename T>(const T& value) -> bool {
        const auto& otherValue { std::get<T>(other.m_id) };

        // ma_device_id is a union without operator==
        if constexpr (std::same_as<T, ma_device_id>)
            return ma_device_id_equal(&value, &otherValue) != MA_FALSE;
        else
            return value == otherValue;
    }, m_id);
}

NativeDataFormat::NativeDataFormat(const Flags_t flags,
                         const audio_format::AudioFormat format,
                         const ChannelCount_t minChannels,
                         const ChannelCount_t maxChannels,
                         const SampleRate_t minSampleRate,
                         const SampleRate_t maxSampleRate)
 :  m_flags { flags },
    m_format  { format },
    m_minChannels { minChannels },
    m_maxChannels { maxChannels },
    m_minSampleRate { minSampleRate },
    m_maxSampleRate { maxSampleRate }
{}

AudioDeviceSummary::AudioDeviceSummary(std::string deviceName, const ChannelCount_t channels)
 :  m_deviceName { std::move(deviceName) },
    m_channels { channels } {}

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
        "Flags: {}\n"
        "Format: {}\n"
        "Min Channels: {}\n"
        "Max Channels: {}\n"
        "Min Sample Rate: {}\n"
        "Max Sample Rate: {}\n",
        nativeFormat.m_flags,
        audio_format::toString(nativeFormat.m_format).value(),
        nativeFormat.m_minChannels,
        nativeFormat.m_maxChannels,
        nativeFormat.m_minSampleRate,
        nativeFormat.m_maxSampleRate);
}

}