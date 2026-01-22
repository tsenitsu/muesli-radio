export module audio_engine;

export import audio_device;
export import audio_driver;
export import audio_stream_params;
export import audio_library_wrapper;
export import miniaudio_library_wrapper;
export import audio_buffer;
export import audio_mixer;
export import audio_channel;
export import channel_routing;

import std;

namespace audio_engine {

export template<class T> requires std::derived_from<T, audio_library_wrapper::AudioLibraryWrapper>
class AudioEngine {
public:
    explicit AudioEngine(const audio_library_wrapper::LogCallback& logCallback, const audio_driver::AudioDriver newAudioDriver = audio_driver::availableAudioDrivers[0])
     :  m_audioMixer { nullptr },
        m_audioLibraryWrapper { nullptr },
        m_audioDevices { std::vector<std::unique_ptr<const audio_device::AudioDevice>> {} },
        m_audioStreamParams { nullptr },
        m_processedInputBuffer { nullptr },
        m_logCallback { logCallback },
        m_audioCallback { [this] (const audio_buffer::AudioBuffer<float>& inputBuffer, const audio_buffer::AudioBuffer<float>& outputBuffer) {
            process(inputBuffer, outputBuffer);
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

        auto streamParamsResult { audio_stream_params::makeAudioStreamParams(m_sampleRate, m_format, bufferLength, m_periodSize, inputDeviceId, inputChannelCount, outputDeviceId, outputChannelCount) };

        if (not streamParamsResult.has_value()) {
            return std::unexpected { std::format("Error creating stream params: {}", streamParamsResult.error()) };
        }

        std::unique_ptr<audio_buffer::AudioBuffer<float>> processedInputBuffer { nullptr };

        if (inputChannelCount.has_value()) {
            processedInputBuffer = audio_buffer::makeAudioBuffer<float>(inputChannelCount.value() * 2, bufferLength);
        }

        // Output audio is always stereo, so for each couple of channels we get 1 mixer channel
        auto audioMixerResult { audio_mixer::makeAudioMixer<float>(inputChannelCount.has_value()? inputChannelCount.value() : 0, outputChannelCount.has_value()? outputChannelCount.value() / 2 : 0) };

        if (not audioMixerResult.has_value()) {
            return std::unexpected { std::format("Error creating audio mixer: {}", audioMixerResult.error()) };
        }

        if (not closeStream()) {
            return std::unexpected { "Could not close running stream" };
        }

        // Audio thread stops here
        m_audioStreamParams.swap(streamParamsResult.value());

        if (processedInputBuffer) {
            m_processedInputBuffer.swap(processedInputBuffer);
            m_processedInputBuffer->clear();
        }

        m_audioMixer.swap(audioMixerResult.value());

        if (not openStream()) {
            return std::unexpected { "Could not open stream" };
        }

        return {};
    }

    std::unique_ptr<audio_mixer::AudioMixer<float>> m_audioMixer;


protected:
    [[nodiscard]] auto getAudioDevice(const std::string& deviceName, audio_device::AudioDeviceType deviceType) const
                    -> std::expected<std::ranges::borrowed_iterator_t<const std::vector<std::unique_ptr<const audio_device::AudioDevice>> &>, std::string> {
        auto deviceItr { std::ranges::find_if(m_audioDevices, [&deviceName, &deviceType] (const auto& currentDevice) { return currentDevice->m_deviceName == deviceName and currentDevice->m_type == deviceType; } ) };

        if (deviceItr == std::ranges::end(m_audioDevices)) {
            return std::unexpected { "Audio device not found" };
        }

        return deviceItr;
    }

    [[nodiscard]] static auto isBufferLengthAllowed(const audio_stream_params::BufferLength_t bufferLength) -> bool {
        return std::ranges::find(m_allowedBufferLengths, bufferLength) != std::ranges::end(m_allowedBufferLengths);
    }

    [[nodiscard]] auto openStream() const -> bool {
        if (m_audioStreamParams == nullptr)
            return false;

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

    [[nodiscard]] auto getDefaultAudioDevice(const audio_device::AudioDeviceType deviceType) const -> std::expected<std::ranges::borrowed_iterator_t<const std::vector<std::unique_ptr<const audio_device::AudioDevice>> &>, std::string> {
        auto deviceItr { std::ranges::find_if(m_audioDevices, [&deviceType] (const auto& currentDevice) { return currentDevice->m_type == deviceType && currentDevice->m_isDefault; }) };

        if (deviceItr == std::ranges::end(m_audioDevices)) {
            return std::unexpected { "Audio device not found" };
        }

        return deviceItr;
    }

    auto processInput(const audio_buffer::AudioBuffer<float>& inputBuffer, const audio_buffer::AudioBuffer<float>& outputBuffer) const -> void {
        const auto inputChannels { inputBuffer.numberOfChannels() };
        for (auto channel { audio_device::ChannelCount_t { 0 } };  channel < inputChannels; ++channel) {
            const auto [inputRouting, outputRouting] { m_audioMixer->inputRouting(channel) };

            if (not inputRouting.isMono() and not inputRouting.isStereo())
                continue;

            auto inputBufferView { inputBuffer.view(inputRouting.m_leftMono.value(), inputRouting.m_right) };
            auto processedInputBufferView { m_processedInputBuffer->view(channel * 2, channel * 2 + 1) };

            m_audioMixer->processInput(inputBufferView, processedInputBufferView, channel);

            if (not outputRouting.isMono() and not outputRouting.isStereo())
                continue;

            auto outputBufferView { outputBuffer.view(outputRouting.m_leftMono.value(), outputRouting.m_right) };
            m_audioMixer->mix(processedInputBufferView, outputBufferView);
        }
    }

    auto processOutput(const audio_buffer::AudioBuffer<float>& outputBuffer) const -> void {
        const auto outputChannels { outputBuffer.numberOfChannels() };
        for (auto channel { audio_device::ChannelCount_t { 0 } }; channel < outputChannels; ++channel) {
            const auto outputRouting { m_audioMixer->outputRouting(channel) };

            if (not outputRouting.isMono() and not outputRouting.isStereo())
                continue;

            // Input routing is same as output for output channels, checking output.
            auto outputBufferView { outputBuffer.view(outputRouting.m_leftMono.value(), outputRouting.m_right) };

            // In place processing
            m_audioMixer->processOutput(outputBufferView,
                outputBufferView,
                channel);
        }
    }

    auto process(const audio_buffer::AudioBuffer<float>& inputBuffer, const audio_buffer::AudioBuffer<float>& outputBuffer) const -> void {
        if (m_processedInputBuffer)
            m_processedInputBuffer->clear();

        processInput(inputBuffer, outputBuffer);
        processOutput(outputBuffer);
    }

    static constexpr audio_device::SampleRate_t m_sampleRate { 48000 };
    static constexpr audio_format::AudioFormat m_format { audio_format::AudioFormat::Float32 };
    static constexpr audio_stream_params::PeriodSize_t m_periodSize { 3 };
    static constexpr std::array<const audio_stream_params::BufferLength_t, 5> m_allowedBufferLengths { 1024, 2048, 4096, 8192, 16384 };

    std::unique_ptr<audio_library_wrapper::AudioLibraryWrapper> m_audioLibraryWrapper;
    std::vector<std::unique_ptr<const audio_device::AudioDevice>> m_audioDevices;
    std::unique_ptr<audio_stream_params::AudioStreamParams> m_audioStreamParams;
    std::unique_ptr<audio_buffer::AudioBuffer<float>> m_processedInputBuffer;

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