module audio_engine_manager;

namespace ae = audio_engine;
namespace ats = async_task_scheduler;

namespace managers {

AudioEngineManager::AudioEngineManager(ats::AsyncTaskScheduler& scheduler, const ae::audio_library_wrapper::LogCallback &logCallback)
  : TaskManager { scheduler },
    m_taskMutex {},
    m_logCallback { logCallback },
    m_audioEngine { nullptr },
    m_writeTaskDependency { std::nullopt } {
    if (auto audioEngineResult { ae::makeAudioEngine<ae::audio_library_wrapper::MiniaudioLibraryWrapper>(logCallback) }; not audioEngineResult.has_value()) {
        throw std::runtime_error { std::string { std::format("Error creating audio engine: {}",  audioEngineResult.error()) } };
    } else {
        m_audioEngine.swap(audioEngineResult.value());
    }
}

AudioEngineManager::~AudioEngineManager() {
    stopRecording();
}

auto AudioEngineManager::allowedBufferLengths() -> decltype(ae::AudioEngine<ae::audio_library_wrapper::MiniaudioLibraryWrapper>::allowedBufferLengths())& {
    return ae::AudioEngine<ae::audio_library_wrapper::MiniaudioLibraryWrapper>::allowedBufferLengths();
}

auto AudioEngineManager::audioDriver() -> ats::Result<std::expected<ae::audio_driver::AudioDriver, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->audioDriver();
    }) };

    auto result { task->result() };
    enqueueTasks(std::move(task));

    return result;
}

auto AudioEngineManager::audioDriver(const ae::audio_driver::AudioDriver newAudioDriver) -> ats::Result<std::expected<void, std::string>> {
    stopRecording();

    auto task { ats::makeAtomicTask([this, newAudioDriver] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->audioDriver(newAudioDriver);
    }) };

    auto result { task->result() };
    enqueueTasks(std::move(task));

    return result;
}

auto AudioEngineManager::probeDevices() -> ats::Result<std::expected<void, std::string> > {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->probeDevices();
    }) };

    auto result { task->result() };
    enqueueTasks(std::move(task));

    return result;
}

auto AudioEngineManager::defaultInputAudioDeviceName() -> ats::Result<std::expected<std::string, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->defaultInputAudioDeviceName();
    }) };

    auto result { task->result() };
    enqueueTasks(std::move(task));

    return result;
}

auto AudioEngineManager::defaultOutputAudioDeviceName() -> ats::Result<std::expected<std::string, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->defaultOutputAudioDeviceName();
    }) };

    auto result { task->result() };
    enqueueTasks(std::move(task));

    return result;
}

auto AudioEngineManager::inputAudioDeviceSummaryList() -> ats::Result<std::expected<std::vector<ae::audio_device::AudioDeviceSummary>, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->inputAudioDeviceSummaryList();
    }) };

    auto result { task->result() };
    enqueueTasks(std::move(task));

    return result;
}

auto AudioEngineManager::outputAudioDeviceSummaryList() -> ats::Result<std::expected<std::vector<ae::audio_device::AudioDeviceSummary>, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->outputAudioDeviceSummaryList();
    }) };

    auto result { task->result() };
    enqueueTasks(std::move(task));

    return result;
}

auto AudioEngineManager::startStream(std::optional<std::string> inputDeviceName,
                                     std::optional<std::string> outputDeviceName, ae::audio_stream_params::BufferLength_t bufferLength) -> ats::Result<std::expected<void, std::string>> {
    stopRecording();

    auto task { ats::makeAtomicTask([this, inputDeviceName = std::move(inputDeviceName), outputDeviceName = std::move(outputDeviceName), bufferLength] () mutable {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->startStream(std::move(inputDeviceName), std::move(outputDeviceName), bufferLength);
    }) };

    auto result { task->result() };
    enqueueTasks(std::move(task));

    return result;
}

ats::ResumableTask<void> write(const std::chrono::milliseconds& triggerInterval, std::mutex& mutex, const std::unique_ptr<ae::AudioEngine<ae::audio_library_wrapper::MiniaudioLibraryWrapper>>& audioEngine) {
    std::unique_lock writeLock { mutex, std::defer_lock };
    auto writeResult { false };
    auto triggerTime { std::chrono::steady_clock::now() + triggerInterval };

    while (true) {
        // This needs to be placed before the write operation otherwise the audio buffer to write
        // is empty. Need to get some samples in the audio buffer
        if (std::chrono::steady_clock::now() < triggerTime) co_await std::suspend_always {};

        writeLock.lock();
        writeResult = audioEngine->write();
        writeLock.unlock();

        if (not writeResult) co_return;

        triggerTime += triggerInterval;
        co_await std::suspend_always {};
    }
}

auto AudioEngineManager::startRecording(const ae::audio_format::AudioFormat format) -> std::expected<void, std::string> {
    stopRecording();

    auto startRecordingTask { ats::makeAtomicTask([this, format] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->startRecording(format);
    }) };

    const auto startRecordingResult { startRecordingTask->result() };
    enqueueTasks(std::move(startRecordingTask));

    if (auto taskResult { startRecordingResult.get() }; not taskResult.has_value()) {
        return taskResult;
    }

    std::unique_ptr<ats::AsyncTask> writeTask { ats::makeResumableTask<void>(write(std::chrono::milliseconds { 500 }, m_taskMutex, m_audioEngine)) };
    m_writeTaskDependency = writeTask->dependency();

    enqueueTasks(std::move(writeTask));

    return {};
}

auto AudioEngineManager::stopRecording() -> void {
    auto stopRecordingTask { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        m_audioEngine->stopRecording();
    }) };

    const auto stopRecordingTaskDependency { stopRecordingTask->dependency() };
    enqueueTasks(std::move(stopRecordingTask));

    stopRecordingTaskDependency.wait();

    if (m_writeTaskDependency.has_value()) {
        m_writeTaskDependency->wait();
    }

    m_writeTaskDependency.reset();

    auto finalizeRecordingTask { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        m_audioEngine->finalizeRecording();
    }) };

    const auto finalizeRecordingTaskDependency { finalizeRecordingTask->dependency() };
    enqueueTasks(std::move(finalizeRecordingTask));

    finalizeRecordingTaskDependency.wait();
}

auto AudioEngineManager::inputChannelName(std::string channelName, const ae::audio_device::ChannelCount_t channelCount) -> ats::Result<void> {
    auto inputNameTask { ats::makeAtomicTask([this, name = std::move(channelName), channelCount] () {
        std::lock_guard lock { m_taskMutex };
        m_audioEngine->audioMixer()->inputName(name, channelCount);
    }) };

    const auto inputChannelNameResult { inputNameTask->result() };
    enqueueTasks(std::move(inputNameTask));

    return inputChannelNameResult;
}

auto AudioEngineManager::inputChannelName(const ae::audio_device::ChannelCount_t channelCount) -> ats::Result<std::string> {
    auto inputNameTask { ats::makeAtomicTask([this, channelCount] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->audioMixer()->inputName(channelCount);
    }) };

    const auto inputNameResult { inputNameTask->result() };
    enqueueTasks(std::move(inputNameTask));

    return inputNameResult;
}

auto AudioEngineManager::outputChannelName(std::string channelName, const ae::audio_device::ChannelCount_t channelCount) -> ats::Result<void> {
    auto outputNameTask { ats::makeAtomicTask([this, name = std::move(channelName), channelCount] () {
        std::lock_guard lock { m_taskMutex };
        m_audioEngine->audioMixer()->outputName(name, channelCount);
    }) };

    const auto outputNameResult { outputNameTask->result() };
    enqueueTasks(std::move(outputNameTask));

    return outputNameResult;
}

auto AudioEngineManager::outputChannelName(const ae::audio_device::ChannelCount_t channelCount) -> ats::Result<std::string> {
    auto outputNameTask { ats::makeAtomicTask([this, channelCount] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->audioMixer()->outputName(channelCount);
    }) };

    const auto outputNameResult { outputNameTask->result() };
    enqueueTasks(std::move(outputNameTask));

    return outputNameResult;
}

auto AudioEngineManager::inputChannelGain(const float gain, const ae::audio_device::ChannelCount_t channelCount) const -> void {
    m_audioEngine->audioMixer()->inputGain(gain, channelCount);
}

auto AudioEngineManager::inputChannelGain(const ae::audio_device::ChannelCount_t channelCount) const -> float {
    return m_audioEngine->audioMixer()->inputGain(channelCount);
}

auto AudioEngineManager::outputChannelGain(const float gain, const ae::audio_device::ChannelCount_t channelCount) const -> void {
    m_audioEngine->audioMixer()->outputGain(gain, channelCount);
}

auto AudioEngineManager::outputChannelGain(const ae::audio_device::ChannelCount_t channelCount) const -> float {
    return m_audioEngine->audioMixer()->outputGain(channelCount);
}

auto AudioEngineManager::inputChannelRouting(const std::pair<ae::audio_mixer::ChannelRouting, ae::audio_mixer::ChannelRouting> routing,
    const ae::audio_device::ChannelCount_t channelCount) const -> void {
    m_audioEngine->audioMixer()->inputRouting(routing, channelCount);
}

auto AudioEngineManager::inputChannelRouting(const ae::audio_device::ChannelCount_t channelCount) const ->
    std::pair<ae::audio_mixer::ChannelRouting, ae::audio_mixer::ChannelRouting> {
    return m_audioEngine->audioMixer()->inputRouting(channelCount);
}

auto AudioEngineManager::outputChannelRouting(const ae::audio_mixer::ChannelRouting routing, const ae::audio_device::ChannelCount_t channelCount) const -> void {
    m_audioEngine->audioMixer()->outputRouting(routing, channelCount);
}

auto AudioEngineManager::outputChannelRouting(const ae::audio_device::ChannelCount_t channelCount) const -> ae::audio_mixer::ChannelRouting {
    return m_audioEngine->audioMixer()->outputRouting(channelCount);
}

auto AudioEngineManager::inputLevels(std::span<float> inputLevels) const -> void {
    m_audioEngine->inputLevels(inputLevels);
}

auto AudioEngineManager::outputLevels(std::span<float> outputLevels) const -> void {
    m_audioEngine->outputLevels(outputLevels);
}

auto makeAudioEngineManager(ats::AsyncTaskScheduler& scheduler,
                            const ae::audio_library_wrapper::LogCallback& logCallback) -> std::expected<std::unique_ptr<AudioEngineManager>, std::string> {
    try {
        return std::make_unique<AudioEngineManager>(scheduler, logCallback);
    } catch (const std::exception& e) {
        return std::unexpected { std::string { e.what() } };
    }
}

}
