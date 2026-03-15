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
export import audio_recorder;
export import ring_audio_buffer;
export import audio_format;

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
        m_inputRingAudioBuffer { nullptr },
        m_outputRingAudioBuffer { nullptr },
        m_isRecording { false },
        m_inputRecorder { nullptr },
        m_outputRecorder { nullptr },
        m_inputAudioStats { std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> {} },
        m_outputAudioStats { std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> {} },
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

        std::unique_ptr<ring_audio_buffer::RingAudioBuffer<float>> inputRingAudioBuffer { nullptr };

        if (inputChannelCount.has_value()) {
            // 10 seconds of audio in ring buffer
            if (auto inputRingAudioBufferResult = ring_audio_buffer::makeRingAudioBuffer<float>(inputChannelCount.value(), streamParamsResult.value()->m_sampleRate * 10); not inputRingAudioBufferResult.has_value()) {
                return std::unexpected { std::format("Error creating input ring audio buffer: {}", inputRingAudioBufferResult.error()) };
            } else {
                inputRingAudioBuffer.swap(inputRingAudioBufferResult.value());
            }
        }

        std::unique_ptr<ring_audio_buffer::RingAudioBuffer<float>> outputRingAudioBuffer { nullptr };

        if (outputChannelCount.has_value()) {
            // 10 seconds of audio in ring buffer
            if (auto outputRingAudioBufferResult = ring_audio_buffer::makeRingAudioBuffer<float>(outputChannelCount.value(), streamParamsResult.value()->m_sampleRate * 10); not outputRingAudioBufferResult.has_value()) {
                return std::unexpected { std::format("Error creating output ring audio buffer: {}", outputRingAudioBufferResult.error()) };
            } else {
               outputRingAudioBuffer.swap(outputRingAudioBufferResult.value());
            }
        }

        std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> inputAudioStats {};
        std::ranges::generate_n(std::back_inserter(inputAudioStats), inputChannelCount.value_or(0), [] () { return audio_buffer::makeAudioStats<float>(); });

        std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> outputAudioStats {};
        std::ranges::generate_n(std::back_inserter(outputAudioStats), outputChannelCount.value_or(0), [] () { return audio_buffer::makeAudioStats<float>(); });

        if (not closeStream()) {
            return std::unexpected { "Could not close running stream" };
        }

        stopRecording();

        // Audio thread stops here
        m_audioStreamParams.swap(streamParamsResult.value());

        if (processedInputBuffer) {
            m_processedInputBuffer.swap(processedInputBuffer);
            m_processedInputBuffer->clear();
        }

        m_audioMixer.swap(audioMixerResult.value());

        m_inputRingAudioBuffer.swap(inputRingAudioBuffer);
        m_outputRingAudioBuffer.swap(outputRingAudioBuffer);

        m_inputAudioStats.swap(inputAudioStats);
        m_outputAudioStats.swap(outputAudioStats);

        if (not openStream()) {
            return std::unexpected { "Could not open stream" };
        }

        return {};
    }

    [[nodiscard]] auto audioMixer() -> const std::unique_ptr<audio_mixer::AudioMixer<float>>& {
        return m_audioMixer;
    }

    [[nodiscard]] auto startRecording(const audio_format::AudioFormat format) -> std::expected<void, std::string> {
        if (not m_audioLibraryWrapper->isStreamRunning()) {
            return std::unexpected { "Audio stream is not running" };
        }

        stopRecording();

        try {
            const auto& inputParams { dynamic_cast<const audio_stream_params::InputAudioStreamParams&>(*m_audioStreamParams) };

            std::vector<std::string> fileNames {};
            std::vector<audio_mixer::ChannelRouting> routingList {};

            for (audio_device::ChannelCount_t channel { 0 }; channel < inputParams.m_numberOfInputChannels; ++channel) {
                fileNames.emplace_back(m_audioMixer->inputName(channel));
                routingList.push_back(m_audioMixer->inputRouting(channel).first);
            }

            if (not m_inputRingAudioBuffer) {
                return std::unexpected { "Input ring audio buffer is null" };
            }

            if (auto inputAudioRecorder { audio_recorder::makeAudioRecorder(m_audioStreamParams->m_sampleRate, format, fileNames, routingList) }; not inputAudioRecorder.has_value()) {
                return std::unexpected { std::format("Could not create input audio recorder: {}", inputAudioRecorder.error()) };
            } else {
                m_inputRecorder.swap(inputAudioRecorder.value());
            }
        } catch ([[maybe_unused]] const std::bad_cast&) {}

        try {
            const auto& outputParams { dynamic_cast<const audio_stream_params::OutputAudioStreamParams&>(*m_audioStreamParams) };

            std::vector<std::string> fileNames {};
            std::vector<audio_mixer::ChannelRouting> routingList {};

            for (audio_device::ChannelCount_t channel { 0 }; channel < outputParams.m_numberOfOutputChannels / 2; ++channel) {
                fileNames.emplace_back(m_audioMixer->outputName(channel));
                routingList.push_back(m_audioMixer->outputRouting(channel));
            }

            if (not m_outputRingAudioBuffer) {
                return std::unexpected { "Output ring audio buffer is null" };
            }

            if (auto outputAudioRecorder { audio_recorder::makeAudioRecorder(m_audioStreamParams->m_sampleRate, format, fileNames, routingList) }; not outputAudioRecorder.has_value()) {
                return std::unexpected { std::format("Could not create output audio recorder: {}", outputAudioRecorder.error()) };
            } else {
                m_outputRecorder.swap(outputAudioRecorder.value());
            }
        } catch ([[maybe_unused]] const std::bad_cast&) {}

        m_isRecording.store(true, std::memory_order_release);
        return {};
    }

    auto stopRecording() -> void {
        m_isRecording.store(false, std::memory_order_release);

        m_inputRecorder.reset();
        m_outputRecorder.reset();
    }

    [[nodiscard]] auto write() const -> bool {
        if (not m_isRecording.load(std::memory_order_acquire)) {
            return false;
        }

        if (m_inputRecorder) {
            const auto audioBuffer { audio_buffer::makeAudioBuffer<float>(0, 0) };

            if (not m_inputRingAudioBuffer->dequeue(*audioBuffer)) {
                return false;
            }

            if (not m_inputRecorder->write(*audioBuffer)) {
                return false;
            }
        }

        if (m_outputRecorder) {
             const auto audioBuffer { audio_buffer::makeAudioBuffer<float>(0, 0) };

            if (not m_outputRingAudioBuffer->dequeue(*audioBuffer)) {
                return false;
            }

            if (not m_outputRecorder->write(*audioBuffer)) {
                return false;
            }
        }

        return true;
    }

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

        const auto isRecording { m_isRecording.load(std::memory_order_acquire) };

        if (m_inputRingAudioBuffer && isRecording) {
            std::ignore = m_inputRingAudioBuffer->enqueue(inputBuffer);
        }

        for (auto channel { audio_device::ChannelCount_t { 0 } }; channel < inputBuffer.numberOfChannels(); ++channel) {
            const auto [min, max, rms] { inputBuffer.computeStats(channel) };

            auto& stats { *m_inputAudioStats[channel] };

            stats.min(min);
            stats.max(max);
            stats.rms(rms);
        }

        processInput(inputBuffer, outputBuffer);

        if (m_outputRingAudioBuffer && isRecording) {
            std::ignore = m_outputRingAudioBuffer->enqueue(outputBuffer);
        }

        for (auto channel { audio_device::ChannelCount_t { 0 } }; channel < outputBuffer.numberOfChannels(); ++channel) {
            const auto [min, max, rms] { outputBuffer.computeStats(channel) };

            auto& stats { *m_outputAudioStats[channel] };

            stats.min(min);
            stats.max(max);
            stats.rms(rms);
        }

        processOutput(outputBuffer);
    }

    static constexpr audio_device::SampleRate_t m_sampleRate { 48000 };
    static constexpr audio_format::AudioFormat m_format { audio_format::AudioFormat::Float32 };
    static constexpr audio_stream_params::PeriodSize_t m_periodSize { 3 };
    static constexpr std::array<const audio_stream_params::BufferLength_t, 5> m_allowedBufferLengths { 1024, 2048, 4096, 8192, 16384 };

    std::unique_ptr<audio_mixer::AudioMixer<float>> m_audioMixer;
    std::unique_ptr<audio_library_wrapper::AudioLibraryWrapper> m_audioLibraryWrapper;
    std::vector<std::unique_ptr<const audio_device::AudioDevice>> m_audioDevices;
    std::unique_ptr<audio_stream_params::AudioStreamParams> m_audioStreamParams;
    std::unique_ptr<audio_buffer::AudioBuffer<float>> m_processedInputBuffer;
    std::unique_ptr<ring_audio_buffer::RingAudioBuffer<float>> m_inputRingAudioBuffer;
    std::unique_ptr<ring_audio_buffer::RingAudioBuffer<float>> m_outputRingAudioBuffer;
    std::atomic_bool m_isRecording;
    std::unique_ptr<audio_recorder::AudioRecorder> m_inputRecorder;
    std::unique_ptr<audio_recorder::AudioRecorder> m_outputRecorder;
    std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> m_inputAudioStats;
    std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> m_outputAudioStats;

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
