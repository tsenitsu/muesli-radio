module audio_engine_manager;

import logger;
import utils;

namespace ae = audio_engine;
namespace ats = async_task_scheduler;

namespace managers {

#ifdef _WIN32
// The engine is created on the thread that constructs the manager, so it must not start on ASIO
static_assert(ae::audio_driver::availableAudioDrivers[0] != ae::audio_driver::AudioDriver::Asio,
              "The first available audio driver must not be ASIO");
#endif

AudioEngineManager::AudioEngineManager(ats::AsyncTaskScheduler& scheduler, LoggerManager& loggerManager)
  : TaskManager { scheduler },
    m_taskMutex {},
    m_loggerManager { loggerManager },
    m_logCallback { [this] (std::optional<logger::LogEntry> entry) { m_loggerManager.enqueueLogEntry(std::move(entry)); } },
    m_audioEngine { nullptr },
    m_writeTaskDependency { std::nullopt } {
    if (auto audioEngineResult { ae::makeAudioEngine(m_logCallback) }; not audioEngineResult.has_value()) {
        throw std::runtime_error { std::string { std::format("Error creating audio engine: {}", audioEngineResult.error()) } };
    } else {
        m_audioEngine.swap(audioEngineResult.value());
    }
}

AudioEngineManager::~AudioEngineManager() {
    stopRecording();
    waitForAllTasks();

#ifdef _WIN32
    // An ASIO driver must be released on the thread that loaded it, not on the one destroying the manager
    if (isAsioInvolved()) {
        auto releaseEngineTask { ats::makeAtomicTask([this] () {
            std::lock_guard lock { m_taskMutex };
            m_audioEngine.reset();
        }) };

        const auto releaseEngineTaskDependency { releaseEngineTask->dependency() };
        enqueueTasks(kAsioThreadId, std::move(releaseEngineTask));

        releaseEngineTaskDependency.wait();
    }
#endif
}

auto AudioEngineManager::allowedSampleRates() -> decltype(ae::AudioEngine::allowedSampleRates())& {
    return ae::AudioEngine::allowedSampleRates();
}

auto AudioEngineManager::allowedBufferLengths() -> decltype(ae::AudioEngine::allowedBufferLengths())& {
    return ae::AudioEngine::allowedBufferLengths();
}

auto AudioEngineManager::audioDriver() -> ats::Result<std::expected<ae::audio_driver::AudioDriver, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->audioDriver();
    }) };

    auto result { task->result() };
    enqueueEngineTask(std::move(task));

    return result;
}

auto AudioEngineManager::audioDriver(const ae::audio_driver::AudioDriver newAudioDriver) -> ats::Result<std::expected<void, std::string>> {
    stopRecording();

#ifdef _WIN32
    // Engine tasks enqueued for the previous driver may still be pending on other threads:
    // none of them may run once the engine holds an ASIO driver
    waitForAllTasks();
    m_isAsioRequested.store(newAudioDriver == ae::audio_driver::AudioDriver::Asio, std::memory_order_release);
#endif

    auto task { ats::makeAtomicTask([this, newAudioDriver] () {
        std::lock_guard lock { m_taskMutex };
        auto switchResult { m_audioEngine->audioDriver(newAudioDriver) };

#ifdef _WIN32
        // On failure the engine keeps its previous driver
        const auto currentAudioDriver { m_audioEngine->audioDriver() };
        m_isAsioActive.store(currentAudioDriver.has_value() and currentAudioDriver.value() == ae::audio_driver::AudioDriver::Asio,
                             std::memory_order_release);
#endif

        return switchResult;
    }) };

    auto result { task->result() };
    enqueueEngineTask(std::move(task));

    return result;
}

auto AudioEngineManager::probeDevices() -> ats::Result<std::expected<void, std::string> > {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->probeDevices();
    }) };

    auto result { task->result() };
    enqueueEngineTask(std::move(task));

    return result;
}

auto AudioEngineManager::defaultInputAudioDeviceName() -> ats::Result<std::expected<std::string, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->defaultInputAudioDeviceName();
    }) };

    auto result { task->result() };
    enqueueEngineTask(std::move(task));

    return result;
}

auto AudioEngineManager::defaultOutputAudioDeviceName() -> ats::Result<std::expected<std::string, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->defaultOutputAudioDeviceName();
    }) };

    auto result { task->result() };
    enqueueEngineTask(std::move(task));

    return result;
}

auto AudioEngineManager::inputAudioDeviceSummaryList() -> ats::Result<std::expected<std::vector<ae::audio_device::AudioDeviceSummary>, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->inputAudioDeviceSummaryList();
    }) };

    auto result { task->result() };
    enqueueEngineTask(std::move(task));

    return result;
}

auto AudioEngineManager::outputAudioDeviceSummaryList() -> ats::Result<std::expected<std::vector<ae::audio_device::AudioDeviceSummary>, std::string>> {
    auto task { ats::makeAtomicTask([this] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->outputAudioDeviceSummaryList();
    }) };

    auto result { task->result() };
    enqueueEngineTask(std::move(task));

    return result;
}

auto AudioEngineManager::startStream(std::optional<std::string> inputDeviceName,
                                     std::optional<std::string> outputDeviceName,
                                     std::optional<std::string> loopbackDeviceName,
                                     const ae::audio_stream_params::BufferLength_t bufferLength,
                                     const ae::audio_device::SampleRate_t sampleRate) -> ats::Result<std::expected<void, std::string>> {
    stopRecording();

    auto task { ats::makeAtomicTask([this, inputName = std::move(inputDeviceName), outputName = std::move(outputDeviceName), loopbackName = std::move(loopbackDeviceName), bufferLength, sampleRate] () mutable {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->startStream(std::move(inputName), std::move(outputName), std::move(loopbackName), bufferLength, sampleRate);
    }) };

    auto result { task->result() };
    enqueueEngineTask(std::move(task));

    return result;
}

ats::ResumableTask<void> write(std::chrono::milliseconds triggerInterval, std::mutex& mutex, const std::unique_ptr<ae::AudioEngine>& audioEngine) {
    std::unique_lock writeLock { mutex, std::defer_lock };
    auto writeResult { false };
    auto triggerTime { std::chrono::steady_clock::now() + triggerInterval };

    while (true) {
        // This needs to be placed before the write operation otherwise the audio buffer to write
        // is empty. Need to get some samples in the audio buffer
        while (std::chrono::steady_clock::now() < triggerTime)
            co_await std::suspend_always {};

        writeLock.lock();
        writeResult = audioEngine->write();
        writeLock.unlock();

        if (not writeResult) co_return;

        triggerTime += triggerInterval;
        co_await std::suspend_always {};
    }
}

auto AudioEngineManager::startRecording(const ae::audio_format::AudioFormat format, std::filesystem::path recordingPath) -> std::expected<void, std::string> {
    stopRecording();

    auto startRecordingTask { ats::makeAtomicTask([this, format, path = std::move(recordingPath)] () mutable {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->startRecording(format, std::move(path));
    }) };

    const auto startRecordingResult { startRecordingTask->result() };
    enqueueEngineTask(std::move(startRecordingTask));

    if (auto taskResult { startRecordingResult.get() }; not taskResult.has_value()) {
        return taskResult;
    }

    // The write task never touches the audio library, so it can run on any thread
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
    enqueueEngineTask(std::move(stopRecordingTask));

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
    enqueueEngineTask(std::move(finalizeRecordingTask));

    finalizeRecordingTaskDependency.wait();
}

auto AudioEngineManager::inputChannelName(std::string channelName, const ae::audio_device::ChannelCount_t channelCount) -> ats::Result<void> {
    auto inputNameTask { ats::makeAtomicTask([this, name = std::move(channelName), channelCount] () {
        std::lock_guard lock { m_taskMutex };
        m_audioEngine->audioMixer()->inputName(utils::sanitizeFileName(name), channelCount);
    }) };

    const auto inputChannelNameResult { inputNameTask->result() };
    enqueueEngineTask(std::move(inputNameTask));

    return inputChannelNameResult;
}

auto AudioEngineManager::inputChannelName(const ae::audio_device::ChannelCount_t channelCount) -> ats::Result<std::string> {
    auto inputNameTask { ats::makeAtomicTask([this, channelCount] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->audioMixer()->inputName(channelCount);
    }) };

    const auto inputNameResult { inputNameTask->result() };
    enqueueEngineTask(std::move(inputNameTask));

    return inputNameResult;
}

auto AudioEngineManager::outputChannelName(std::string channelName, const ae::audio_device::ChannelCount_t channelCount) -> ats::Result<void> {
    auto outputNameTask { ats::makeAtomicTask([this, name = std::move(channelName), channelCount] () {
        std::lock_guard lock { m_taskMutex };
        m_audioEngine->audioMixer()->outputName(utils::sanitizeFileName(name), channelCount);
    }) };

    const auto outputNameResult { outputNameTask->result() };
    enqueueEngineTask(std::move(outputNameTask));

    return outputNameResult;
}

auto AudioEngineManager::outputChannelName(const ae::audio_device::ChannelCount_t channelCount) -> ats::Result<std::string> {
    auto outputNameTask { ats::makeAtomicTask([this, channelCount] () {
        std::lock_guard lock { m_taskMutex };
        return m_audioEngine->audioMixer()->outputName(channelCount);
    }) };

    const auto outputNameResult { outputNameTask->result() };
    enqueueEngineTask(std::move(outputNameTask));

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

auto AudioEngineManager::enqueueEngineTask(std::unique_ptr<ats::AsyncTask> task) -> void {
#ifdef _WIN32
    if (isAsioInvolved()) {
        enqueueTasks(kAsioThreadId, std::move(task));
        return;
    }
#endif

    enqueueTasks(std::move(task));
}

#ifdef _WIN32
auto AudioEngineManager::isAsioInvolved() const -> bool {
    return m_isAsioRequested.load(std::memory_order_acquire) or m_isAsioActive.load(std::memory_order_acquire);
}
#endif

auto makeAudioEngineManager(ats::AsyncTaskScheduler& scheduler,
                            LoggerManager& loggerManager) -> std::expected<std::unique_ptr<AudioEngineManager>, std::string> {
    try {
        return std::make_unique<AudioEngineManager>(scheduler, loggerManager);
    } catch (const std::exception& e) {
        return std::unexpected { std::string { e.what() } };
    }
}

}
