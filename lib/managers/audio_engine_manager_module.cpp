export module audio_engine_manager;

import std;

import task_manager;
import async_task_scheduler;
import audio_engine;
import logger_manager;

namespace ae = audio_engine;
namespace ats = async_task_scheduler;

namespace managers {

export class AudioEngineManager final: public ats::TaskManager {
public:
    AudioEngineManager(ats::AsyncTaskScheduler& scheduler, LoggerManager& loggerManager);
    ~AudioEngineManager() override;

    [[nodiscard]] static auto allowedSampleRates() -> decltype(ae::AudioEngine::allowedSampleRates())&;
    [[nodiscard]] static auto allowedBufferLengths() -> decltype(ae::AudioEngine::allowedBufferLengths())&;

    [[nodiscard]] auto audioDriver(ae::audio_driver::AudioDriver newAudioDriver) -> ats::Result<std::expected<void, std::string>>;
    [[nodiscard]] auto audioDriver() -> ats::Result<std::expected<ae::audio_driver::AudioDriver, std::string>>;

    [[nodiscard]] auto probeDevices() -> ats::Result<std::expected<void, std::string>>;

    [[nodiscard]] auto defaultInputAudioDeviceName() -> ats::Result<std::expected<std::string, std::string>>;
    [[nodiscard]] auto defaultOutputAudioDeviceName() -> ats::Result<std::expected<std::string, std::string>>;

    [[nodiscard]] auto inputAudioDeviceSummaryList() -> ats::Result<std::expected<std::vector<ae::audio_device::AudioDeviceSummary>, std::string>>;
    [[nodiscard]] auto outputAudioDeviceSummaryList() -> ats::Result<std::expected<std::vector<ae::audio_device::AudioDeviceSummary>, std::string>>;

    [[nodiscard]] auto startStream(std::optional<std::string> inputDeviceName,
                                   std::optional<std::string> outputDeviceName,
                                   std::optional<std::string> loopbackDeviceName,
                                   ae::audio_stream_params::BufferLength_t bufferLength,
                                   ae::audio_device::SampleRate_t sampleRate) -> ats::Result<std::expected<void, std::string>>;

    [[nodiscard]] auto startRecording(ae::audio_format::AudioFormat format, std::filesystem::path path) -> std::expected<void, std::string>;
                  auto stopRecording() -> void;

    [[nodiscard]] auto inputChannelName(std::string channelName, ae::audio_device::ChannelCount_t channelCount) -> ats::Result<void>;
    [[nodiscard]] auto inputChannelName(ae::audio_device::ChannelCount_t channelCount) -> ats::Result<std::string>;

    [[nodiscard]] auto outputChannelName(std::string channelName, ae::audio_device::ChannelCount_t channelCount) -> ats::Result<void>;
    [[nodiscard]] auto outputChannelName(ae::audio_device::ChannelCount_t channelCount) -> ats::Result<std::string>;

                  auto inputChannelGain(float gain, ae::audio_device::ChannelCount_t channelCount) const -> void;
    [[nodiscard]] auto inputChannelGain(ae::audio_device::ChannelCount_t channelCount) const -> float;

                  auto outputChannelGain(float gain, ae::audio_device::ChannelCount_t channelCount) const -> void;
    [[nodiscard]] auto outputChannelGain(ae::audio_device::ChannelCount_t channelCount) const -> float;

                  auto inputChannelRouting(std::pair<ae::audio_mixer::ChannelRouting, ae::audio_mixer::ChannelRouting> routing,
                      ae::audio_device::ChannelCount_t channelCount) const -> void;
    [[nodiscard]] auto inputChannelRouting(ae::audio_device::ChannelCount_t channelCount) const ->
        std::pair<ae::audio_mixer::ChannelRouting,ae::audio_mixer::ChannelRouting>;

                  auto outputChannelRouting(ae::audio_mixer::ChannelRouting routing, ae::audio_device::ChannelCount_t channelCount) const -> void;
    [[nodiscard]] auto outputChannelRouting(ae::audio_device::ChannelCount_t channelCount) const -> ae::audio_mixer::ChannelRouting;

                  auto inputLevels(std::span<float> inputLevels) const -> void;
                  auto outputLevels(std::span<float> outputLevels) const -> void;

private:
    // Engine tasks run on kAsioThreadId while ASIO is involved, on a random thread otherwise
    auto enqueueEngineTask(std::unique_ptr<ats::AsyncTask> task) -> void;

#ifdef _WIN32
    // True from the moment ASIO is requested until the engine no longer uses it
    [[nodiscard]] auto isAsioInvolved() const -> bool;

    // ASIO drivers are COM objects bound to the apartment of the thread that created them,
    // so every engine task must run on the same thread while ASIO is in use
    static constexpr auto kAsioThreadId { 0u };
#endif

    std::mutex m_taskMutex;
    LoggerManager& m_loggerManager;
    ae::audio_library_wrapper::LogCallback m_logCallback;
    std::unique_ptr<ae::AudioEngine> m_audioEngine;
    std::optional<ats::Dependency> m_writeTaskDependency;
#ifdef _WIN32
    // Set by audioDriver() before its task is enqueued
    std::atomic_bool m_isAsioRequested { false };
    // Set by the driver change task, from the driver the engine actually ended up with
    std::atomic_bool m_isAsioActive { false };
#endif
};

export [[nodiscard]] auto makeAudioEngineManager(ats::AsyncTaskScheduler& scheduler,
    LoggerManager& loggerManager) -> std::expected<std::unique_ptr<AudioEngineManager>, std::string>;

}