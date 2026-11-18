module audio_engine;

namespace audio_engine {

AudioEngine::AudioEngine(const audio_library_wrapper::LogCallback& logCallback,
                         std::unique_ptr<audio_library_wrapper::AudioLibraryWrapper> audioLibraryWrapper)
 :  m_audioMixer { nullptr },
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
    m_audioLibraryWrapper { std::move(audioLibraryWrapper) },
    m_logCallback { logCallback },
    m_audioCallback { [this] (const audio_buffer::AudioBuffer<float>& inputBuffer, audio_buffer::AudioBuffer<float>& outputBuffer) {
        process(inputBuffer, outputBuffer);
    } } {}

auto AudioEngine::audioDriver(const audio_driver::AudioDriver newAudioDriver) -> std::expected<void, std::string> {
    auto audioLibraryWrapper { audio_library_wrapper::makeAudioLibraryWrapper<audio_library_wrapper::MiniaudioLibraryWrapper>(m_logCallback, newAudioDriver) };

    if (not audioLibraryWrapper.has_value()) {
        return std::unexpected { audioLibraryWrapper.error() };
    }

    m_audioLibraryWrapper.swap(audioLibraryWrapper.value());
    return {};
}

auto AudioEngine::probeDevices() -> std::expected<void, std::string> {
    auto audioDevicesResult { m_audioLibraryWrapper->probeDevices() };

    if (not audioDevicesResult.has_value()) {
        return std::unexpected { std::format("Could not probe devices: {}\n", audioDevicesResult.error()) };
    }

    m_audioDevices.swap(audioDevicesResult.value());
    return {};
}

auto AudioEngine::defaultInputAudioDeviceName() const -> std::expected<std::string, std::string> {
    return getDefaultAudioDevice(audio_device::AudioDeviceType::Input).transform([] (const auto&& deviceItr) {return (*deviceItr)->m_deviceName; });
}

auto AudioEngine::defaultOutputAudioDeviceName() const -> std::expected<std::string, std::string> {
    return getDefaultAudioDevice(audio_device::AudioDeviceType::Output).transform([] (const auto&& deviceItr) {return (*deviceItr)->m_deviceName; });
}

auto AudioEngine::inputAudioDeviceSummaryList() -> std::expected<std::vector<audio_device::AudioDeviceSummary>, std::string> {
    return audioDeviceSummaryList(audio_device::AudioDeviceType::Input);
}

auto AudioEngine::outputAudioDeviceSummaryList() -> std::expected<std::vector<audio_device::AudioDeviceSummary>, std::string> {
    return audioDeviceSummaryList(audio_device::AudioDeviceType::Output);
}

auto AudioEngine::startStream(std::optional<std::string> inputDeviceName,
                              std::optional<std::string> outputDeviceName,
                              std::optional<std::string> loopbackDeviceName,
                              const audio_stream_params::BufferLength_t bufferLength,
                              const audio_device::SampleRate_t sampleRate) -> std::expected<void, std::string> {
    if (not isSampleRateAllowed(sampleRate)) {
        return std::unexpected { std::format("Sample rate {} is not allowed", sampleRate) };
    }

    if (not isBufferLengthAllowed(bufferLength)) {
        return std::unexpected { std::format("Buffer length {} is not allowed", bufferLength) };
    }

    const auto inputResult { getDeviceSelection(inputDeviceName, audio_device::AudioDeviceType::Input, "input") };

    if (not inputResult.has_value()) {
        return std::unexpected { inputResult.error() };
    }

    const auto outputResult { getDeviceSelection(outputDeviceName, audio_device::AudioDeviceType::Output, "output") };

    if (not outputResult.has_value()) {
        return std::unexpected { outputResult.error() };
    }

    const auto loopbackResult { getDeviceSelection(loopbackDeviceName, audio_device::AudioDeviceType::Output, "loopback") };

    if (not loopbackResult.has_value()) {
        return std::unexpected { loopbackResult.error() };
    }

    const auto& input { inputResult.value() };
    const auto& output { outputResult.value() };
    const auto& loopback { loopbackResult.value() };

    auto streamParamsResult { audio_stream_params::makeAudioStreamParams(sampleRate, m_format, bufferLength, m_periodSize, input, output, loopback) };

    if (not streamParamsResult.has_value()) {
        return std::unexpected { std::format("Error creating stream params: {}", streamParamsResult.error()) };
    }

    const auto inputChannelCount { input.transform([] (const auto& selection) { return selection.m_channelCount; }) };
    const auto outputChannelCount { [&] () -> std::optional<audio_device::ChannelCount_t> {
        if (output.has_value()) return output->m_channelCount;
        if (loopback.has_value()) return loopback->m_channelCount;
        return std::nullopt;
    } () };

    return replaceStream(std::move(streamParamsResult).value(), inputChannelCount, outputChannelCount, bufferLength);
}

auto AudioEngine::startRecording(const audio_format::AudioFormat format, std::filesystem::path path) -> std::expected<void, std::string> {
    if (not m_audioLibraryWrapper->isStreamRunning()) {
        return std::unexpected { std::string { "Audio stream is not running" } };
    }

    if (m_isRecording.load(std::memory_order_acquire)) {
        return std::unexpected { std::string { "Recording already started" } };
    }

    try {
        const auto& inputParams { dynamic_cast<const audio_stream_params::InputAudioStreamParams&>(*m_audioStreamParams) };

        if (auto result { createInputRecorder(inputParams.m_numberOfInputChannels, format, path) }; not result.has_value()) {
            return result;
        }
    } catch ([[maybe_unused]] const std::bad_cast&) {}

    try {
        const auto& outputParams { dynamic_cast<const audio_stream_params::OutputAudioStreamParams&>(*m_audioStreamParams) };

        if (auto result { createOutputRecorder(outputParams.m_numberOfOutputChannels, format, path, "output") }; not result.has_value()) {
            return result;
        }
    } catch ([[maybe_unused]] const std::bad_cast&) {}

    try {
        const auto& loopbackParams { dynamic_cast<const audio_stream_params::LoopbackAudioStreamParams&>(*m_audioStreamParams) };

        if (auto result { createOutputRecorder(loopbackParams.m_numberOfLoopbackChannels, format, path, "loopback") }; not result.has_value()) {
            return result;
        }
    } catch ([[maybe_unused]] const std::bad_cast&) {}

    if (not m_inputRecorder and not m_outputRecorder) {
        return std::unexpected { "No routing selected" };
    }

    m_isRecording.store(true, std::memory_order_release);
    return {};
}

auto AudioEngine::finalizeRecording() -> void {
    m_inputRecorder.reset();
    m_outputRecorder.reset();

    if (m_inputRingAudioBuffer) m_inputRingAudioBuffer->reset();
    if (m_outputRingAudioBuffer) m_outputRingAudioBuffer->reset();
}

auto AudioEngine::write() const -> bool {
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

auto AudioEngine::inputLevels(std::span<float> inputLevels) const -> void {
    std::ranges::transform(m_inputAudioStats, std::ranges::begin(inputLevels), [] (const auto& stats) { return stats->max(); });
}

auto AudioEngine::outputLevels(std::span<float> outputLevels) const -> void {
    std::ranges::transform(m_outputAudioStats, std::ranges::begin(outputLevels), [] (const auto& stats) { return stats->max(); });
}

auto AudioEngine::getAudioDevice(const std::string& deviceName, const audio_device::AudioDeviceType deviceType) const
                -> std::expected<std::ranges::borrowed_iterator_t<const std::vector<std::unique_ptr<const audio_device::AudioDevice>> &>, std::string> {
    auto deviceItr { std::ranges::find_if(m_audioDevices, [&deviceName, &deviceType] (const auto& currentDevice) { return currentDevice->m_deviceName == deviceName and currentDevice->m_type == deviceType; } ) };

    if (deviceItr == std::ranges::end(m_audioDevices)) {
        return std::unexpected { "Audio device not found" };
    }

    return deviceItr;
}

auto AudioEngine::isSampleRateAllowed(const audio_device::SampleRate_t sampleRate) -> bool {
    return std::ranges::find(m_allowedSampleRates, sampleRate) != std::ranges::end(m_allowedSampleRates);
}

auto AudioEngine::isBufferLengthAllowed(const audio_stream_params::BufferLength_t bufferLength) -> bool {
    return std::ranges::find(m_allowedBufferLengths, bufferLength) != std::ranges::end(m_allowedBufferLengths);
}

auto AudioEngine::openStream() const -> bool {
    if (m_audioStreamParams == nullptr)
        return false;

    return m_audioLibraryWrapper->openStream(*m_audioStreamParams, m_audioCallback) && m_audioLibraryWrapper->startStream();
}

auto AudioEngine::closeStream() const -> bool {
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

auto AudioEngine::getDefaultAudioDevice(const audio_device::AudioDeviceType deviceType) const
                -> std::expected<std::ranges::borrowed_iterator_t<const std::vector<std::unique_ptr<const audio_device::AudioDevice>> &>, std::string> {
    auto deviceItr { std::ranges::find_if(m_audioDevices, [&deviceType] (const auto& currentDevice) { return currentDevice->m_type == deviceType && currentDevice->m_isDefault; }) };

    if (deviceItr == std::ranges::end(m_audioDevices)) {
        return std::unexpected { "Audio device not found" };
    }

    return deviceItr;
}

auto AudioEngine::audioDeviceSummaryList(const audio_device::AudioDeviceType deviceType) -> std::expected<std::vector<audio_device::AudioDeviceSummary>, std::string> {
    if (auto probeResult { probeDevices() }; not probeResult.has_value()) {
        return std::unexpected { std::move(probeResult).error() };
    }

    std::vector<audio_device::AudioDeviceSummary> audioDeviceSummaryList {};
    for (auto& device : m_audioDevices
                        | std::views::filter([&deviceType] (const auto& audioDevice) { return audioDevice->m_type == deviceType; })) {
        audioDeviceSummaryList.emplace_back(device->m_deviceName, device->m_nativeDataFormats[0].m_maxChannels);
    }

    return audioDeviceSummaryList;
}

auto AudioEngine::processInput(const audio_buffer::AudioBuffer<float>& inputBuffer, const audio_buffer::AudioBuffer<float>& outputBuffer) const -> void {
    const auto inputChannels { inputBuffer.numberOfChannels() };
    for (auto channel { audio_device::ChannelCount_t { 0 } };  channel < inputChannels; ++channel) {
        const auto [inputRouting, outputRouting] { m_audioMixer->inputRouting(channel) };

        if (not inputRouting.isMono() and not inputRouting.isStereo())
            continue;

        auto inputBufferView { inputBuffer.view(inputRouting.m_leftMono.value(), inputRouting.m_right) };
        auto processedInputBufferView { m_processedInputBuffer->view(channel * 2,
            inputRouting.isStereo() ? std::optional { channel * 2 + 1 } : std::nullopt) };

        m_audioMixer->processInput(inputBufferView, processedInputBufferView, channel);

        if (not outputRouting.isMono() and not outputRouting.isStereo())
            continue;

        auto outputBufferView { outputBuffer.view(outputRouting.m_leftMono.value(), outputRouting.m_right) };
        m_audioMixer->mix(processedInputBufferView, outputBufferView);
    }
}

auto AudioEngine::processOutput(audio_buffer::AudioBuffer<float>& outputBuffer) const -> void {
    const auto outputChannels { outputBuffer.numberOfChannels() };
    // Output mixer is always stereo
    for (auto channel { audio_device::ChannelCount_t { 0 } }; channel < outputChannels / 2; ++channel) {
        const auto outputRouting { m_audioMixer->outputRouting(channel) };

        if (not outputRouting.isMono() and not outputRouting.isStereo()) {
            // A mixer channel is stereo, so if we don't have a routing for a stereo channel,
            // we clear channel and next channel
            outputBuffer.clear(channel * 2);
            outputBuffer.clear((channel * 2) + 1);
            continue;
        }

        // Input routing is same as output for output channels, checking output.
        auto outputBufferView { outputBuffer.view(outputRouting.m_leftMono.value(), outputRouting.m_right) };

        // In place processing
        m_audioMixer->processOutput(outputBufferView,
            outputBufferView,
            channel);
    }
}

auto AudioEngine::process(const audio_buffer::AudioBuffer<float>& inputBuffer, audio_buffer::AudioBuffer<float>& outputBuffer) const -> void {
    if (m_processedInputBuffer)
        m_processedInputBuffer->clear();

    const auto isRecording { m_isRecording.load(std::memory_order_acquire) };

    if (m_inputRingAudioBuffer && isRecording) {
        std::ignore = m_inputRingAudioBuffer->enqueue(inputBuffer);
    }

    updateAudioStats(inputBuffer, m_inputAudioStats);

    processInput(inputBuffer, outputBuffer);
    processOutput(outputBuffer);

    if (m_outputRingAudioBuffer && isRecording) {
        std::ignore = m_outputRingAudioBuffer->enqueue(outputBuffer);
    }

    updateAudioStats(outputBuffer, m_outputAudioStats);
}

auto AudioEngine::getDeviceSelection(const std::optional<std::string>& deviceName,
                                     const audio_device::AudioDeviceType deviceType,
                                     const std::string_view label) const
                -> std::expected<std::optional<audio_stream_params::DeviceSelection>, std::string> {
    if (not deviceName.has_value()) {
        return std::nullopt;
    }

    const auto deviceItrResult { getAudioDevice(deviceName.value(), deviceType) };

    if (not deviceItrResult.has_value()) {
        return std::unexpected { std::format("Could not find {} device {}: {}", label, deviceName.value(), deviceItrResult.error()) };
    }

    const auto& device { *deviceItrResult.value()->get() };

    return std::optional<audio_stream_params::DeviceSelection> { std::in_place, device.m_deviceId, device.m_nativeDataFormats[0].m_maxChannels };
}

auto AudioEngine::replaceStream(std::unique_ptr<audio_stream_params::AudioStreamParams> streamParams,
                                const std::optional<audio_device::ChannelCount_t> inputChannelCount,
                                const std::optional<audio_device::ChannelCount_t> outputChannelCount,
                                const audio_stream_params::BufferLength_t bufferLength) -> std::expected<void, std::string> {
    // Everything that can fail is built before the running stream is touched
    std::unique_ptr<audio_buffer::AudioBuffer<float>> processedInputBuffer { nullptr };

    if (inputChannelCount.has_value()) {
        processedInputBuffer = audio_buffer::makeAudioBuffer<float>(inputChannelCount.value() * 2, bufferLength);
    }

    // Output audio is always stereo, so for each couple of channels we get 1 mixer channel
    auto audioMixerResult { audio_mixer::makeAudioMixer<float>(inputChannelCount.has_value()? inputChannelCount.value() : 0, outputChannelCount.has_value()? outputChannelCount.value() / 2 : 0) };

    if (not audioMixerResult.has_value()) {
        return std::unexpected { std::format("Error creating audio mixer: {}", audioMixerResult.error()) };
    }

    auto inputRingAudioBuffer { createRingAudioBuffer(inputChannelCount, *streamParams, "input") };

    if (not inputRingAudioBuffer.has_value()) {
        return std::unexpected { std::move(inputRingAudioBuffer).error() };
    }

    auto outputRingAudioBuffer { createRingAudioBuffer(outputChannelCount, *streamParams, "output") };

    if (not outputRingAudioBuffer.has_value()) {
        return std::unexpected { std::move(outputRingAudioBuffer).error() };
    }

    std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> inputAudioStats {};
    std::ranges::generate_n(std::back_inserter(inputAudioStats), inputChannelCount.value_or(0), [] () { return audio_buffer::makeAudioStats<float>(); });

    std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>> outputAudioStats {};
    std::ranges::generate_n(std::back_inserter(outputAudioStats), outputChannelCount.value_or(0), [] () { return audio_buffer::makeAudioStats<float>(); });

    if (not closeStream()) {
        return std::unexpected { "Could not close running stream" };
    }

    // Audio thread stops here
    m_audioStreamParams.swap(streamParams);

    m_processedInputBuffer.swap(processedInputBuffer);

    if (m_processedInputBuffer) {
        m_processedInputBuffer->clear();
    }

    m_audioMixer.swap(audioMixerResult.value());

    m_inputRingAudioBuffer.swap(inputRingAudioBuffer.value());
    m_outputRingAudioBuffer.swap(outputRingAudioBuffer.value());

    m_inputAudioStats.swap(inputAudioStats);
    m_outputAudioStats.swap(outputAudioStats);

    if (not openStream()) {
        return std::unexpected { "Could not open stream" };
    }

    return {};
}

auto AudioEngine::createRingAudioBuffer(const std::optional<audio_device::ChannelCount_t> channelCount,
                                        const audio_stream_params::AudioStreamParams& streamParams,
                                        const std::string_view label)
                -> std::expected<std::unique_ptr<ring_audio_buffer::RingAudioBuffer<float>>, std::string> {
    if (not channelCount.has_value()) {
        return nullptr;
    }

    // 10 seconds of audio in ring buffer
    auto ringAudioBufferResult { ring_audio_buffer::makeRingAudioBuffer<float>(channelCount.value(), streamParams.m_sampleRate * 10) };

    if (not ringAudioBufferResult.has_value()) {
        return std::unexpected { std::format("Error creating {} ring audio buffer: {}", label, ringAudioBufferResult.error()) };
    }

    return std::move(ringAudioBufferResult).value();
}

auto AudioEngine::createInputRecorder(const audio_device::ChannelCount_t channelCount,
                                      const audio_format::AudioFormat format,
                                      const std::filesystem::path& path) -> std::expected<void, std::string> {
    std::vector<std::string> fileNames {};
    std::vector<audio_mixer::ChannelRouting> routingList {};

    for (audio_device::ChannelCount_t channel { 0 }; channel < channelCount; ++channel) {
        fileNames.emplace_back(m_audioMixer->inputName(channel));
        routingList.push_back(m_audioMixer->inputRouting(channel).first);
    }

    if (not m_inputRingAudioBuffer) {
        return std::unexpected { "Input ring audio buffer is null" };
    }

    const auto isDefaultRouting { std::ranges::all_of(routingList, [] (const auto& routing) { return not routing.isMono() and not routing.isStereo(); }) };

    if (isDefaultRouting) {
        return {};
    }

    auto inputAudioRecorder { audio_recorder::makeAudioRecorder(m_audioStreamParams->m_sampleRate, format, path, fileNames, routingList) };

    if (not inputAudioRecorder.has_value()) {
        return std::unexpected { std::format("Could not create input audio recorder: {}", inputAudioRecorder.error()) };
    }

    m_inputRecorder.swap(inputAudioRecorder.value());
    return {};
}

auto AudioEngine::createOutputRecorder(const audio_device::ChannelCount_t channelCount,
                                       const audio_format::AudioFormat format,
                                       const std::filesystem::path& path,
                                       const std::string_view label) -> std::expected<void, std::string> {
    std::vector<std::string> fileNames {};
    std::vector<audio_mixer::ChannelRouting> routingList {};

    // Output audio is always stereo, so for each couple of channels we get 1 mixer channel
    for (audio_device::ChannelCount_t channel { 0 }; channel < channelCount / 2; ++channel) {
        fileNames.emplace_back(m_audioMixer->outputName(channel));
        routingList.push_back(m_audioMixer->outputRouting(channel));
    }

    if (not m_outputRingAudioBuffer) {
        return std::unexpected { "Output ring audio buffer is null" };
    }

    const auto isDefaultRouting { std::ranges::all_of(routingList, [] (const auto& routing) { return not routing.isMono() and not routing.isStereo(); }) };

    if (isDefaultRouting) {
        return {};
    }

    auto outputAudioRecorder { audio_recorder::makeAudioRecorder(m_audioStreamParams->m_sampleRate, format, path, fileNames, routingList) };

    if (not outputAudioRecorder.has_value()) {
        return std::unexpected { std::format("Could not create {} audio recorder: {}", label, outputAudioRecorder.error()) };
    }

    m_outputRecorder.swap(outputAudioRecorder.value());
    return {};
}

auto AudioEngine::updateAudioStats(const audio_buffer::AudioBuffer<float>& buffer,
                                   const std::vector<std::unique_ptr<audio_buffer::AudioStats<float>>>& audioStats) -> void {
    for (auto channel { audio_device::ChannelCount_t { 0 } }; channel < buffer.numberOfChannels(); ++channel) {
        const auto [min, max, rms] { buffer.computeStats(channel) };

        auto& stats { *audioStats[channel] };

        stats.min(min);
        stats.max(max);
        stats.rms(rms);
    }
}

auto makeAudioEngine(const audio_library_wrapper::LogCallback& logCallback, const audio_driver::AudioDriver audioDriver)
                -> std::expected<std::unique_ptr<AudioEngine>, std::string> {
    try {
        auto audioLibraryWrapper { audio_library_wrapper::makeAudioLibraryWrapper<audio_library_wrapper::MiniaudioLibraryWrapper>(logCallback, audioDriver) };

        if (not audioLibraryWrapper.has_value()) {
            return std::unexpected { std::format("Error setting audio driver: {}", audioLibraryWrapper.error()) };
        }

        return std::make_unique<AudioEngine>(logCallback, std::move(audioLibraryWrapper).value());
    } catch (const std::exception& e) {
        return std::unexpected { std::string { e.what() } };
    }
}

}
