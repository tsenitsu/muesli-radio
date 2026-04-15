export module audio_engine_manager;

import std;

import task_manager;
import async_task_scheduler;
import audio_engine;

namespace ae = audio_engine;
namespace ats = async_task_scheduler;

namespace managers {

export class AudioEngineManager final: public ats::TaskManager {
public:
    AudioEngineManager(ats::AsyncTaskScheduler& scheduler, const ae::audio_library_wrapper::LogCallback& logCallback);
    ~AudioEngineManager() override;

    [[nodiscard]] auto audioDriver(ae::audio_driver::AudioDriver newAudioDriver) -> ats::Result<std::expected<void, std::string>>;
    [[nodiscard]] auto audioDriver() -> ats::Result<std::expected<ae::audio_driver::AudioDriver, std::string>>;

    [[nodiscard]] auto probeDevices() -> ats::Result<std::expected<void, std::string>>;

    [[nodiscard]] auto defaultInputAudioDeviceName() -> ats::Result<std::expected<std::string, std::string>>;
    [[nodiscard]] auto defaultOutputAudioDeviceName() -> ats::Result<std::expected<std::string, std::string>>;

    [[nodiscard]] auto startStream(const std::optional<std::string>& inputDeviceName,
        const std::optional<std::string>& outputDeviceName, ae::audio_stream_params::BufferLength_t bufferLength) -> ats::Result<std::expected<void, std::string>>;

    [[nodiscard]] auto startRecording(ae::audio_format::AudioFormat format) -> std::expected<void, std::string>;
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

private:
    std::mutex m_taskMutex;
    ae::audio_library_wrapper::LogCallback m_logCallback;
    std::unique_ptr<ae::AudioEngine<ae::audio_library_wrapper::MiniaudioLibraryWrapper>> m_audioEngine;
    std::optional<ats::Dependency> m_writeTaskDependency;
};

export [[nodiscard]] auto makeAudioEngineManager(ats::AsyncTaskScheduler& scheduler,
    const ae::audio_library_wrapper::LogCallback& logCallback) -> std::expected<std::unique_ptr<AudioEngineManager>, std::string>;

}