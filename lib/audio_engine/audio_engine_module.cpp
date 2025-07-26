export module audio_engine;

export import audio_device;
export import audio_driver;
export import audio_stream_params;
export import audio_library_wrapper;
export import miniaudio_library_wrapper;
export import audio_buffer;

import std;

namespace audio_engine {

export template<class T>
class AudioEngine {
public:
    explicit AudioEngine(const audio_library_wrapper::LogCallback& logCallback, const audio_driver::AudioDriver newAudioDriver = audio_driver::availableAudioDrivers[0])
     :  m_audioLibraryWrapper { nullptr },
        m_audioDevices { std::vector<std::shared_ptr<const audio_device::AudioDevice>> {} },
        m_audioStreamParams { nullptr },
        m_logCallback { logCallback },
        m_audioCallback { [] (audio_buffer::AudioBuffer<float>& inputBuffer, audio_buffer::AudioBuffer<float>& outputBuffer) {
            outputBuffer = inputBuffer;
        } } {
        if (const auto setAudioDriverResult { audioDriver(newAudioDriver) }; not setAudioDriverResult.has_value()) {
            throw std::runtime_error { std::string { std::format("Error setting audio driver: {}",  setAudioDriverResult.error()) } };
        }
    }

    virtual ~AudioEngine() = default;

    [[nodiscard]] auto audioDriver(const audio_driver::AudioDriver newAudioDriver) -> std::expected<void, std::string> {
        auto audioLibraryWrapper { audio_library_wrapper::makeAudioLibraryWrapper<T>(m_logCallback, newAudioDriver) };

        if (not audioLibraryWrapper.has_value()) {
            return std::unexpected { audioLibraryWrapper.error() };
        }

        m_audioLibraryWrapper.swap(audioLibraryWrapper.value());
        return {};
    }

    [[nodiscard]] auto audioDriver() const -> std::expected<audio_driver::AudioDriver, std::string> {
        return m_audioLibraryWrapper->audioDriver();
    }

    [[nodiscard]] auto probeDevices() -> std::expected<void, std::string> {
        auto audioDevicesResult { m_audioLibraryWrapper->probeDevices() };

        if (not audioDevicesResult.has_value()) {
            return std::unexpected { std::format("Could not probe devices: {}\n", audioDevicesResult.error()) };
        }

        m_audioDevices.swap(audioDevicesResult.value());
        return {};
    }

    [[nodiscard]] auto defaultInputAudioDeviceName() const -> std::expected<std::string, std::string> {
        return getDefaultAudioDevice(audio_device::AudioDeviceType::Input).transform([] (const auto&& deviceItr) {return (*deviceItr)->m_deviceName; });
    }

    [[nodiscard]] auto defaultAudioOutputDeviceName() const -> std::expected<std::string, std::string> {
        return getDefaultAudioDevice(audio_device::AudioDeviceType::Output).transform([] (const auto&& deviceItr) {return (*deviceItr)->m_deviceName; });
    }

    [[nodiscard]] auto startStream(const std::optional<std::string>& inputDeviceName,
                        const std::optional<std::string>& outputDeviceName, audio_stream_params::BufferLength_t bufferLength) -> std::expected<void, std::string> {
        if (not isBufferLengthAllowed(bufferLength)) {
            return std::unexpected { std::format("Buffer length {} is not allowed", bufferLength) };
        }

        std::optional<audio_device::DeviceId> inputDeviceId { std::nullopt };
        std::optional<audio_device::ChannelCount_t> inputChannelCount { std::nullopt };

        if (inputDeviceName.has_value()) {
            if (const auto deviceItrResult { getAudioDevice(inputDeviceName.value(), audio_device::AudioDeviceType::Input) }; deviceItrResult.has_value()) {
                inputDeviceId = deviceItrResult.value()->get()->m_deviceId;
                inputChannelCount = deviceItrResult.value()->get()->m_nativeDataFormats[0].m_channels;
            } else {
                return std::unexpected { std::format("Could not find input device {}: {}", inputDeviceName.value(), deviceItrResult.error()) };
            }
        }

        std::optional<audio_device::DeviceId> outputDeviceId { std::nullopt };
        std::optional<audio_device::ChannelCount_t> outputChannelCount { std::nullopt };

        if (outputDeviceName.has_value()) {
            if (const auto deviceItrResult { getAudioDevice(outputDeviceName.value(), audio_device::AudioDeviceType::Output) }; deviceItrResult.has_value()) {
                outputDeviceId = deviceItrResult.value()->get()->m_deviceId;
                outputChannelCount = deviceItrResult.value()->get()->m_nativeDataFormats[0].m_channels;
            } else {
                return std::unexpected { std::format("Could not find output device {}: {}", outputDeviceName.value(), deviceItrResult.error()) };
            }
        }

        const auto streamParamsResult { audio_stream_params::makeAudioStreamParams(m_sampleRate, m_format, bufferLength, m_periodSize, inputDeviceId, inputChannelCount, outputDeviceId, outputChannelCount) };

        if (not streamParamsResult.has_value()) {
            return std::unexpected { std::format("Error creating stream params: {}", streamParamsResult.error()) };
        }

        if (not closeStream()) {
            return std::unexpected { "Could not close running stream" };
        }

        // Audio thread stops here
        m_audioStreamParams = std::move(streamParamsResult.value());

        if (not openStream()) {
            return std::unexpected { "Could not open stream" };
        }

        return {};
    }

protected:
    [[nodiscard]] auto getAudioDevice(const std::string& deviceName, audio_device::AudioDeviceType deviceType) const
                    -> std::expected<std::ranges::borrowed_iterator_t<const std::vector<std::shared_ptr<const audio_device::AudioDevice>> &>, std::string> {
        auto deviceItr { std::ranges::find_if(m_audioDevices, [&deviceName] (const auto& currentDevice) { return currentDevice->m_deviceName == deviceName; }) };

        if (deviceItr == std::ranges::end(m_audioDevices)) {
            return std::unexpected { "Audio device not found" };
        }

        if ((*deviceItr)->m_type != deviceType) {
            return std::unexpected { "Audio device is not of type provided" };
        }

        return deviceItr;
    }

    [[nodiscard]] static auto isBufferLengthAllowed(const audio_stream_params::BufferLength_t bufferLength) -> bool {
        return std::ranges::find(m_allowedBufferLengths, bufferLength) != std::ranges::end(m_allowedBufferLengths);
    }

    [[nodiscard]] auto openStream() const -> bool {
        return m_audioLibraryWrapper->openStream(*m_audioStreamParams, m_audioCallback) && m_audioLibraryWrapper->startStream();
    }

    [[nodiscard]] auto closeStream() const -> bool {
        if (not m_audioLibraryWrapper->isStreamOpen()) {
            return true;
        }

        if (not m_audioLibraryWrapper->isStreamRunning()) {
            m_audioLibraryWrapper->closeStream();
            return true;
        }

        if (not m_audioLibraryWrapper->stopStream()) {
            return false;
        }

        m_audioLibraryWrapper->closeStream();
        return true;
    }

    [[nodiscard]] auto getDefaultAudioDevice(const audio_device::AudioDeviceType deviceType) const -> std::expected<std::ranges::borrowed_iterator_t<const std::vector<std::shared_ptr<const audio_device::AudioDevice>> &>, std::string> {
        auto deviceItr { std::ranges::find_if(m_audioDevices, [&deviceType] (const auto& currentDevice) { return currentDevice->m_type == deviceType && currentDevice->m_isDefault; }) };

        if (deviceItr == std::ranges::end(m_audioDevices)) {
            return std::unexpected { "Audio device not found" };
        }

        return deviceItr;
    }

    static constexpr audio_device::SampleRate_t m_sampleRate { 48000 };
    static constexpr audio_format::AudioFormat m_format { audio_format::AudioFormat::Float32 };
    static constexpr audio_stream_params::PeriodSize_t m_periodSize { 3 };
    static constexpr std::array<const audio_stream_params::BufferLength_t, 5> m_allowedBufferLengths { 1024, 2048, 4096, 8192, 16384 };

    std::unique_ptr<audio_library_wrapper::AudioLibraryWrapper> m_audioLibraryWrapper;
    std::vector<std::shared_ptr<const audio_device::AudioDevice>> m_audioDevices;
    std::shared_ptr<audio_stream_params::AudioStreamParams> m_audioStreamParams;

private:
    audio_library_wrapper::LogCallback m_logCallback;
    audio_library_wrapper::AudioCallback m_audioCallback;
};

export template <class T>
[[nodiscard]] auto makeAudioEngine(const audio_library_wrapper::LogCallback& logCallback, audio_driver::AudioDriver audioDriver = audio_driver::availableAudioDrivers[0]) -> std::expected<std::unique_ptr<AudioEngine<T>>, std::string> {
    try {
        return std::make_unique<AudioEngine<T>>(logCallback, audioDriver);
    } catch (const std::exception& e) {
        return std::unexpected { std::string { e.what() } };
    }
}

}